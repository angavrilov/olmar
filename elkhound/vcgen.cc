// vcgen.cc
// vcgen methods on the C abstract syntax tree

#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // abstract domain values
#include "aenv.h"               // AEnv
#include "cc_type.h"            // FunctionType, etc.
#include "sobjlist.h"           // SObjList
#include "trace.h"              // tracingSys
#include "strutil.h"            // quoted, plural
#include "paths.h"              // countExprPaths
#include "predicate.ast.gen.h"  // Predicate ast, incl. P_equal, etc.

#define IN_PREDICATE(env) Restorer<bool> restorer(env.inPredicate, true)


// use for places that aren't implemented yet
AbsValue *avTodo()
{
  return new AVint(12345678);   // hopefully stands out in debug output
}


// ------------------- TranslationUnit ---------------------
void TranslationUnit::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(TopForm, topForms, iter) {
    iter.data()->vcgen(env);
  }
}


// --------------------- TopForm ----------------------
void TF_decl::vcgen(AEnv &env) const
{
  decl->vcgen(env);
}


void TF_func::vcgen(AEnv &env) const
{
  env.currentFunc = this;
  FunctionType const &ft = *(ftype());

  int numRoots = roots.count();
  traceProgress() << "analyzing " << name()
                  << " (" << numPaths << plural(numPaths, " path")
                  << " total from " << numRoots << plural(numRoots, " root")
                  << ") ...\n";

  // synthesized logic variable for return value
  env.result = nameParams->decl->asD_func()->result;

  // for each path...
  int rootIndex=0;
  SFOREACH_OBJLIST(Statement, roots, rootIter) {
    Statement const *root = rootIter.data();
    rootIndex++;

    traceProgress() << "  root " << rootIndex << "/" << numRoots
                    << ", " << root->numPaths
                    << plural(root->numPaths, " path") << "\n";

    // for each path from that root...
    for (int path=0; path < root->numPaths; path++) {
      traceProgress() << "    path " << path << "\n";

      if (tracingSys("printAnalysisPath")) {
        SObjList<Statement> nodeList;
        printPathFrom(nodeList, path /*index*/, root, false /*isContinue*/);
      }

      // ----------- build the abstract environment ----------
      // clear anything left over in env
      env.clear();

      // ------------- establish set of known facts ---------------
      // add the path's start predicate as an assumption
      if (root == body) {
        // root is start of function; precondition is start predicate
        if (ft.precondition) {
          IN_PREDICATE(env);

          // add any precondition bindings
          FOREACH_ASTLIST(Declaration, ft.precondition->decls, iter) {
            iter.data()->vcgen(env);
          }

          env.addFact(ft.precondition->expr->vcgenPred(env, 0 /*path*/),
                      "precondition");
        }
      }
      else {
        // the root should be an invariant statement, and that is
        // the start predicate
        S_invariant const *inv = root->asS_invariantC();

        IN_PREDICATE(env);
        env.addFact(inv->expr->vcgenPred(env, 0 /*path*/), "invariant");
      }

      // -------------- interpret the code ------------
      // now interpret the function body
      SObjList<Statement /*const*/> stmtList;
      root->vcgenPath(env, stmtList, path, false /*cont*/);

      // NOTE: the path's termination predicate will be proven
      // inside the vcgenPath call above

      // print the results
      if (tracingSys("absInterp")) {
        cout << "interpretation path:\n";
        env.print();
      }
    }
  }

  env.result = NULL;
  env.currentFunc = NULL;
}


// --------------------- Declaration --------------------
// this is only used for globals; local declarations are
// taken apart by S_decl
void Declaration::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(Declarator, decllist, iter) {
    Declarator const *d = iter.data();
    if (d->init) {
      d->vcgen(env, d->init->vcgen(env, d->var->type, 0));
    }
    else {
      // declarator's vcgen just acts like an assignment, so if
      // there's no initializing expression, we don't do anything here
    }
  }
}

AVvar *fieldRep(CompoundType::Field const *field)
{
  return new AVvar(field->name, "field value representative");
}

