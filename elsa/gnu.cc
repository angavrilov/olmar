// gnu.cc
// tcheck and print routines for gnu.ast/gnu.gr extensions

#include "generic_aux.h"      // C++ AST, and genericPrintAmbiguities, etc.
#include "cc_env.h"           // Env
#include "cc_print.h"         // olayer, PrintEnv
#include "generic_amb.h"      // resolveAmbiguity, etc.


// --------------------------- Env ---------------------------------
// Caveat: All of the uses of GNU builtin functions arise from
// preprocessing with the gcc compiler's headers.  Strictly speaking,
// this is inappropriate, as Elsa is a different implementation and
// has its own compiler-specific headers (in the include/ directory).
// But in practice people don't often seem to be willing to adjust
// their build process enough to actually use Elsa's headers, and
// insist on using the gcc headers since that's what (e.g.) gcc -E
// finds by default.  Therefore Elsa makes a best-effort attempt to
// accept the resulting files, even though they are gcc-specific (and
// sometimes specific to a particular *version* of gcc).  This
// function is part of that effort.
//
// See http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Other-Builtins.html
void Env::addGNUBuiltins()
{
  Type *t_void = getSimpleType(SL_INIT, ST_VOID);
//    Type *t_voidconst = getSimpleType(SL_INIT, ST_VOID, CV_CONST);
  Type *t_voidptr = makePtrType(SL_INIT, t_void);
//    Type *t_voidconstptr = makePtrType(SL_INIT, t_voidconst);

  Type *t_int = getSimpleType(SL_INIT, ST_INT);
  Type *t_unsigned_int = getSimpleType(SL_INIT, ST_UNSIGNED_INT);
  Type *t_char = getSimpleType(SL_INIT, ST_CHAR);
  Type *t_charconst = getSimpleType(SL_INIT, ST_CHAR, CV_CONST);
  Type *t_charptr = makePtrType(SL_INIT, t_char);
  Type *t_charconstptr = makePtrType(SL_INIT, t_charconst);

  // dsw: This is a form, not a function, since it takes an expression
  // AST node as an argument; however, I need a function that takes no
  // args as a placeholder for it sometimes.
  var__builtin_constant_p = declareSpecialFunction("__builtin_constant_p");

  // typedef void *__builtin_va_list;
  Variable *var__builtin_va_list =
    makeVariable(SL_INIT, str("__builtin_va_list"),
                 t_voidptr, DF_TYPEDEF | DF_BUILTIN | DF_GLOBAL);
  addVariable(var__builtin_va_list);

  // void __builtin_stdarg_start(__builtin_va_list __list, char const *__format);
  // trying this instead:
  // void __builtin_stdarg_start(__builtin_va_list __list, void const *__format);
  // nope; see in/d0120.cc.  It doesn't work if the arg to '__format' is an int.
  // ironically, making it vararg does work
  declareFunction1arg(t_void, "__builtin_stdarg_start",
                      var__builtin_va_list->type, "__list",
//                        t_charconstptr, "__format",
//                        t_voidconstptr, "__format",
                      FF_VARARGS, NULL);

  // void __builtin_va_end(__builtin_va_list __list);
  declareFunction1arg(t_void, "__builtin_va_end",
                      var__builtin_va_list->type, "__list");

  // void *__builtin_alloca(unsigned int __len);
  declareFunction1arg(t_voidptr, "__builtin_alloca",
                      t_unsigned_int, "__len");

  // char *__builtin_strchr(char const *str, int ch);
  declareFunction2arg(t_charptr, "__builtin_strchr",
                      t_charconstptr, "str",
                      t_int, "ch",
                      FF_NONE, NULL);

  // char *__builtin_strpbrk(char const *str, char const *accept);
  declareFunction2arg(t_charptr, "__builtin_strpbrk",
                      t_charconstptr, "str",
                      t_charconstptr, "accept",
                      FF_NONE, NULL);

  // char *__builtin_strchr(char const *str, int ch);
  declareFunction2arg(t_charptr, "__builtin_strrchr",
                      t_charconstptr, "str",
                      t_int, "ch",
                      FF_NONE, NULL);

  // char *__builtin_strstr(char const *haystack, char const *needle);
  declareFunction2arg(t_charptr, "__builtin_strstr",
                      t_charconstptr, "haystack",
                      t_charconstptr, "needle",
                      FF_NONE, NULL);

  // we made some attempts to get accurate prototypes for the above
  // functions, but at some point just started using "int ()(...)"
  // as the type; the set below all get this generic type

  static char const * const arr[] = {
    // group 1: ?
    //"alloca",
    "bcmp",
    "bzero",
    "index",
    "rindex",
    "ffs",
    "fputs_unlocked",
    "printf_unlocked",
    "fprintf_unlocked",

    // group 2: C99
    "conj",
    "conjf",
    "conjl",
    "creal",
    "crealf",
    "creall",
    "cimag",
    "cimagf",
    "cimagl",
    "llabs",
    "imaxabs",

    // group 3: C99 / reserved C89
    "cosf",
    "cosl",
    "fabsf",
    "fabsl",
    "sinf",
    "sinl",
    "sqrtf",
    "sqrtl",

    // group 4: C89
    "abs",
    "cos",
    "fabs",
    "fprintf",
    "fputs",
    "labs",
    "memcmp",
    "memcpy",
    "memset",
    "printf",
    "sin",
    "sqrt",
    "strcat",
    //"strchr",
    "strcmp",
    "strcpy",
    "strcspn",
    "strlen",
    "strncat",
    "strncmp",
    "strncpy",
    //"strpbrk",
    //"strrchr",
    "strspn",
    //"strstr",

    // group 5: C99 floating point comparison macros
    "isgreater",
    "isgreaterequal",
    "isless",
    "islessequal",
    "islessgreater",
    "isunordered",
    
    // one more for good measure
    "prefetch",
  };
  
  for (int i=0; i < TABLESIZE(arr); i++) {
    makeImplicitDeclFuncVar(str(stringc << "__builtin_" << arr[i]));
  }
}


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
  expr->tcheck(env);
  // FIX: check the asRval(); A use in kernel suggests it should be
  // there as otherwise you get "error: cannot create a pointer to a
  // reference" when used to specify the type in a declarator that
  // comes from a de-reference (which yeilds a reference).
  return expr->getType()->asRval();
}


