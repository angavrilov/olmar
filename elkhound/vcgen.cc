// vcgen.cc
// vcgen methods on the C abstract syntax tree

#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // abstract domain values
#include "aenv.h"               // AEnv
#include "cc_type.h"            // FunctionType, etc.
#include "sobjlist.h"           // SObjList


// ------------------- TranslationUnit ---------------------
void TranslationUnit::vcgen(AEnv &env) VCGEN_CONST
{
  FOREACH_ASTLIST(TopForm, topForms, iter) {
    iter.data()->vcgen(env);
  }
}


// --------------------- TopForm ----------------------
void TF_decl::vcgen(AEnv &env) VCGEN_CONST
{}


void TF_func::vcgen(AEnv &env) VCGEN_CONST
{
  // clear anything left over in env
  env.clear();

  // add the function parameters to the environment
  FOREACH_OBJLIST(FunctionType::Param, ftype()->params, iter) {
    FunctionType::Param const *p = iter.data();

    if (p->name && p->type->isIntegerType()) {
      // the function parameter's initial value is represented
      // by a logic variable of the same name
      env.set(p->name, new IVvar(p->name,
        stringc << "initial value of parameter " << p->name));
    }
  }

  // now interpret the function body
  body->vcgen(env);

  // print the results
  cout << "interpretation after " << name() << ":\n";
  env.print();
}


// --------------------- Declaration --------------------
void Declaration::vcgen(AEnv &env) VCGEN_CONST
{
  FOREACH_ASTLIST(Declarator, decllist, iter) {
    iter.data()->vcgen(env);
  }
}

void Declarator::vcgen(AEnv &env) VCGEN_CONST
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

void S_skip::vcgen(AEnv &env) VCGEN_CONST {}
void S_label::vcgen(AEnv &env) VCGEN_CONST {}
void S_case::vcgen(AEnv &env) VCGEN_CONST {}
void S_caseRange::vcgen(AEnv &env) VCGEN_CONST {}
void S_default::vcgen(AEnv &env) VCGEN_CONST {}

void S_expr::vcgen(AEnv &env) VCGEN_CONST
{        
  // evaluate and discard
  env.discard(expr->vcgen(env));
}

void S_compound::vcgen(AEnv &env) VCGEN_CONST
{
  FOREACH_ASTLIST(Statement, stmts, iter) {
    iter.data()->vcgen(env);
  }
}

void S_if::vcgen(AEnv &env) VCGEN_CONST {}
void S_switch::vcgen(AEnv &env) VCGEN_CONST {}
void S_while::vcgen(AEnv &env) VCGEN_CONST {}
void S_doWhile::vcgen(AEnv &env) VCGEN_CONST {}
void S_for::vcgen(AEnv &env) VCGEN_CONST {}
void S_break::vcgen(AEnv &env) VCGEN_CONST {}
void S_continue::vcgen(AEnv &env) VCGEN_CONST {}
void S_return::vcgen(AEnv &env) VCGEN_CONST {}
void S_goto::vcgen(AEnv &env) VCGEN_CONST {}


void S_decl::vcgen(AEnv &env) VCGEN_CONST 
{
  decl->vcgen(env);
}


void S_assert::vcgen(AEnv &env) VCGEN_CONST
{                                
  // map the expression to my abstract domain
  IntValue *v = expr->vcgen(env);            
  xassert(v);     // already checked it was boolean

  // try to prove it (is not equal to 0)
  env.prove(v);
}


// ---------------------- Expression -----------------
IntValue *E_intLit::vcgen(AEnv &env) VCGEN_CONST
{
  return env.grab(new IVint(i));
}

IntValue *E_floatLit::vcgen(AEnv &env) VCGEN_CONST { return NULL; }
IntValue *E_stringLit::vcgen(AEnv &env) VCGEN_CONST { return NULL; }

IntValue *E_charLit::vcgen(AEnv &env) VCGEN_CONST
{
  return env.grab(new IVint(c));
}

