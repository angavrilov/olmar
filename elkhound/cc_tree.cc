// cc_tree.cc
// code for cc_tree.h

#include "cc_tree.h"     // this module
#include "dataflow.h"    // DataflowEnv, etc.
#include "cc.h"          // for simplify()


// --------------------- CCTreeNode ----------------------
CCTreeNode::~CCTreeNode()
{}


void CCTreeNode::declareVariable(Env *env, char const *name,
                                 DeclFlags flags, Type const *type) const
{
  if (!env->declareVariable(name, flags, type)) {
    SemanticError err(this, SE_DUPLICATE_VAR_DECL);
    err.varName = name;
    env->report(err);
  }
}


void CCTreeNode::throwError(char const *msg) const
{
  SemanticError err(this, SE_GENERAL);
  err.msg = msg;
  THROW(XSemanticError(err));
}


void CCTreeNode::reportError(Env *env, char const *msg) const
{
  SemanticError err(this, SE_GENERAL);
  err.msg = msg;
  env->report(err);
}


void CCTreeNode::internalError(char const *msg) const
{
  SemanticError err(this, SE_INTERNAL_ERROR);
  err.msg = msg;
  THROW(XSemanticError(err));
}


CilExpr *CCTreeNode::disambiguate(Env *passedEnv, CilInstructions &inst, 
                                  DisambFn func)
{
  // not perfectly ideal to be doing this here, it seems, but
  // I'm basically sprinkling these all over at this point and
  // don't have a clear criteria for deciding where to and where
  // not to ..
  env = passedEnv;

  // if it's not ambiguous, or we've already disambiguated,
  // go straight to the real deal
  if (reductions.count() == 1) {
    return (this->*func)(env, inst);
  }

  // pull all the competing reductions out into a private list
  ObjList<Reduction> myReds;
  myReds.concat(reductions);

  // need place to put the ones that are finished
  ObjList<Reduction> okReds;
  ObjList<Reduction> badReds;

  // one at a time, put them back, and attempt to type-check them
  while (myReds.isNotEmpty()) {
    // put it back
    Reduction *red = myReds.removeAt(0);
    reductions.append(red);

    // attempt type-check in a new environment so it can't
    // corrupt the main one we're working on
    Env newEnv(passedEnv);
    newEnv.setTrialBalloon(true);
    CilInstructions newInst;
    try {
      (this->*func)(&newEnv, newInst);
    }
    catch (XSemanticError &x) {
      newEnv.report(x.err);
    }

    // remove the reduction from the main node
    reductions.removeAt(0);

    // did that work?
    if (newEnv.numLocalErrors() == 0) {
      // yes
      okReds.append(red);
    }
    else {
      // no
      badReds.append(red);

      // throw away the errors so they don't get
      // moved into the original environment
      newEnv.forgetLocalErrors();
    }
  }

  // put all the good ones back
  reductions.concat(okReds);

  // see what the verdict is
  if (reductions.count() == 1) {
    // successfully disambiguated
    trace("disamb") << "disambiguated " << getLHS()->name
                    << " at " << locString() << endl;
  }
  else if (reductions.count() > 1) {
    // more than one worked..
    THROW(XAmbiguity(this, "multiple trees passed semantic checks"));
  }
  else {
    // none of them worked.. let's arbitrarily pick the first and
    // throw away the rest, and report the errors from that one
    reductions.append(badReds.removeAt(0));
  }

  // throw away the bad ones
  badReds.deleteAll();

  // now, we have exactly one reduction -- typecheck it
  // in the current environment
  xassert(reductions.count() == 1);
  return (this->*func)(passedEnv, inst);
}


