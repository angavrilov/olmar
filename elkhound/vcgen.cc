// vcgen.cc
// vcgen methods on the C abstract syntax tree

#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // abstract domain values
#include "aenv.h"               // AEnv
#include "cc_type.h"            // FunctionType, etc.
#include "sobjlist.h"           // SObjList
#include "trace.h"              // tracingSys


// ------------------- TranslationUnit ---------------------
void TranslationUnit::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(TopForm, topForms, iter) {
    iter.data()->vcgen(env);
  }
}


// --------------------- TopForm ----------------------
void TF_decl::vcgen(AEnv &env) const
{}


void TF_func::vcgen(AEnv &env) const
{
  FunctionType const &ft = *(ftype());

  // clear anything left over in env
  env.clear();

  // add the function parameters to the environment
  FOREACH_OBJLIST(FunctionType::Param, ft.params, iter) {
    FunctionType::Param const *p = iter.data();

    if (p->name && p->type->isIntegerType()) {
      // the function parameter's initial value is represented
      // by a logic variable of the same name
      env.set(p->name, new IVvar(p->name,
        stringc << "initial value of parameter " << p->name));
    }
  }

  // add the precondition as an assumption
  if (ft.precondition) {
    env.addFact(ft.precondition->vcgen(env));
  }

  // add 'result' to the environment so we can record what value
  // is actually returned
  if (!ft.retType->isVoid()) {
    env.set(env.str("result"), env.freshIntVariable("return value"));
  }

  // now interpret the function body
  body->vcgen(env);

  // print the results
  if (tracingSys("absInterp")) {
    cout << "interpretation after " << name() << ":\n";
    env.print();
  }

  // prove the postcondition now holds
  // ASSUMPTION: none of the parameters have been changed
  if (ft.postcondition) {
    env.prove(ft.postcondition->vcgen(env));
  }
}


// --------------------- Declaration --------------------
void Declaration::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(Declarator, decllist, iter) {
    iter.data()->vcgen(env);
  }
}

void Declarator::vcgen(AEnv &env) const
{
  if (name && type->isIntegerType()) {
    IntValue *value;
    if (init) {
      // evaluate 'init' to an abstract value; the initializer
      // will need to know which type it's constructing
      value = init->vcgen(env, type);
    }
    else {
      // make up a new name for the uninitialized value
      // (if it's global we get to assume it's 0... not implemented..)
      value = new IVvar(name, stringc << "UNINITialized value of var " << name);
    }

    env.set(name, value);
  }
}


// ----------------------- Statement ---------------
// for now, ignore all control flow

void S_skip::vcgen(AEnv &env) const {}
void S_label::vcgen(AEnv &env) const {}
void S_case::vcgen(AEnv &env) const {}
void S_caseRange::vcgen(AEnv &env) const {}
void S_default::vcgen(AEnv &env) const {}

void S_expr::vcgen(AEnv &env) const
{        
  // evaluate and discard
  env.discard(expr->vcgen(env));
}

void S_compound::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(Statement, stmts, iter) {
    iter.data()->vcgen(env);
  }
}

void S_if::vcgen(AEnv &env) const {}
void S_switch::vcgen(AEnv &env) const {}
void S_while::vcgen(AEnv &env) const {}
void S_doWhile::vcgen(AEnv &env) const {}
void S_for::vcgen(AEnv &env) const {}
void S_break::vcgen(AEnv &env) const {}
void S_continue::vcgen(AEnv &env) const {}

void S_return::vcgen(AEnv &env) const
{
  if (expr) {
    // model as an assignment to 'result' (and leave the
    // control flow implications for later)
    env.set(env.str("result"), expr->vcgen(env));
  }
}

void S_goto::vcgen(AEnv &env) const {}


void S_decl::vcgen(AEnv &env) const 
{
  decl->vcgen(env);
}


void S_assert::vcgen(AEnv &env) const
{                                
  // map the expression to my abstract domain
  IntValue *v = expr->vcgen(env);            
  xassert(v);     // already checked it was boolean

  // try to prove it (is not equal to 0)
  env.prove(v);
}

void S_assume::vcgen(AEnv &env) const
{
  // evaluate
  IntValue *v = expr->vcgen(env);            
  xassert(v); 
  
  // remember it as a known fact
  env.addFact(v);
}

void S_invariant::vcgen(AEnv &env) const
{
  // ignore for now
}


// ---------------------- Expression -----------------
IntValue *E_intLit::vcgen(AEnv &env) const
{
  return env.grab(new IVint(i));
}

IntValue *E_floatLit::vcgen(AEnv &env) const { return NULL; }
IntValue *E_stringLit::vcgen(AEnv &env) const { return NULL; }

IntValue *E_charLit::vcgen(AEnv &env) const
{
  return env.grab(new IVint(c));
}

IntValue *E_structLit::vcgen(AEnv &env) const { return NULL; }

IntValue *E_variable::vcgen(AEnv &env) const
{
  if (type->isIntegerType()) {
    // look up the current abstract value for this variable, and
    // simply return that
    return env.get(name);
  }
  else {
    return NULL;
  }
}


IntValue *E_arrayAcc::vcgen(AEnv &env) const
{
  // having a good answer here would require having an abstract value
  // to tell me about the structure of the array
  if (type->isIntegerType()) {
    return env.freshIntVariable(stringc
             << "array access: " << toString());
  }
  else {
    return NULL;
  }
}