Type *TS_typeof_type::itcheck(Env &env, DeclFlags dflags)
{
  ASTTypeId::Tcheck tc(DF_NONE /*dflags don't apply to this type*/,
                       DC_TS_TYPEOF_TYPE);
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();
  return t;
}


Type *TS_typeof::itcheck(Env &env, DeclFlags dflags)
{
  atype = atype->tcheck(env, dflags);
  return atype->type;
}


void S_function::itcheck(Env &env)
{
  env.setLoc(loc);
  f->tcheck(env);
}


Type *E_compoundLit::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_COMPOUNDLIT);
  
  if (tcheckedType) {
    // The 'stype' has already been tchecked, because this expression
    // is in a context that involves multiple tcheck passes
    // (e.g. gnu/g0013.cc); just skip subsequent tchecks of 'stype'
    // b/c its interpretation won't be changing, and if it happens to
    // define a type then on the second pass this avoids an error
    // about redeclaration.
    //
    // Hmm... I'd been thinking this would help deal with ambiguous
    // contexts too, but the ambiguity resolver doesn't want changes
    // to the environment to take place ..... ?  Oh well...
  }
  else {                
    // tcheck it normally
    stype = stype->tcheck(env, tc);
    tcheckedType = true;
  }

  init->tcheck(env, NULL);
  
  return env.computeArraySizeFromCompoundInit(env.loc(), stype->getType(), stype->getType(), init);
  // TODO: check that the cast (literal) makes sense
}


