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

// lattice of values
//
//          top: not seen any value = {}                     .
//              /   /     \                                  .
//             /   /       \                                 .
//            /   /         \                    ^           .
//           /  null        init                 |           .
//          /  /    \       /                    | more      .
//         /  /      \     /                     | precise   .
//        /  /        \   /                      | info      .
//       uninit        init? = init U null       |           .
//           \          /                                    .
//            \        /                                     .
//        bottom: could be anything                          .
//          uninit U init U null                             .

enum FlowValue {
  FV_TOP   =0,          // initial value of Variable::value
  FV_NULL  =1,
  FV_INIT  =2,
  FV_UNINIT=5,          // uninit; can be NULL
  FV_INITQ =3,          // FV_NULL | FV_INIT
  FV_BOTTOM=7,          // FV_INIT | FV_UNINIT
};

// typesafe accessor
FlowValue &fv(Variable *v)
{
  return (FlowValue&)(v->value);
}

FlowValue fvC(Variable const *v)
{
  return (FlowValue)(v->value);
}


char const *fv_name(FlowValue v)
{
  switch (v) {
    default: xfailure("bad fv code");
    #define N(val) case val: return #val;
    N(FV_TOP)
    N(FV_NULL)
    N(FV_INIT)
    N(FV_UNINIT)
    N(FV_INITQ)
    N(FV_BOTTOM)
    #undef N
  }
}


// is v1 >= v2?
// i.e., is there a path from v1 down to v2 in the lattice?
// e.g.:
//   top >= init
//   null >= init?
//   init >= init
// but NOT:
//   init >= uninit
bool fv_geq(FlowValue v1, FlowValue v2)
{
  return (v1 & v2) == v1;
}


void printVar(char const *name, Variable const *var)
{
  cout << "  " << name << " : " << var->type->toString()
       << ", fv=" << fv_name(fvC(var)) << endl;
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
  cout << locString() << ", free(" << name << ")\n";

  // get variable
  Variable *var = env->getVariable(name);
  printVar(name, var);

  // check restrictions
  if (!isOwnerPointer(var->type)) {
    cout << "  ERROR: can only free owner pointers\n";
    return;
  }

  // check dataflow
  if (! fv_geq(fv(var), FV_INIT) ) {
    cout << "  ERROR: can only free inited owner pointers\n";
    return;
  }

  // flow dataflow
  fv(var) = FV_UNINIT;
}


void CCTreeNode::ana_malloc(string name)
{
  cout << locString() << ", " << name << " = malloc()\n";

  // get variable
  Variable *var = env->getVariable(name);
  printVar(name, var);

  // check restrictions
  if (!isOwnerPointer(var->type)) {
    cout << "  ERROR: can only assign malloc to owner pointers\n";
    return;
  }

  // check dataflow
  if (! fv_geq(fv(var), FV_UNINIT) ) {
    cout << "  ERROR: can only free uninited owner pointers\n";
    return;
  }

  // flow dataflow
  fv(var) = FV_INITQ;
}


void CCTreeNode::ana_endScope(Env *localEnv)
{
  StringObjDict<Variable>::Iter iter(localEnv->getVariables());
  for(; !iter.isDone(); iter.next()) {
    cout << locString() << ", end of scope, ";
    printVar(iter.key(), iter.value());

    if (isOwnerPointer(iter.value()->type)) {
      if (! fv_geq(fvC(iter.value()), FV_UNINIT) ) {
        cout << "  ERROR: `" << iter.key() 
             << "': owners must die uninited\n";
      }
    }
  }
}