#if 0    // unfinished..
void CCTreeNode::simplify()
{
  if (reductions.count() != 1) {
    cout << locString() << ": not simplifying because it's ambiguous\n";
    return;
  }
  Reduction *red = first();

  // simplify sizeof
  if (getLHS()->name.equals("UnaryExpression") &&
      // size of expression or size of type
      (red->children.count() == 2 || red->children.count() == 4) &&
      red->children.first()->isTerm() &&
      red->children.first()->asTerm().token->type == L2_SIZEOF) {
    // see if we know its value
    UnaryExpression_Node *ue = (UnaryExpression_Node*)this;
    try {
      int sz = ue->intEval(env);

      // it didn't throw -- we know the value; synthesize a
      // simpler tree here in place of the existing 'sizeof' expr
      reductions.empty();

      red = new Reduction(  /* oops! */
#endif // 0


// ---------------------- analysis routines --------------------
void printVar(DataflowVar const *var)
{
  cout << "  " << var->getName()
       << " : "<< var->getType()->toString()
       << ", fv=" << var->value.toString() << endl;
}

bool isOwnerPointer(Type const *t)
{
  if (t->isPointerType()) {
    PointerType const &pt = t->asPointerTypeC();
    return pt.op == PO_POINTER &&
           (pt.cv & CV_OWNER) != 0;
  }
  else {
    return false;
  }
}


bool isArrayOfOwnerPointer(Type const *t)
{
  return t->isArrayType() &&
         isOwnerPointer(t->asArrayTypeC().eltType);
}


DataflowVar *CCTreeNode::getOwnerPtr(char const *name)
{
  DataflowVar *var = env->getDenv().getVariable(name);
  if (var && isOwnerPointer(var->getType())) {
    return var;
  }
  else {
    return NULL;
  }
}


void CCTreeNode::ana_free(string name)
{
  cout << locString() << ": free(" << name << ")\n";

  // get variable
  DataflowVar *var = getOwnerPtr(name);
  if (!var) {
    cout << "  ERROR: can only free owner pointers\n";
    return;
  }

  printVar(var);

  // check dataflow
  if (! var->value .geq( AOV_INIT ) ) {
    cout << "  ERROR: can only free inited owner pointers\n";
    //return;    // compute resulting flow value anyway
  }

  // flow dataflow
  var->value = AOV_UNINIT;
}


void CCTreeNode::ana_malloc(string name)
{
  cout << locString() << ": " << name << " = malloc()\n";

  // get variable
  DataflowVar *var = getOwnerPtr(name);
  if (!var) {
    cout << "  ERROR: can only assign malloc to owner pointers\n";
    return;
  }

  printVar(var);

  // check dataflow
  if (! var->value .geq( AOV_UNINIT ) ) {
    cout << "  ERROR: can only assign malloc to uninited owner pointers\n";
    return;
  }

  // flow dataflow
  var->value = AOV_INITQ;
}


void CCTreeNode::ana_endScope(Env *localEnv)
{
  StringObjDict<Variable>::Iter iter(localEnv->getVariables());
  for(; !iter.isDone(); iter.next()) {
    // look in *localEnv*, not env
    DataflowVar *var = localEnv->getDenv().getVariable(iter.key());
    if (!var) { continue; }

    cout << locString() << ", end of scope,";
    printVar(var);

    if (!( var->value.geq(AOV_UNINIT) )) {
      cout << "  ERROR: `" << var->getName()
           << "': owners must die uninited\n";
    }
  }
}


void CCTreeNode::ana_applyConstraint(bool negated)
{
  // for now, I only consider "if(p)"
  if (numGroundTerms() == 1) {
    string varName = unparseString();

    DataflowVar *var = getOwnerPtr(varName);
    if (var) {
      cout << locString() << ", applying constraint: "
           << unparseString() << endl;
      printVar(var);
    
      // apply p==0 or p!=0 (depending on 'negated') to
      // var->value

      // figure out the abstraction of the set of values
      // that could allow control flow to pass into the
      // guarded branch
      AbsOwnerValue branchReqt;
      if (!negated) {
        // e.g. 'then' branch of "if(p)"
        branchReqt = AOV_NOT_NULL;
      }
      else {
        // e.g. 'else' branch of "if(p)"
        branchReqt = AOV_NULL;
      }

      // now intersect this with the set of values we do
      // in fact have
      var->value = var->value .join( branchReqt );
    }
  }
}


void CCTreeNode::ana_checkDeref()
{
  if (numGroundTerms() == 1) {
    string varName = unparseString();

    // only look at owner pointers
    DataflowVar *var = getOwnerPtr(varName);
    if (var) {
      cout << locString() << ", checking deref:\n";
      printVar(var);

      if (var->value != AOV_INIT) {
        cout << "  ERROR: can only deref inited owners\n";
      }
    }
  }
}