void Declarator::vcgen(AEnv &env, AbsValue *value) const
{
  // treat the declaration as an assignment; the variable was added
  // to the abstract environment in TF_func::vcgen
  env.updateVar(var, value);
}


// ----------------------- Statement ----------------------
// eval 'expr' in a predicate (theorem-proving) context
Predicate *vcgenPredicate(AEnv &env, Expression *expr, int path)
{
  if (path != 0) {
    // used to be just an assertion, but then I needed more info...
    cout << "(path=" << path
         << ") predicate expr problem: " << expr->toString() << endl;
    xfailure("bad mojo");
  }

  IN_PREDICATE(env);
  return expr->vcgenPred(env, path);
}


// uber-vcgen: generic, as it uses the embedded CFG; but it
// calls the specific 'vcgen' for the particular statement kind
void Statement::vcgenPath(AEnv &env, SObjList<Statement /*const*/> &path,
                          int index, bool isContinue) const
{
  // validate 'index'
  int exprPaths = countExprPaths(this, isContinue);
  xassert(exprPaths >= 1);
  xassert(0 <= index && index < (numPaths * exprPaths));

  // debugging check
  if (path.contains(this)) {
    cout << "  CIRCULAR path\n";
    return;
  }
  path.prepend(const_cast<Statement*>(this));

  // pick which expression path we will follow
  int exprPath = index % exprPaths;
  index = index / exprPaths;

  // retrieve all successors of this node
  VoidList successors;
  getSuccessors(successors, isContinue);

  if (successors.isEmpty()) {
    // this is a return statement (or otherwise end of function)
    xassert(index == 0);

    // vcgen this statement, telling it no continuation path
    vcgen(env, isContinue, exprPath, NULL);
    
    // prove the function postcondition
    FA_postcondition const *post = env.currentFunc->ftype()->postcondition;
    if (post) {
      IN_PREDICATE(env);
      env.prove(post->expr->vcgenPred(env, 0 /*path*/), "postcondition");
    }
  }
  else {
    // consider each choice
    for (VoidListIter iter(successors); !iter.isDone(); iter.adv()) {
      void *np = iter.data();
      Statement const *s = nextPtrStmt(np);

      // are we going to follow 's'?
      if (index < s->numPaths) {
        // yes; vcgen 'node', telling it 's' as the continuation
        vcgen(env, isContinue, exprPath, s);

        // is 's' an invariant?
        if (s->isS_invariant()) {
          // terminate the path & prove the invariant
          env.prove(vcgenPredicate(env, s->asS_invariantC()->expr, exprPath),   // bugfix: had "index" where meant "exprPath" (it's like vcgen call above)
                    stringc << "invariant at " << s->loc.toString());
        }
        else {
          // continue the path
          s->vcgenPath(env, path, index, nextPtrContinue(np));
        }
        index = 0;       // remember that we found a path to follow
        break;
      }

      else {
        // no; factor out s's contribution to the path index
        index -= s->numPaths;
      }
    }

    // make sure we followed *some* path
    xassert(index == 0);
  }

  path.removeFirst();
}


void S_skip::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }

// control-flow targets add nothing; implied predicates are
// added by the preceeding statement
void S_label::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }
void S_case::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); } 
void S_caseRange::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }
void S_default::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }


void S_expr::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate and discard
  env.discard(expr->vcgen(env, path));
}


void S_compound::vcgen(STMT_VCGEN_PARAMS) const
{
  // this statement does nothing, because successor info
  // is already part of the CFG
  xassert(path == 0);
}


// canonical statement vcgen: hit the expression language in
// the guard, and then add a predicate based on outgoing edge
void S_if::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate the guard
  Predicate *pred = cond->vcgenPred(env, path);

  if (next == thenBranch) {
    // remember that the guard was true
    env.addFact(pred, stringc << "true guard of 'if' at " << loc.toString());
  }
  else {
    // guard is false
    xassert(next == elseBranch);
    env.addFalseFact(pred, stringc << "false guard of 'if' at " << loc.toString());
  }
}


