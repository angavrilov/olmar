// cc_parse.cc
// toplevel driver for the C/C++ parser

#include "trace.h"     // traceAddSys
#include "glr.h"       // StackNode::printAllocStats
#include "cc.h"        // details of C++ parse tree
#include "ckheap.h"    // checkHeap
#include "mysig.h"     // signal handling
#include "stmt2bb.h"   // doTranslationStuff


// little hack to get more info when the parser crashes
int lastLineChecked = -1;


// where to start emitting types
enum {                   
  // this makes us print everything
  //FIRST_EMITTED_TYPE = 0

  // with this one, we don't emit info about the simple types; we'll
  // take that as implicit (and it clutters the output for small
  // examples)
  FIRST_EMITTED_TYPE = NUM_SIMPLE_TYPES
};


// global translation state; initialized before we start
// processing the file, and updated as we process each
// toplevel declaration
class TranslationState {
public:
  Env *globalEnv;             // global environment (incl. ptr to type env)
  TypeId nextType;            // id of next one to emit
  AtomicTypeId nextAtomic;    // similarly for atomic types
  CilProgram prog;            // Cil translation of this module

  struct wes_state *wes;      // stuff related to the ocaml translation

public:
  TranslationState(Env *e, wes_state *w)
    : globalEnv(e),
      nextType(FIRST_EMITTED_TYPE),
      nextAtomic(FIRST_EMITTED_TYPE),
      prog(),
      wes(w)
  {}
  ~TranslationState();
  
  void printTree();
};


// stuff related to exporting data to ocaml
#ifdef WES_OCAML_LINKAGE
  #include "my_caml.h"     // ML interface
  struct wes_ast_node {
      const char *name;
      int num_children;
      int line, col;
      struct wes_ast_node ** children;
  };
  struct wes_list {
      struct wes_ast_node * hd;
      struct wes_list * tl;
  };

  // global translation state for ocaml stuff
  struct wes_state {
    bool want_cil;                      // true:Cil; false:C-parse-tree
    struct wes_list *result_list;       // toplevel C-parse-tree result
  };

  extern "C" { value cil_program_to_ocaml(CilProgram *prog); }
#endif /* WES_OCAML_LINKAGE */


struct wes_ast_node *emptyCamlAST()
{
  #ifdef WES_OCAML_LINKAGE
    struct wes_ast_node * retval = new struct wes_ast_node;
    retval->name = "empty parse tree";
    retval->num_children = 0;
    retval->children = NULL;
    retval->line = retval->col = 0;
    return retval;
  #else /* WES_OCAML_LINKAGE */
    return NULL;
  #endif /* WES_OCAML_LINKAGE */
}


TranslationState::~TranslationState()
{}


void TranslationState::printTree()
{
  if (tracingSys("cil-tree")) {
    // emit types that are new since the last type we did this
    TypeEnv *te = globalEnv->getTypeEnv();

    // the current plan is to emit typedefs for named atomics
    // (structs and enums) only, so let's not print these by
    // default
    if (tracingSys("cil-nonatomic-types")) {
      while (nextType < te->numTypes()) {
        Type *t = te->lookup(nextType);
        cout << "type " << nextType << ": "
             << t->toMLValue(1 /*depth*/) << ";\n";
        nextType++;
      }
    }

    while (nextAtomic < te->numAtomicTypes()) {
      AtomicType *at = te->lookupAtomic(nextAtomic);
      cout << "atomicType " << nextAtomic << ": "
           << at->toMLValue(2 /*depth*/, CV_NONE) << ";\n";
      nextAtomic++;
    }

    prog.printTree(0 /*indent*/, cout, true /*ml*/);
  }
}