Type *E___builtin_constant_p::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

//    // TODO: this will fail an assertion if someone asks for the
//    // size of a variable of template-type-parameter type..
//    // dsw: If this is turned back on, be sure to catch the possible
//    // XReprSize exception and add its message to the env.error-s
//    size = expr->type->asRval()->reprSize();
//    TRACE("sizeof", "sizeof(" << expr->exprToString() <<
//                    ") is " << size);

  // dsw: the type of a __builtin_constant_p is an int:
  // http://gcc.gnu.org/onlinedocs/gcc-3.2.2/gcc/Other-Builtins.html#Other%20Builtins
  // TODO: is this right?
  return expr->type->isError()?
           expr->type : env.getSimpleType(SL_UNKNOWN, ST_UNSIGNED_INT);
}


Type *E___builtin_va_arg::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_BUILTIN_VA_ARG);
  expr->tcheck(env, expr);
  atype = atype->tcheck(env, tc);
  return atype->getType();
}


Type *E_alignofType::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_ALIGNOFTYPE);
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();
  // dsw: If this is turned back on, be sure to catch the possible
  // XReprSize exception and add its message to the env.error-s
//    size = t->reprSize();

  return t->isError()? t : env.getSimpleType(SL_UNKNOWN, ST_UNSIGNED_INT);
}


Type *E_statement::itcheck_x(Env &env, Expression *&replacement)
{
  s = s->tcheck(env)->asS_compound();
  if (s->stmts.count() < 1) {
    return env.error("`({ ... })' cannot be empty");
  }

  Statement *last = s->stmts.last();
  if (last->isS_expr()) {
    return last->asS_expr()->expr->getType();
  }
  else {
    return env.getSimpleType(env.loc(), ST_VOID, CV_NONE);
    // There are examples that do not end with an S_expr.
//      return env.error("last thing in `({ ... })' must be an expression");
  }
}


Type *E_gnuCond::itcheck_x(Env &env, Expression *&replacement)
{
  cond->tcheck(env, cond);
  el->tcheck(env, el);
  
  // presumably the correct result type is some sort of intersection
  // of the 'cond' and 'el' types?

  return el->type;
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


// ------------------ const-eval, etc. -------------------
bool E_gnuCond::extConstEval(string &msg, int &result) const
{
  if (!cond->constEval(msg, result)) return false;

  if (result) {
    return result;
  }
  else {
    return el->constEval(msg, result);
  }
}

bool E_gnuCond::extHasUnparenthesizedGT() const
{
  return cond->hasUnparenthesizedGT() ||
         el->hasUnparenthesizedGT();
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


void S_function::iprint(PrintEnv &env)
{
  olayer ol("S_function::iprint");
  f->print(env);
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

void E___builtin_va_arg::iprint(PrintEnv &env)
{
  olayer ol("E___builtin_va_arg::iprint");
  codeout co(env, "__builtin_va_arg", "(", ")");
  expr->print(env);
  env << ", ";
  atype->print(env);
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

void E_gnuCond::iprint(PrintEnv &env)
{
  olayer ol("E_gnuCond::iprint");
  codeout co(env, "", "(", ")");
  cond->print(env);
  env << " ?: ";
  el->print(env);
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


// ------------------------ cfg --------------------------

// WARNING: The control flow graph will show that the statement before
// the S_function flows into the S_function and that the S_function
// flows into the next statement.  If you know that an S_function is
// just a function definition and does nothing at run time, this is
// harmless, but it is a little odd, as in reality control would jump
// over the S_function.  The only way to prevent this that I can see
// would be for cfg.cc:Statement::computeCFG() to know about
// S_function which would eliminate the usefulness of having it in the
// gnu extension, or for S_function::icfg to go up and do some surgery
// on edges that have already been added, which I consider to be too
// weird.
//
// Scott says: "the entire S_function::icfg can be empty, just like
// S_skip."
void S_function::icfg(CFGEnv &env) {}