void S_switch::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate switch-expression
  expr->vcgen(env, path);

  // for now, learn nothing from the outgoing edge
  cout << "TODO: ignoring implication of followed switch edge\n";
}


void S_while::vcgen(STMT_VCGEN_PARAMS) const
{
  Predicate *pred = cond->vcgenPred(env, path);

  if (next == body) {
    env.addFact(pred, stringc << "true guard of 'while' at " << loc.toString());
  }
  else {
    // 'next' is whatever follows the loop
    env.addFalseFact(pred, stringc << "false guard of 'while' at " << loc.toString());
  }
}


void S_doWhile::vcgen(STMT_VCGEN_PARAMS) const
{
  if (!isContinue) {
    // drop straight into body, learn nothing
    xassert(path == 0);
  }
  else {
    Predicate *pred = cond->vcgenPred(env, path);
    if (next == body) {
      env.addFact(pred, stringc << "true guard of 'do' at " << loc.toString());
    }
    else {
      env.addFalseFact(pred, stringc << "false guard of 'do' at " << loc.toString());
    }
  }
}


void S_for::vcgen(STMT_VCGEN_PARAMS) const 
{
  int modulus = cond->numPaths1();

  if (!isContinue) {
    // init
    init->vcgen(env, false, path / modulus, NULL);
  }
  else {
    // inc
    after->vcgen(env, path / modulus);
  }

  // guard
  Predicate *pred = cond->vcgenPred(env, path % modulus);

  // body?
  if (next == body) {
    env.addFact(pred, stringc << "true guard of 'for' at " << loc.toString());
  }
  else {
    env.addFalseFact(pred, stringc << "false guard of 'for' at " << loc.toString());
  }
}


void S_break::vcgen(STMT_VCGEN_PARAMS) const { xassert(path==0); }
void S_continue::vcgen(STMT_VCGEN_PARAMS) const { xassert(path==0); }

void S_return::vcgen(STMT_VCGEN_PARAMS) const
{
  xassert(next == NULL);
  if (expr) {
    // model as an assignment to 'result'
    env.set(env.result, expr->vcgen(env, path));
  }
  else {
    xassert(path == 0);
  }
}


void S_goto::vcgen(STMT_VCGEN_PARAMS) const { xassert(path==0); }


void S_decl::vcgen(STMT_VCGEN_PARAMS) const
{
  int subexpPaths = 1;
  FOREACH_ASTLIST(Declarator, decl->decllist, dcltr) {
    Declarator const *d = dcltr.data();

    AbsValue *initVal;
    if (d->init) {
      subexpPaths = countExprPaths(d->init);

      // evaluate 'init' to an abstract value; the initializer
      // will need to know which type it's constructing
      initVal = d->init->vcgen(env, d->var->type, path % subexpPaths);

      path = path / subexpPaths;
    }
    else {
      // make up a new name for the uninitialized value
      // (if it's global we get to assume it's 0... not implemented..)
      initVal = env.freshVariable(d->var->name, 
        stringc << "UNINITialized value of var " << d->var->name);
    }

    // add a binding for the variable
    dcltr.data()->vcgen(env, initVal);
  }

  // if we entered the loop at all, this ensures the last iteration
  // consumed all of 'path'; if we never entered the loop, this
  // checks that path was 0
  xassert(path < subexpPaths);
}


void S_assert::vcgen(STMT_VCGEN_PARAMS) const
{
  // map the expression to my abstract domain
  Predicate *p = vcgenPredicate(env, expr, path);
  xassert(p);

  // try to prove it (is not equal to 0)
  env.prove(p, stringc << "assertion at " << loc.toString());
}

void S_assume::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate
  Predicate *p = vcgenPredicate(env, expr, path);
  xassert(p);

  // remember it as a known fact
  env.addFact(p, stringc << "thmprv_assume at " << loc.toString());
}

void S_invariant::vcgen(STMT_VCGEN_PARAMS) const
{
  // we only call the 'vcgen' of an invariant at the *start* of a path
  //env.addFact(vcgenPredicate(env, expr, path));
  
  // update: I'll let the path driver take care of both adding premises,
  // and proving obligations; so s_invariant itself just holds onto the
  // expression
}

