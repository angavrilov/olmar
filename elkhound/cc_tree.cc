// cc_tree.cc
// code for cc_tree.h

#include "cc_tree.h"     // this module


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