IntValue *E_structLit::vcgen(AEnv &env) VCGEN_CONST { return NULL; }

IntValue *E_variable::vcgen(AEnv &env) VCGEN_CONST
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


IntValue *E_arrayAcc::vcgen(AEnv &env) VCGEN_CONST
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


IntValue *E_funCall::vcgen(AEnv &env) VCGEN_CONST
{
  // evaluate the argument expressions
  SObjList<IntValue> argExps;
  FOREACH_ASTLIST(Expression, args, iter) {
    // vcgen might return NULL, that's ok for an SObjList
    argExps.prepend(iter.data()->vcgen(env));
  }
  argExps.reverse();      // when it's easy to stay linear..

  // if the function has a precondition, check it
  FunctionType const &ft = func->type->asFunctionTypeC();

  // the precondition gets (abstractly) evaluated in an empty environment
  // for now; eventually it should be empty except for globals, but right
  // now I don't have globals in the abstract environment..
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

  // I wait until here to check whether ft.precondition is NULL, to
  // maximize the common code, so it's less likely there will be
  // anomalous differences between calls with and w/o preconditions
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

  // make a new variable to stand for the value returned
  if (type->isIntegerType()) {
    return env.freshIntVariable(stringc
             << "function call result: " << toString());
  }
  else {
    return NULL;
  }
}


IntValue *E_fieldAcc::vcgen(AEnv &env) VCGEN_CONST
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


IntValue *E_unary::vcgen(AEnv &env) VCGEN_CONST
{
  // TODO: deal with ++, --
  
  if (op == UNY_SIZEOF) {
    return env.grab(new IVint(expr->type->reprSize()));
  }
  else {
    return env.grab(new IVunary(op, expr->vcgen(env)));
  }
}

IntValue *E_binary::vcgen(AEnv &env) VCGEN_CONST
{
  // TODO: deal with &&, ||
  return env.grab(new IVbinary(e1->vcgen(env), op, e2->vcgen(env)));
}


IntValue *E_addrOf::vcgen(AEnv &env) VCGEN_CONST
{
  // result is always of pointer type
  return NULL;
}


IntValue *E_deref::vcgen(AEnv &env) VCGEN_CONST
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


IntValue *E_cast::vcgen(AEnv &env) VCGEN_CONST
{
  // I can sustain integer->integer casts..
  if (type->isIntegerType() && expr->type->isIntegerType()) {
    return expr->vcgen(env);
  }
  else {
    return NULL;
  }
}


IntValue *E_cond::vcgen(AEnv &env) VCGEN_CONST
{
  // for now I'll assume there are no side effects in the branches;
  // same assumption applies to '&&' and '||' above
  return env.grab(new IVcond(cond->vcgen(env), th->vcgen(env), el->vcgen(env)));
}


IntValue *E_gnuCond::vcgen(AEnv &env) VCGEN_CONST
{
  // again on the assumption there are no side effects in the 'else' branch
  // TODO: make a deep-copier for astgen
  return NULL;
}


IntValue *E_comma::vcgen(AEnv &env) VCGEN_CONST
{
  // evaluate and discard
  env.discard(e1->vcgen(env));

  // evaluate and keep
  return e2->vcgen(env);
}


IntValue *E_sizeofType::vcgen(AEnv &env) VCGEN_CONST
{
  return env.grab(new IVint(size));
}


IntValue *E_assign::vcgen(AEnv &env) VCGEN_CONST
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


IntValue *E_arithAssign::vcgen(AEnv &env) VCGEN_CONST
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
IntValue *IN_expr::vcgen(AEnv &env, Type const *) VCGEN_CONST
{
  return e->vcgen(env);
}


IntValue *IN_compound::vcgen(AEnv &/*env*/, Type const */*type*/) VCGEN_CONST
{
  // I have no representation for array and structure values
  return NULL;
}