void S_thmprv::vcgen(STMT_VCGEN_PARAMS) const
{
  xassert(path==0);
  IN_PREDICATE(env);
  s->vcgen(env, false, path, NULL);
}


// ---------------------- Expression -----------------
AbsValue *E_intLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(i));
}

AbsValue *E_floatLit::vcgen(AEnv &env, int path) const { return avTodo(); }
AbsValue *E_stringLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  // make a new variable to stand for this string's object (address)
  AbsValue *object = env.freshVariable("str", stringc << "address of " << quoted(s));
  env.addDistinct(object);

  // assert the length of this object is the length of the string (w/ null)
  env.addFact(P_equal(env.avLength(object),
                      env.avInt(strlen(s)+1)), "string literal length");

  // assert that the first zero is (currently?) at the end
  env.addFact(P_equal(env.avFirstZero(env.getMem(), object),
                      env.avInt(strlen(s)+1)), "string literal zero");

  // the result of this expression is a pointer to the string's start
  return env.avPointer(object, new AVint(0));
}


AbsValue *E_charLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(c));
}

//AbsValue *E_structLit::vcgen(AEnv &env, int path) const { return avTodo(); }

AbsValue *E_variable::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  if (var->type->isFunctionType()) {
    // this is the name of a function.. let's say for each global
    // function we have a logic variable of the same name which
    // represents its address
    return env.grab(new AVvar(var->name, "address of global function"));
  }

  if (!env.isMemVar(var)) {
    // ordinary variable: look up the current abstract value for this
    // variable, and simply return that
    return env.get(var);
  }
  else if (var->type->isArrayType()) {
    // is name of an array, in which case we return the name given
    // to the array's location (it's like an immutable pointer)
    return env.avPointer(env.getMemVarAddr(var), new AVint(0));
  }
  else {
    // memory variable: return an expression which will read it out of memory
    return env.avSelect(env.getMem(), env.getMemVarAddr(var), new AVint(0));
  }
}


AbsValue *E_funCall::vcgen(AEnv &env, int path) const
{
  // evaluate the argument expressions, taking into consideration
  // which path to follow
  SObjList<AbsValue> argExps;
  {
    // 'func'
    int subexpPaths = func->numPaths1();
    func->vcgen(env, path % subexpPaths);   // ignore value; my analysis only uses type info
    path = path / subexpPaths;

    // args
    FOREACH_ASTLIST(Expression, args, iter) {
      subexpPaths = iter.data()->numPaths1();
      argExps.prepend(iter.data()->vcgen(env, path % subexpPaths));
      path = path / subexpPaths;
    }
    xassert(path < subexpPaths);

    argExps.reverse();      // when it's easy to stay linear..
  }

  FunctionType const &ft = func->type->asRval()->asFunctionTypeC();

  // --------------- predicate: fn symbol application ----------------
  if (env.inPredicate) {
    StringRef funcName = func->asE_variable()->name;
    AVfunc *f = new AVfunc(funcName, NULL);
    while (argExps.isNotEmpty()) {
      f->args.append(argExps.removeAt(0));
    }
    return f;
  }

  // -------------- modify environment for pre/post ----------------
  // the pre- and postconditions get evaluated to an abstract value in
  // an environment augmented to include parameters (and in the case
  // of the postcondition, 'result'); I do *not* create another
  // environment, because I can't possibly get confused by variable
  // names, since every variable in the program has a unique name
  SObjListMutator<AbsValue> argExpIter(argExps);
  FOREACH_OBJLIST(FunctionType::Param, ft.params, paramIter) {
    // bind the names to the expressions that are passed
    env.set(paramIter.data()->decl,
            argExpIter.data());

    argExpIter.adv();
  }

  // let pre_mem = mem
  //newEnv.set(env.str("pre_mem"), newEnv.getMem());

  // ----------------- prove precondition ---------------
  IN_PREDICATE(env);

  if (ft.precondition) {
    // include precondition bindings in the environment
    FOREACH_ASTLIST(Declaration, ft.precondition->decls, iter) {
      iter.data()->vcgen(env);
    }

    // finally, interpret the precondition to arrive at a predicate to
    // prove
    Predicate *predicate = ft.precondition->expr->vcgenPred(env, 0);
    xassert(predicate);      // tcheck should have verified we can represent it

    // prove this predicate
    env.prove(predicate, stringc << "precondition of call: " << toString());
  }

  // -------------- conservatively destroy state ------------
  // NOTE: by keeping the environment from before, we are interpreting
  // all references to parameters as their values in the *pre* state
  env.forgetAcrossCall(this);

  // ----------------- assume postcondition ---------------
  // make a new variable to stand for the value returned
  AbsValue *resultVal = NULL;
  if (!ft.retType->isVoid()) {
    resultVal = env.freshVariable("result",
                  stringc << "function call result: " << toString());

    // add this to the environment so if the programmer talks about
    // the result, it will state something about the result variable
    env.set(ft.result, resultVal);
  }

  if (ft.postcondition) {
    // evaluate it
    Predicate *predicate = ft.postcondition->expr->vcgenPred(env, 0);
    xassert(predicate);

    // assume it
    env.addFact(predicate, stringc << "postcondition from call: " << toString());
  }

  // result of calling this function is the result variable
  return resultVal;
}