// this fn is called each time we parse a C/C++ toplevel
// declaration, which is is typically a function definition,
// but there are other possibilities like globals or prototypes
//   red    - the reduction that is the root of the decl; it's
//            already been added to the 'reductions' list
//   tree   - essentially, parameters global to the parse;
//            in particular, carries the toplevel Env structure
void TranslationUnit_Node::analyzeToplevelDecl(Reduction *red, ParseTree &tree)
{
  // if we just constructed the "-> empty" production,
  // do nothing special since this is just a degenerate tree
  if (red->children.isEmpty()) {
    return;
  }

  // optionally print tree *before* typeChecking
  if (tracingSys("print-tree-before")) {
    printTree();
  }

  // cast context argument
  TranslationState &state = *((TranslationState*)(tree.extra));
  env = state.globalEnv;

  // clear the existing dataflow environment (effect is we
  // don't analyze globals ....)
  env->getDenv().reset();

  // debugging..
  trace("typeCheck") << locString() << endl;
  lastLineChecked = loc()->line;

  // run the analysis on this piece of the tree
  delete typeCheck(env, CilContext(state.prog));

  #ifdef WES_OCAML_LINKAGE
    if (state.wes->want_cil) {
        state.wes->result_list = (struct wes_list *) cil_program_to_ocaml(&state.prog);
        register_global_root((value *)&(state.wes->result_list));
    } else {
        struct wes_list * new_cell = new struct wes_list;
        new_cell->hd = camlAST();
        new_cell->tl = state.wes->result_list;
        state.wes->result_list = new_cell;
    }
  #endif /* WES_OCAML_LINKAGE */

  // this is, more or less, the product of the analysis
  // (removed the #ifdef's because nothing is printed
  // unless certain tracing flags are set)
  printTree();

  // print Cil if we want that
  //state.printTree();

  // yeeha! translation!
  MUTATE_EACH_OBJLIST(CilFnDefn, state.prog.funcs, iter) {
    doTranslationStuff(*( iter.data() ));
  }

  // can delete the per-fn info, or keep it; when this
  // is commented-out, we keep it all until the end
  //state.prog.empty();

  #if 0      // for tracking down leaks
  CilExpr::printAllocStats(false /*anyway*/);
  CilInst::printAllocStats(false /*anyway*/);
  CilStmt::printAllocStats(false /*anyway*/);
  #endif // 0


  // ---- throw away most of the tree now ----
  // grab a pointer to the left child reduction, which should be
  // an instance of "TranslationUnit -> empty"
  NonterminalNode *leftNT = &( red->children.nth(0)->asNonterm() );
  Reduction *leftReduction = leftNT->only();

  // the tree right now looks like this:
  //             'this': NonterminalNode
  //                         |
  // 'red': TranslationUnit -> TranslationUnit Declaration
  //                            /                   \           .
  //      'leftNT': NonterminalNode      (code just analyzed)
  //                           |
  // 'leftReduction': TranslationUnit -> empty
  //                      (no children)

  // of these, all the NonterminalNodes are in the 'treeNodes'
  // list except for 'this'

  // let's verify some of this
  xassert(reductions.count() == 1);
  xassert(red->children.count() == 2);
  xassert(leftNT->reductions.count() == 1);
  xassert(leftReduction->children.count() == 0);

  // the goal is to throw away everything except for
  // 'leftNT' and below .. to maintain the invariant that
  // all NonterminalNodes have at least one reduction,
  // we'll simply swap 'red' and 'leftReduction'

  // first, remove from both..
  this->reductions.removeItem(red);
  leftNT->reductions.removeItem(leftReduction);

  // .. now add back to opposite one
  this->reductions.prepend(leftReduction);
  leftNT->reductions.prepend(red);

  // blow away all the other tree nodes; this deletes everything
  // except 'this' and its newly-acquired 'leftReduction'

  // we must kill the parent links first (this is, in essence
  // the *big* *fix* to my earlier corruption errors)
  MUTATE_EACH_OBJLIST(TreeNode, tree.treeNodes, iter) {
    iter.data()->killParentLink();
  }

  // now do the real deallocations
  tree.treeNodes.deleteAll();
}


// ------------------ main, etc ------------------------
CilExpr *todoCilExpr(CilExtraInfo tn)
{
  // rather than segfaulting, let's make something reasonable
  return newIntLit(tn, 12345678);    // hopefully recognizable
}

CilStmt *todoCilStmt(CilExtraInfo tn)
{
  return newLabel(tn, "unimplementedStmt");
}


