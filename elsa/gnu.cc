// gnu.cc
// tcheck and print routines for gnu.ast/gnu.gr extensions

#include "generic_aux.h"      // C++ AST, and genericPrintAmbiguities, etc.
#include "cc_env.h"           // Env
#include "cc_print.h"         // olayer, PrintEnv
#include "generic_amb.h"      // resolveAmbiguity, etc.
#include "cc_tcheck.h"        // computeArraySizeFromCompoundInit

// -------------------- tcheck --------------------
ASTTypeof *ASTTypeof::tcheck(Env &env, DeclFlags dflags)
{
  if (!ambiguity) {
    mid_tcheck(env, dflags);
    return this;
  }
  
  return resolveAmbiguity(this, env, "ASTTypeof", false /*priority*/, dflags);
}

void ASTTypeof::mid_tcheck(Env &env, DeclFlags &dflags)
{
  type = itcheck(env, dflags);
}


Type *TS_typeof_expr::itcheck(Env &env, DeclFlags dflags)
{
  // FIX: dflags discarded?
  expr->tcheck(env, expr);
  // FIX: check the asRval(); A use in kernel suggests it should be
  // there as otherwise you get "error: cannot create a pointer to a
  // reference" when used to specify the type in a declarator that
  // comes from a de-reference (which yeilds a reference).
  return expr->type->asRval();
}


Type *TS_typeof_type::itcheck(Env &env, DeclFlags dflags)
{
  // FIX: dflags discarded?
  ASTTypeId::Tcheck tc;
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();
  return t;
}


Type *TS_typeof::itcheck(Env &env, DeclFlags dflags)
{
  atype = atype->tcheck(env, dflags);
  return atype->type;
}


Type *E_compoundLit::itcheck(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc;
  stype = stype->tcheck(env, tc);
  init->tcheck(env, NULL);
  
  return computeArraySizeFromCompoundInit(env, env.loc(), stype->getType(), stype->getType(), init);
  // TODO: check that the cast (literal) makes sense
}


Type *E___builtin_constant_p::itcheck(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

//    // TODO: this will fail an assertion if someone asks for the
//    // size of a variable of template-type-parameter type..
//    size = expr->type->asRval()->reprSize();
//    TRACE("sizeof", "sizeof(" << expr->exprToString() <<
//                    ") is " << size);

  // dsw: the type of a __builtin_constant_p is an int:
  // http://gcc.gnu.org/onlinedocs/gcc-3.2.2/gcc/Other-Builtins.html#Other%20Builtins
  // TODO: is this right?
  return expr->type->isError()?
           expr->type : env.getSimpleType(SL_UNKNOWN, ST_UNSIGNED_INT);
}


Type *E_alignofType::itcheck(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc;
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();
//    size = t->reprSize();

  return t->isError()? t : env.getSimpleType(SL_UNKNOWN, ST_UNSIGNED_INT);
}


Type *E_statement::itcheck(Env &env, Expression *&replacement)
{
  s = s->tcheck(env)->asS_compound();
  if (s->stmts.count() < 1) {
    return env.error("`({ ... })' cannot be empty");
  }

  Statement *last = s->stmts.last();
  if (last->isS_expr()) {
    return last->asS_expr()->expr->type;
  }
  else {
    return env.error("last thing in `({ ... })' must be an expression");
  }
}


static void compile_time_compute_int_expr(Env &env, Expression *e, int &x, char *error_msg) {
  e->tcheck(env, e);
  if (!e->constEval(env, x)) env.error(error_msg);
}

static void check_designator_list(Env &env, FakeList<Designator> *dl)
{
  xassert(dl);
  FAKELIST_FOREACH_NC(Designator, dl, d) {
    if (SubscriptDesignator *sd = dynamic_cast<SubscriptDesignator*>(d)) {
      compile_time_compute_int_expr(env, sd->idx_expr, sd->idx_computed,
                                    "compile-time computation of range start designator array index fails");
      if (sd->idx_expr2) {
        compile_time_compute_int_expr(env, sd->idx_expr2, sd->idx_computed2,
                                      "compile-time computation of range end designator array index fails");
      }
    }
    // nothing to do for FieldDesignator-s
  }
}

void IN_designated::tcheck(Env &env, Type *type)
{
  init->tcheck(env, type);
  check_designator_list(env, designator_list);
}


// ------------------------ print --------------------------
void TS_typeof::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    olayer ol("TS_typeof_expr");
}


void ASTTypeof::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "TypeSpecifier", os, indent);
    
  // sm: what was this here for?
  //genericCheckNexts(this);
}


void ASTTypeof::addAmbiguity(ASTTypeof *alt)
{
  //genericAddAmbiguity(this, alt);
  
  // insert 'alt' at the head of the 'ambiguity' list
  xassert(alt->ambiguity == NULL);
  alt->ambiguity = ambiguity;
  ambiguity = alt;
}


void E_compoundLit::iprint(PrintEnv &env)
{
  olayer ol("E_compoundLit::iprint");
  {
    codeout co(env, "", "(", ")");
    stype->print(env);
  }
  init->print(env);
}

void E___builtin_constant_p::iprint(PrintEnv &env)
{
  olayer ol("E___builtin_constant_p::iprint");
  codeout co(env, "__builtin_constant_p", "(", ")");
  expr->print(env);
}

void E_alignofType::iprint(PrintEnv &env)
{
  olayer ol("E_alignofType::iprint");
  codeout co(env, "__alignof__", "(", ")");
  atype->print(env);
}

void E_statement::iprint(PrintEnv &env)
{
  olayer ol("E_statement::iprint");
  codeout co(env, "", "(", ")");
  s->iprint(env);
}

// prints designators in the new C99 style, not the obsolescent ":"
// style
static void print_DesignatorList(PrintEnv &env, FakeList<Designator> *dl) {
  xassert(dl);
  FAKELIST_FOREACH_NC(Designator, dl, d) d->print(env);
  env << "=";
}

void IN_designated::print(PrintEnv &env)
{
  print_DesignatorList(env, designator_list);
  init->print(env);
}

// -------------------- Designator ---------------

void FieldDesignator::print(PrintEnv &env)
{
  olayer ol("FieldDesignator");
  xassert(id);
  env << "." << id;
}

void SubscriptDesignator::print(PrintEnv &env)
{
  olayer ol("SubscriptDesignator");
  xassert(idx_expr);
  codeout co(env, "", "[", "]");
  idx_expr->print(env);
  if (idx_expr2) {
    env << " ... ";
    idx_expr2->print(env);
  }
}


void Designator::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Designator", os, indent);
  
  genericCheckNexts(this);
}

void Designator::addAmbiguity(Designator *alt)
{
  genericAddAmbiguity(this, alt);
}

void Designator::setNext(Designator *newNext)
{
  genericSetNext(this, newNext);
}