AbsValue *E_fieldAcc::vcgen(AEnv &env, int path) const
{
  if (obj->isE_variable()) {
    Variable *var = obj->asE_variable()->var;
    if (!var->hasAddrTaken()) {
      // modeled as an unaliased tuple
      // retrieve named piece of the (whole) structure value
      return env.avGetElt(env.avInt(field->index), env.get(var));
    }
  }

  cout << "TODO: unhandled structure field access: " << toString();
  return env.freshVariable("field",
           stringc << "structure field access: " << toString());
}


AbsValue *E_sizeof::vcgen(AEnv &env, int path) const
{
  // whether 'expr' has side effects or not is irrelevant; they
  // won't be executed at run time; we only are about the type
  return env.grab(new AVint(size));
}

AbsValue *E_unary::vcgen(AEnv &env, int path) const
{
  return env.grab(new AVunary(op, expr->vcgen(env, path)));
}

AbsValue *E_effect::vcgen(AEnv &env, int path) const
{
  cout << "TODO: unhandled side effect: " << toString() << endl;
  return expr->vcgen(env, path);
}


AbsValue *E_binary::vcgen(AEnv &env, int path) const
{
  if (e2->numPaths == 0) {
    // simple side-effect-free binary op
    return env.grab(new AVbinary(e1->vcgen(env, path), op, e2->vcgen(env, 0)));
  }

  // consider operator
  if (op==BIN_AND || op==BIN_OR) {
    // evaluate LHS
    int modulus = e1->numPaths1();
    Predicate *lhs = e1->vcgenPred(env, path % modulus);
    path = path / modulus;

    // eval RHS *if* it's followed
    if (path > 0) {
      // we follow rhs; this implies something about the value of lhs:
      // if it's &&, lhs had to be true; false for ||
      env.addBoolFact(lhs, op==BIN_AND, 
        stringc << "LHS guard given RHS is eval'd " << toString());

      // the true/false value is now entirely determined by rhs
      return e2->vcgen(env, path-1);
    }
    else {
      // we do *not* follow rhs; this also implies something about lhs
      env.addBoolFact(lhs, op==BIN_OR,
        stringc << "LHS guard given RHS is *not* eval'd " << toString());

      // it's a C boolean, yielding 1 or 0 (and we know which)
      return env.grab(new AVint(op==BIN_OR? 1 : 0));
    }
  }

  else {
    // evaluate LHS
    int modulus = e1->numPaths1();
    AbsValue *lhs = e1->vcgen(env, path % modulus);
    path = path / modulus;

    // non-short-circuit: always evaluate RHS
    return env.grab(new AVbinary(lhs, op, e2->vcgen(env, path)));
  }
}