IntValue *E_funCall::vcgen(AEnv &env) const
{
  // evaluate the argument expressions
  SObjList<IntValue> argExps;
  FOREACH_ASTLIST(Expression, args, iter) {
    // vcgen might return NULL, that's ok for an SObjList
    argExps.prepend(iter.data()->vcgen(env));
  }
  argExps.reverse();      // when it's easy to stay linear..

  FunctionType const &ft = func->type->asFunctionTypeC();

  // -------------- build an environment for pre/post ----------------
  // the pre- and postconditions get evaluated to an abstract value in
  // an environment containing only parameters (and in the case of the
  // postcondition, 'result); eventually it should also contain
  // globals, but right now I don't have globals in the abstract
  // environment..
  AEnv newEnv(env.stringTable);

  // bind the parameters in the new environment
  SObjListMutator<IntValue> argExpIter(argExps);
  FOREACH_OBJLIST(FunctionType::Param, ft.params, paramIter) {
    // bind the names to the expressions that are passed; only
    // bind if the expression is not NULL
    if (argExpIter.data()) {
      newEnv.set(paramIter.data()->name,
                 argExpIter.data());
    }

    // I claim that putting the arg exp into the new environment
    // counts as "using" it, so discarding it is not necessary
    // (except that it was created in 'env' and passed to 'newEnv'...)

    argExpIter.adv();
  }

  // ----------------- prove precondition ---------------
  if (ft.precondition) {
    // finally, interpret the precondition in the parameter-only
    // environment, to arrive at a predicate to prove
    IntValue *predicate = ft.precondition->vcgen(newEnv);
    xassert(predicate);      // tcheck should have verified we can represent it

    // prove this predicate; the assumptions from the *old* environment
    // are relevant
    env.prove(predicate);

    // no longer needed
    newEnv.discard(predicate);
  }

  // ----------------- assume postcondition ---------------
  // make a new variable to stand for the value returned
  IntValue *result = NULL;
  if (type->isIntegerType()) {
    result = env.freshIntVariable(stringc
               << "function call result: " << toString());

    // add this to the mini environment so if the programmer talks
    // about the result, it will state something about the result variable
    newEnv.set(env.stringTable.add("result"), result);
  }

  // NOTE: by keeping the environment from before, we are interpreting
  // all references to parameters as their values in the *pre* state

  if (ft.postcondition) {
    // evaluate it
    IntValue *predicate = ft.postcondition->vcgen(newEnv);
    xassert(predicate);
    
    // assume it
    env.addFact(predicate);
  }

  // result of calling this function is the result variable
  return result;
}


IntValue *E_fieldAcc::vcgen(AEnv &env) const
{
  // good results here would require abstract values for structures
  if (type->isIntegerType()) {
    return env.freshIntVariable(stringc
             << "structure field access: " << toString());
  }
  else {
    return NULL;
  }
}


IntValue *E_unary::vcgen(AEnv &env) const
{
  // TODO: deal with ++, --
  
  if (op == UNY_SIZEOF) {
    return env.grab(new IVint(expr->type->reprSize()));
  }
  else {
    return env.grab(new IVunary(op, expr->vcgen(env)));
  }
}

IntValue *E_binary::vcgen(AEnv &env) const
{
  // TODO: deal with &&, ||
  return env.grab(new IVbinary(e1->vcgen(env), op, e2->vcgen(env)));
}


IntValue *E_addrOf::vcgen(AEnv &env) const
{
  // result is always of pointer type
  return NULL;
}


IntValue *E_deref::vcgen(AEnv &env) const
{
  // abstract knowledge about memory contents, possibly aided by
  // detailed knowledge of what this pointer points at, could yield
  // something more specific here
  if (type->isIntegerType()) {
    return env.freshIntVariable(stringc
             << "pointer read: " << toString());
  }
  else {
    return NULL;
  }
}


IntValue *E_cast::vcgen(AEnv &env) const
{
  // I can sustain integer->integer casts..
  if (type->isIntegerType() && expr->type->isIntegerType()) {
    return expr->vcgen(env);
  }
  else {
    return NULL;
  }
}


IntValue *E_cond::vcgen(AEnv &env) const
{
  // for now I'll assume there are no side effects in the branches;
  // same assumption applies to '&&' and '||' above
  return env.grab(new IVcond(cond->vcgen(env), th->vcgen(env), el->vcgen(env)));
}


IntValue *E_gnuCond::vcgen(AEnv &env) const
{
  // again on the assumption there are no side effects in the 'else' branch
  // TODO: make a deep-copier for astgen
  return NULL;
}


IntValue *E_comma::vcgen(AEnv &env) const
{
  // evaluate and discard
  env.discard(e1->vcgen(env));

  // evaluate and keep
  return e2->vcgen(env);
}


IntValue *E_sizeofType::vcgen(AEnv &env) const
{
  return env.grab(new IVint(size));
}


IntValue *E_assign::vcgen(AEnv &env) const
{
  IntValue *v = src->vcgen(env);

  // since I have no reasonable hold on pointers yet, I'm only
  // going to keep track of assignments to (integer) variables
  if (target->isE_variable() && type->isIntegerType()) {
    // this removes the old mapping
    env.set(target->asE_variable()->name, v);
  }

  return env.dup(v);
}


IntValue *E_arithAssign::vcgen(AEnv &env) const
{
  IntValue *v = src->vcgen(env);

  // again, only support variables
  if (target->isE_variable()) {
    StringRef name = target->asE_variable()->name;

    // extract old value
    IntValue *old = env.get(name);
    
    // combine it
    v = env.grab(new IVbinary(old, op, v));
    
    // replace in environment
    env.set(name, v);

    return env.dup(v);
  }
  else {
    return v;
  }
}


// --------------------- Initializer --------------------
IntValue *IN_expr::vcgen(AEnv &env, Type const *) const
{
  return e->vcgen(env);
}


IntValue *IN_compound::vcgen(AEnv &/*env*/, Type const */*type*/) const
{
  // I have no representation for array and structure values
  return NULL;
}


