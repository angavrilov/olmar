// cc_tree.cc
// code for cc_tree.h

#include "cc_tree.h"     // this module
#include "dataflow.h"    // DataflowEnv, etc.


// --------------------- CCTreeNode ----------------------
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


void CCTreeNode::disambiguate(Env *passedEnv, DisambFn func)
{
  // not perfectly ideal to be doing this here, it seems, but
  // I'm basically sprinkling these all over at this point and
  // don't have a clear criteria for deciding where to and where
  // not to ..
  env = passedEnv;

  // if it's not ambiguous, or we've already disambiguated,
  // go straight to the real deal
  if (reductions.count() == 1) {
    (this->*func)(env);
    return;
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
    try {
      (this->*func)(&newEnv);
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
  (this->*func)(passedEnv);
}


// ---------------------- analysis routines --------------------
void printVar(DataflowVar const *var)
{
  cout << "  " << var->getName()
       << " : "<< var->getType()->toString()
       << ", fv=" << fv_name(var->value) << endl;
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


void CCTreeNode::ana_free(string name)
{
  cout << locString() << ": free(" << name << ")\n";

  // get variable
  DataflowVar *var = env->getDenv().getVariable(env, name);
  printVar(var);

  // check restrictions
  if (!isOwnerPointer(var->getType())) {
    cout << "  ERROR: can only free owner pointers\n";
    return;
  }

  // check dataflow
  if (!fv_geq(var->value, FV_INIT)) {
    cout << "  ERROR: can only free inited owner pointers\n";
    //return;    // compute resulting flow value anyway
  }

  // flow dataflow
  var->value = FV_UNINIT;
}


void CCTreeNode::ana_malloc(string name)
{
  cout << locString() << ": " << name << " = malloc()\n";

  // get variable
  DataflowVar *var = env->getDenv().getVariable(env, name);
  printVar(var);

  // check restrictions
  if (!isOwnerPointer(var->getType())) {
    cout << "  ERROR: can only assign malloc to owner pointers\n";
    return;
  }

  // check dataflow
  if (!fv_geq(var->value, FV_UNINIT) ) {
    cout << "  ERROR: can only assign malloc to uninited owner pointers\n";
    return;
  }

  // flow dataflow
  var->value = FV_INITQ;
}


void CCTreeNode::ana_endScope(Env *localEnv)
{
  StringObjDict<Variable>::Iter iter(localEnv->getVariables());
  for(; !iter.isDone(); iter.next()) {
    cout << locString() << ", end of scope,";

    DataflowVar *var = localEnv->getDenv().getVariable(localEnv, iter.key());
    printVar(var);

    if (isOwnerPointer(var->getType())) {
      if (!fv_geq(var->value, FV_UNINIT)) {
        cout << "  ERROR: `" << var->getName()
             << "': owners must die uninited\n";
      }
    }
  }
}