AbsValue *E_addrOf::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  if (expr->isE_variable()) {
    return env.avPointer(env.getMemVarAddr(expr->asE_variable()->var), // obj
                         new AVint(0));                                // offset
  }
  else {
    cout << "TODO: unhandled addrof: " << toString() << endl;
    return avTodo();
  }
}


void verifyPointerAccess(AEnv &env, Expression const *expr, AbsValue *addr)
{
  env.prove(new P_relation(env.avOffset(addr), RE_GREATEREQ,
                           env.avInt(0)),
                           stringc << "pointer lower bound: " << expr->toString());
  env.prove(new P_relation(env.avOffset(addr), RE_LESS,
                           env.avLength(env.avObject(addr))),
                           stringc << "pointer upper bound: " << expr->toString());
}

AbsValue *E_deref::vcgen(AEnv &env, int path) const
{
  AbsValue *addr = ptr->vcgen(env, path);

  // emit a proof obligation for safe access through 'addr'
  verifyPointerAccess(env, this, addr);

  return env.avSelect(env.getMem(), env.avObject(addr), env.avOffset(addr));
}


AbsValue *E_cast::vcgen(AEnv &env, int path) const
{
  // I don't know what account I should take of casts..
  return expr->vcgen(env, path);
}


AbsValue *E_cond::vcgen(AEnv &env, int path) const
{
  if (th->numPaths == 0 && el->numPaths == 0) {
    int modulus = cond->numPaths1();
    AbsValue *guard = cond->vcgen(env, path % modulus);
    path = path / modulus;

    // no side effects in either branch; use ?: operator
    return env.grab(new AVcond(guard, th->vcgen(env,0), el->vcgen(env,0)));
  }

  else {
    int modulus = cond->numPaths1();
    Predicate *guard = cond->vcgenPred(env, path % modulus);
    path = path / modulus;

    int thenPaths = th->numPaths1();
    int elsePaths = el->numPaths1();

    if (path < thenPaths) {
      // taking 'then' branch, so we can assume guard is true
      env.addFact(guard, stringc << "true guard of '?:': " << toString());
      return th->vcgen(env, path);
    }
    else {
      // taking 'else' branch, so guard is false
      env.addFalseFact(guard, stringc << "false guard of '?:': " << toString());
      return el->vcgen(env, path - elsePaths);
    }
  }
}


#if 0
AbsValue *E_gnuCond::vcgen(AEnv &env, int path) const
{
  // again on the assumption there are no side effects in the 'else' branch
  // TODO: make a deep-copier for astgen
  return avTodo();
}    
#endif // 0


AbsValue *E_comma::vcgen(AEnv &env, int path) const
{
  int modulus = e1->numPaths1();

  // evaluate and discard
  env.discard(e1->vcgen(env, path % modulus));

  // evaluate and keep
  return e2->vcgen(env, path / modulus);
}


AbsValue *E_sizeofType::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(size));
}


AbsValue *E_assign::vcgen(AEnv &env, int path) const
{
  int modulus = src->numPaths1();
  AbsValue *v = src->vcgen(env, path % modulus);
  path = path / modulus;

  if (target->isE_variable()) {
    xassert(path==0);
    Variable const *var = target->asE_variable()->var;

    if (op != BIN_ASSIGN) {
      // extract old value
      AbsValue *old = env.get(var);

      // combine it
      v = env.grab(new AVbinary(old, op, v));
    }

    // properly handles memvars vs regular vars
    env.updateVar(var, v);
  }
  else if (op == BIN_ASSIGN && target->isE_deref()) {
    string syntax = target->toString();

    // get an abstract value for the address being modified
    AbsValue *addr = target->asE_deref()->ptr->vcgen(env, path);

    // emit a proof obligation for safe access               
    verifyPointerAccess(env, this, addr);

    // memory changes
    env.setMem(env.avUpdate(env.getMem(), env.avObject(addr),
                            env.avOffset(addr), v));
  }

  // structure field access
  else if (target->isE_fieldAcc()) {
    E_fieldAcc *fldAcc = target->asE_fieldAcc();

    if (fldAcc->obj->isE_variable()) {
      Variable *var = fldAcc->obj->asE_variable()->var;

      if (var->type->isStructType() &&
          !var->hasAddrTaken()) {
        // modeled as an unaliased tuple

        // get current (whole) structure value
        AbsValue *current = env.get(var);

        // compute updated structure value
        AbsValue *updated =
          env.avSetElt(
            env.avInt(fldAcc->field->index),      // field index
            current,                              // value being updated
            v);                                   // new field value

        // replace old with new in abstract environment
        env.set(var, updated);

        return env.dup(v);
      }
    }

    // other forms aren't handled yet
    cout << "TODO: unhandled structure field assignment: " << toString() << endl;
  }

  else {
    cout << "TODO: unhandled assignment: " << toString() << endl;
  }

  return env.dup(v);
}


