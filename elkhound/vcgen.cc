// vcgen.cc
// vcgen methods on the C abstract syntax tree

#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // abstract domain values
#include "aenv.h"               // AEnv
#include "cc_type.h"            // FunctionType, etc.


// ------------------- TranslationUnit ---------------------
void TranslationUnit::vcgen(AEnv &env)
{
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->vcgen(env);
  }
}


// --------------------- TopForm ----------------------
void TF_decl::vcgen(AEnv &env)
{}


void TF_func::vcgen(AEnv &env)
{
  // clear anything left over in env
  env.clear();

  // add the function parameters to the environment
  FOREACH_OBJLIST(FunctionType::Param, ftype->params, iter) {
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
  cout << "interpretation after " << name << ":\n";
  env.print();
}


// --------------------- Declaration --------------------
void Declaration::vcgen(AEnv &env)
{
  FOREACH_ASTLIST_NC(Declarator, decllist, iter) {
    Declarator *dr = iter.data();

    StringRef name = dr->getName();
    if (name && dr->type->isIntegerType()) {
      env.set(name, new IVvar(name,
        stringc << "initial value of local " << name));
    }
  }
}


// ----------------------- Statement ---------------
// for now, ignore all control flow

void S_skip::vcgen(AEnv &env) {}
void S_label::vcgen(AEnv &env) {}
void S_case::vcgen(AEnv &env) {}
void S_caseRange::vcgen(AEnv &env) {}
void S_default::vcgen(AEnv &env) {}

void S_expr::vcgen(AEnv &env)
{        
  // evaluate and discard
  env.discard(expr->vcgen(env));
}

void S_compound::vcgen(AEnv &env)
{
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->vcgen(env);
  }
}

void S_if::vcgen(AEnv &env) {}
void S_switch::vcgen(AEnv &env) {}
void S_while::vcgen(AEnv &env) {}
void S_doWhile::vcgen(AEnv &env) {}
void S_for::vcgen(AEnv &env) {}
void S_break::vcgen(AEnv &env) {}
void S_continue::vcgen(AEnv &env) {}
void S_return::vcgen(AEnv &env) {}
void S_goto::vcgen(AEnv &env) {}


void S_decl::vcgen(AEnv &env) 
{
  decl->vcgen(env);
}


// ---------------------- Expression -----------------
IntValue *E_intLit::vcgen(AEnv &env)
{
  return env.grab(new IVint(i));
}

IntValue *E_floatLit::vcgen(AEnv &env) { return NULL; }
IntValue *E_stringLit::vcgen(AEnv &env) { return NULL; }

IntValue *E_charLit::vcgen(AEnv &env)
{
  return env.grab(new IVint(c));
}

IntValue *E_structLit::vcgen(AEnv &env) { return NULL; }

IntValue *E_variable::vcgen(AEnv &env)
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


IntValue *E_arrayAcc::vcgen(AEnv &env)
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


IntValue *E_funCall::vcgen(AEnv &env)
{
  // in all cases I plan to make a new variable to stand for the
  // value returned; I would then add assertions about that variable
  // based on the pre/postcondition of the function
  if (type->isIntegerType()) {
    return env.freshIntVariable(stringc
             << "function call result: " << toString());
  }
  else {
    return NULL;
  }
}


IntValue *E_fieldAcc::vcgen(AEnv &env)
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


IntValue *E_unary::vcgen(AEnv &env)
{
  // TODO: deal with ++, --
  return env.grab(new IVunary(op, expr->vcgen(env)));
}

IntValue *E_binary::vcgen(AEnv &env)
{
  // TODO: deal with &&, ||
  return env.grab(new IVbinary(e1->vcgen(env), op, e2->vcgen(env)));
}


IntValue *E_addrOf::vcgen(AEnv &env)
{
  // result is always of pointer type
  return NULL;
}


IntValue *E_deref::vcgen(AEnv &env)
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


IntValue *E_cast::vcgen(AEnv &env)
{
  // I can sustain integer->integer casts..
  if (type->isIntegerType() && expr->type->isIntegerType()) {
    return expr->vcgen(env);
  }
  else {
    return NULL;
  }
}


IntValue *E_cond::vcgen(AEnv &env)
{
  // for now I'll assume there are no side effects in the branches;
  // same assumption applies to '&&' and '||' above
  return env.grab(new IVcond(cond->vcgen(env), th->vcgen(env), el->vcgen(env)));
}


IntValue *E_gnuCond::vcgen(AEnv &env)
{
  // again on the assumption there are no side effects in the 'else' branch
  // TODO: make a deep-copier for astgen
  return NULL;
}


IntValue *E_comma::vcgen(AEnv &env)
{
  // evaluate and discard
  env.discard(e1->vcgen(env));

  // evaluate and keep
  return e2->vcgen(env);
}


IntValue *E_sizeofType::vcgen(AEnv &env)
{
  return env.grab(new IVint(size));
}


IntValue *E_assign::vcgen(AEnv &env)
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


IntValue *E_arithAssign::vcgen(AEnv &env)
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