int fakemain(int argc, char *argv[], struct wes_state *wes)
{
  // by default, print progress reports
  traceAddSys("progress");

  // do this before creating the first environment
  // (this messes up the usage string printed by treeMain
  // but who cares)
  while (traceProcessArg(argc, argv))
    {}

  if (setjmp(sane_state) == 0) {    // C's version of "try"
    setHandler(SIGSEGV, jmpHandler);
    try {
      // this program's exception-handling structure is mature
      // enough that printing all thrown exceptions has crossed
      // the line from helpful to annoying
      xBase::logExceptions = false;
      if (tracingSys("eh")) {
        xBase::logExceptions = true;     // sometimes I want them .. :)
      }

      // place to store the tree
      ParseTreeAndTokens tree;

      // toplevel type environment
      TypeEnv topTypeEnv;

      // global variable environment
      VariableEnv topVarEnv;

      // dataflow environment ...
      DataflowEnv denv;

      // toplevel environment, in which each toplevel form
      // will be evaluated, just before that form is thrown
      // away
      //traceAddSys("env-declare");
      Env env(&denv, &topTypeEnv, &topVarEnv);

      // carry this context in the tree, since it is an argument
      // to node constructors
      TranslationState state(&env, wes);
      tree.extra = &state;

      // use another try to catch ambiguities so we can
      // report them before the tree is destroyed
      try {
        // this sets everything in motion, including analysis of
        // tree, etc; it returns when the entire tree has been
        // processed
        if (!treeMain(tree, argc, argv)) {
          return 2;    // parse error
        }
        
        // it's now parsed .. print out the Cil translation
        state.printTree();
      }
      catch (XAmbiguity &x) {
        cout << x << endl;
        cout << "last line checked: " << lastLineChecked << endl;
        return 4;
      }
    }
    catch (xBase &x) {
      cout << "died on exception: " << x << endl;
      cout << "last line checked: " << lastLineChecked << endl;
      return 6;
    }
  }
  else {       // caught a signal
    cout << "exiting due to signal\n";
    cout << "last line checked: " << lastLineChecked << endl;
    return 7;
  }
  
  // restore the default handler, because we're about to exit
  // the scope of the sane_state jmp_buf
  setHandler(SIGSEGV, SIG_DFL);

  // since this is outside all the scopes that create interesting
  // nodes, the stat counts should now be 0
  bool anyway = tracingSys("allocStats");
  if (anyway || TreeNode::numTreeNodesAllocd != 0) {
    TreeNode::printAllocStats();
  }
  if (anyway || StackNode::numStackNodesAllocd != 0) {
    StackNode::printAllocStats();
  }
  CilExpr::printAllocStats(anyway);
  CilInst::printAllocStats(anyway);
  CilStmt::printAllocStats(anyway);
  CilBB::printAllocStats(anyway);
  Type::printAllocStats(anyway);
  AtomicType::printAllocStats(anyway);

  return 0;
}


// toplevel entries to the parser for the ocaml code
#ifdef WES_OCAML_LINKAGE
extern "C" {

value c_to_ocaml(struct wes_ast_node *n, char *filename)
{
    CAMLparam0();
    CAMLlocal5(result,nname,arr,fname,child);
    int i;

    result = alloc(5,0);
    nname = my_copy_string(n->name);
    fname = my_copy_string(filename);
    Store_field(result, 0, nname);
    Store_field(result, 2, Val_int(n->line));
    Store_field(result, 3, Val_int(n->col));
    Store_field(result, 4, fname);
    if (n->num_children == 0) {
	Store_field(result, 1, Atom(0));
	CAMLreturn(result);
    } else {
	arr = alloc(n->num_children, 0);
	for (i=0;i<n->num_children;i++) {
	    child = c_to_ocaml(n->children[i],filename);
	    modify(&Field(arr,i), child);
	}
	Store_field(result, 1, arr);
	CAMLreturn(result);
    }
}

void wes_print(struct wes_ast_node * n)
{
    int i;
    printf("(%s",n->name);
    for (i=0;i<n->num_children;i++)
	wes_print(n->children[i]);
    printf(")");
}


value wes_main(char *filename) {
    CAMLparam0();
    CAMLlocal2(retval, c);
    char *argv[5] = {
	"cc.gr",
	"-tr",
	"",	/* wes: was parse-tree,sizeof */
        "cc.bin",
	filename};
    int argc = 5;
    int res;
    struct wes_list * l;

    struct wes_state wes;
    wes.result_list = NULL;
    wes.want_cil = false;

    res = fakemain(argc, argv, &wes);
    // examine wes.result_list

#if 0
    for (l = wes.result_list; l != NULL; l = l->tl) {
	struct wes_ast_node * n = l->hd;
	wes_print(n);
    }
    printf("\n\n");
#endif

    // convert it over to OCAML
    c = Val_int(0);
    for (l = wes.result_list; l != NULL; l = l->tl) {
	struct wes_ast_node * n = l->hd;
	retval = alloc(2,0);
	Store_field(retval, 0, c_to_ocaml(n,filename));
	Store_field(retval, 1, c);
	c = retval;
    }
    wes.result_list = NULL; // memory leak!
    CAMLreturn(c);
}

#include "cil_to_ocaml.c"

value cil_main(char *filename) {
    CAMLparam0();
    CAMLlocal3(retval, c, val);
    char *argv[5] = {
	"cc.gr",
	"-tr",
	"",	/* wes: was parse-tree,sizeof */
        "cc.bin",
	filename};
    int argc = 5;
    int res;

    struct wes_state wes;
    wes.result_list = NULL;
    wes.want_cil = true;

    res = fakemain(argc, argv, &wes);
    // examine wes.result_list

    // convert it over to OCAML
    c = (value) wes.result_list;

    printf("Returning from C land with value %p ...\n", wes.result_list);

    wes.result_list = NULL;

    CAMLreturn(c);
}

} // extern "C"

#else /* !WES_OCAML_LINKAGE */

// 'main' for a standalone executable
int main(int argc, char *argv[])
{
  //fakemain(argc, argv);
  return fakemain(argc, argv, NULL /*wes*/);
}

#endif /* !WES_OCAML_LINKAGE */