AbsValue *E_forall::vcgen(AEnv &env, int path) const
{                                  
  // this is quite nonideal, but at the moment I don't know exactly
  // what syntactic rule to use, so I just wait until the problem
  // shows up, and then bomb
  xfailure("can't use forall in a non-obviously-boolean context");
  return NULL;   // silence warning
}


// --------------- Expression::vcgenPred -------------------
Predicate *Expression::vcgenPredDefault(AEnv &env, int path) const
{
  // use exprToPred to give this another chance to map into the
  // predicate space; the vcgenPred functions below do a good job
  // when it's obvious something is being used in a predicate context,
  // but if I save a value into a variable and *then* try to treat
  // it as a predicate, I won't learn until later that I need to
  // map it into predicate space
  return exprToPred(vcgen(env, path));
}


Predicate *E_intLit::vcgenPred(AEnv &env, int path) const
{
  xassert(path==0);

  // when a literal integer is interpreted as a predicate,
  // I can map it immediately to true/false
  return new P_lit(i != 0);
}


Predicate *E_floatLit::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }

Predicate *E_stringLit::vcgenPred(AEnv &env, int path) const
{
  return new P_lit(true);
}

Predicate *E_charLit::vcgenPred(AEnv &env, int path) const
{
  return new P_lit(c != 0);
}

Predicate *E_variable::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }
Predicate *E_funCall::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }
Predicate *E_fieldAcc::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }
Predicate *E_sizeof::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }

Predicate *E_unary::vcgenPred(AEnv &env, int path) const
{
  if (op == UNY_NOT) {
    return new P_not(expr->vcgenPred(env, path));
  }
  else {
    return vcgenPredDefault(env, path);
  }
}

Predicate *E_effect::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }


Predicate *E_binary::vcgenPred(AEnv &env, int path) const
{
  int modulus = e1->numPaths1();
  int lhsPath = path % modulus;
  int rhsPath = path / modulus;

  if (isPredicateCombinator(op)) {
    if (e2->numPaths == 0) {
      xassert(rhsPath == 0);

      // simple side-effect-free combinator, so there's no path stuff;
      // *but* we do have to assume something about the LHS while
      // analyzing the RHS, e.g. for "if (p && p->foo) ..."
      Predicate *lhs = e1->vcgenPred(env, lhsPath);

      Predicate *rhs;
      if (op != BIN_OR) {
        env.pushFact(lhs);
        rhs = e2->vcgenPred(env, 0);
        env.popFact();
      }
      else {
        // for "or", assume the negation
        P_not notLHS(lhs);
        try {
          env.pushFact(&notLHS);
          rhs = e2->vcgenPred(env, 0);
          env.popFact();
        }
        catch (...) {
          notLHS.p = NULL;
          throw;
        }
        notLHS.p = NULL;
      }

      return P_combinator(op, lhs, rhs);
    }

    else {
      // RHS has a side effect, so we need to analyze paths
      xassert(op != BIN_IMPLIES);     // we don't allow ==> in side-effecting exprs

      // evaluate LHS
      Predicate *lhs = e1->vcgenPred(env, lhsPath);

      // eval RHS *if* it's followed
      if (rhsPath > 0) {
        // we follow rhs; this implies something about the value of lhs:
        // if it's &&, lhs had to be true; false for ||
        env.addBoolFact(lhs, op==BIN_AND,
          stringc << "LHS guard given RHS is eval'd " << toString());

        // the true/false value is now entirely determined by rhs
        return e2->vcgenPred(env, rhsPath-1);
      }
      else {
        // we do *not* follow rhs; this also implies something about lhs
        env.addBoolFact(lhs, op==BIN_OR,
          stringc << "LHS guard given RHS is *not* eval'd " << toString());

        // it's a C boolean, yielding 1 or 0 (and we know which)
        return new P_lit(op==BIN_OR? true : false);
      }
    }
  }

  else if (isRelational(op)) {
    // non-short-circuit: always evaluate RHS
    AbsValue *lhs = e1->vcgen(env, lhsPath);
    AbsValue *rhs = e2->vcgen(env, rhsPath);
    return new P_relation(lhs, binOpToRelation(op), rhs);
  }

  else {
    return vcgenPredDefault(env, path);
  }
}


Predicate *E_addrOf::vcgenPred(AEnv &env, int path) const
{ 
  return new P_lit(true);
}

Predicate *E_deref::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }

Predicate *E_cast::vcgenPred(AEnv &env, int path) const
{
  // assume casts preserve truth or falsity (I think that's sound..)
  return expr->vcgenPred(env, path);
}

Predicate *E_cond::vcgenPred(AEnv &env, int path) const
{
  // condition
  int modulus = cond->numPaths1();
  Predicate *guard = cond->vcgenPred(env, path % modulus);
  path = path / modulus;

  if (th->numPaths == 0 && el->numPaths == 0) {
    // no side effects in either branch;
    // map ?: as a pair of implications
    return P_and2(new P_impl(cond->vcgenPred(env, 0),
                             th->vcgenPred(env, 0)),
                  new P_impl(new P_not(cond->vcgenPred(env, 0)),
                             el->vcgenPred(env, 0)));
  }

  else {
    int thenPaths = th->numPaths1();
    int elsePaths = el->numPaths1();

    if (path < thenPaths) {
      // taking 'then' branch, so we can assume guard is true
      env.addFact(guard, stringc << "true guard of '?:': " << toString());
      return th->vcgenPred(env, path);
    }
    else {
      // taking 'else' branch, so guard is false
      env.addFalseFact(guard, stringc << "false guard of '?:': " << toString());
      return el->vcgenPred(env, path - elsePaths);
    }
  }
}

Predicate *E_comma::vcgenPred(AEnv &env, int path) const
{ 
  int modulus = e1->numPaths1();
  
  // evaluate and discard
  env.discard(e1->vcgen(env, path % modulus));

  // evaluate and keep
  return e2->vcgenPred(env, path / modulus);
}


Predicate *E_sizeofType::vcgenPred(AEnv &env, int path) const
{ 
  xassert(path==0);
  return new P_lit(size != 0);     // should almost always be true
}

Predicate *E_assign::vcgenPred(AEnv &env, int path) const
{
  // full analysis in here would be a major pain, and suggests whether
  // my vcgen vs vcgenPred separation is really the right one.. but
  // for now I can punt and just keep using exprToPred to map the
  // term that results from analyzing this thing
  return exprToPred(this->vcgen(env, path));
}


Predicate *E_forall::vcgenPred(AEnv &env, int path) const
{
  // analyze the body; this will cause all the quantified
  // variables to be instantiaged in AEnv::get
  Predicate *body = pred->vcgenPred(env, path);
                                           
  // gather up the quantifiers
  P_forall *ret = new P_forall(NULL, body);

  FOREACH_ASTLIST(Declaration, decls, outer) {
    FOREACH_ASTLIST(Declarator, outer.data()->decllist, inner) {
      Variable *var = inner.data()->var;

      ret->variables.append(env.get(var)->asAVvar());
    }
  }

  return ret;
}


// --------------------- Initializer --------------------
AbsValue *IN_expr::vcgen(AEnv &env, Type const *, int path) const
{
  return e->vcgen(env, path);
}


AbsValue *IN_compound::vcgen(AEnv &/*env*/, Type const */*type*/, int /*path*/) const
{
  // I have no representation for array and structure values
  return avTodo();
}


