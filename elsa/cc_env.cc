// cc_env.cc            see license.txt for copyright and terms of use
// code for cc_env.h

#include "cc_env.h"        // this module
#include "trace.h"         // tracingSys
#include "ckheap.h"        // heapCheck
#include "strtable.h"      // StringTable
#include "cc_lang.h"       // CCLang
#include "strutil.h"       // suffixEquals, prefixEquals
#include "overload.h"      // OVERLOADTRACE


void gdbScopeSeq(ScopeSeq &ss)
{
  cout << "scope sequence" << endl;
  for(int i=0; i<ss.length(); ++i) {
    Scope *scope = ss[i];
    xassert(scope);
    cout << "\t[" << i << "] ";
    scope->gdb();
  }
}


inline ostream& operator<< (ostream &os, SourceLoc sl)
  { return os << toString(sl); }


// --------------------

// true if 't' is reference to 'ct', ignoring any c/v
static bool isRefToCt(Type const *t, CompoundType *ct)
{
  if (!t->isReference()) return false;

  ReferenceType const *rt = t->asReferenceTypeC();
  if (!rt->atType->isCVAtomicType()) return false;

  CVAtomicType const *at = rt->atType->asCVAtomicTypeC();
  if (at->atomic != ct) return false; // NOTE: atomics are equal iff pointer equal

  return true;
}


// cppstd 12.8 para 2: "A non-template constructor for a class X is a
// _copy_ constructor if its first parameter is of type X&, const X&,
// volatile X&, or const volatile X&, and either there are no other
// parameters or else all other parameters have default arguments
// (8.3.6)."
bool isCopyConstructor(Variable const *funcVar, CompoundType *ct)
{
  FunctionType *ft = funcVar->type->asFunctionType();
  if (!ft->isConstructor()) return false; // is a ctor?
  if (funcVar->isTemplate()) return false; // non-template?
  if (ft->params.isEmpty()) return false; // has at least one arg?

  // is the first parameter a ref to the class type?
  if (!isRefToCt(ft->params.firstC()->type, ct)) return false;

  // do all the parameters past the first one have default arguments?
  bool first_time = true;
  SFOREACH_OBJLIST(Variable, ft->params, paramiter) {
    // skip the first variable
    if (first_time) {
      first_time = false;
      continue;
    }
    if (!paramiter.data()->value) return false;
  }

  return true;                  // all test pass
}


// cppstd 12.8 para 9: "A user-declared _copy_ assignment operator
// X::operator= is a non-static non-template member function of class
// X with exactly one parameter of type X, X&, const X&, volatile X&
// or const volatile X&."
bool isCopyAssignOp(Variable const *funcVar, CompoundType *ct)
{
  FunctionType *ft = funcVar->type->asFunctionType();
  if (!ft->isMethod()) return false; // is a non-static member?
  if (funcVar->isTemplate()) return false; // non-template?
  if (ft->params.count() != 2) return false; // has two args, 1) this and 2) other?

  // the second parameter; the first is "this"
  Type *t0 = ft->params.nthC(1 /*that is, the second element*/)->type;

  // is the parameter of the class type?  NOTE: atomics are equal iff
  // pointer equal
  if (t0->isCVAtomicType() && t0->asCVAtomicType()->atomic == ct) return true;

  // or, is the parameter a ref to the class type?
  return isRefToCt(t0, ct);
}


typedef bool (*MemberFnTest)(Variable const *funcVar, CompoundType *ct);

// test for any match among a variable's overload set
bool testAmongOverloadSet(MemberFnTest test, Variable *v, CompoundType *ct)
{
  if (!v) {
    // allow this, and say there's no match, because there are
    // no variables at all
    return false;
  }

  if (!v->overload) {
    // singleton set
    if (test(v, ct)) {
      return true;
    }
  }
  else {
    // more than one element; note that the original 'v' is always
    // among the elements of this list
    SFOREACH_OBJLIST_NC(Variable, v->overload->set, iter) {
      if (test(iter.data(), ct)) {
        return true;
      }
    }
  }

  return false;        // no match
}

// this adds:
//   - a default (no-arg) ctor, if no ctor (of any kind) is already present
//   - a copy ctor if no copy ctor is present
//   - an operator= if none is present
//   - a dtor if none is present
// 'loc' remains a hack ...
//
// FIX: this should be a method on TS_classSpec
// sm: I don't agree.
void addCompilerSuppliedDecls(Env &env, SourceLoc loc, CompoundType *ct)
{
  // we shouldn't even be here if the language isn't C++.
  xassert(env.lang.isCplusplus);

  // the caller should already have arranged so that 'ct' is the
  // innermost scope
  xassert(env.acceptingScope() == ct);

  // don't bother for anonymous classes (this was originally because
  // the destructor would then not have a name, but it's been retained
  // even as more compiler-supplied functions have been added)
  if (!ct->name) {
    return;
  }

  // **** implicit no-arg (aka "default") ctor: cppstd 12.1 para 5:
  // "If there is no user-declared constructor for class X, a default
  // constructor is implicitly declared."
  if (!ct->getNamedField(env.constructorSpecialName, env, LF_INNER_ONLY)) {
    // add a no-arg ctor declaration: "Class();".  For now we just
    // add the variable to the scope and don't construct the AST, in
    // order to be symmetric with what is going on with the dtor
    // below.
    FunctionType *ft = env.beginConstructorFunctionType(loc, ct);
    env.doneParams(ft);
    Variable *v = env.makeVariable(loc, env.constructorSpecialName, ft,
                  DF_MEMBER | DF_IMPLICIT);
    // NOTE: we don't use env.addVariableWithOload() because this is
    // a special case: we only insert if there are no ctors AT ALL.
    env.addVariable(v);
    env.madeUpVariables.push(v);
  }

  // **** implicit copy ctor: cppstd 12.8 para 4: "If the class
  // definition does not explicitly declare a copy constructor, one
  // is declared implicitly."
  Variable *ctor0 = ct->getNamedField(env.constructorSpecialName, env, LF_INNER_ONLY);
  xassert(ctor0);             // we just added one if there wasn't one

  // is there a copy constructor?  I'm rolling my own here.
  if (!testAmongOverloadSet(isCopyConstructor, ctor0, ct)) {
    // cppstd 12.8 para 5: "The implicitly-declared copy constructor
    // for a class X will have the form
    //
    //   X::X(const X&)
    //
    // if [lots of complicated conditions about the superclasses have
    // const copy ctors, etc.] ... Otherwise, the implicitly-declared
    // copy constructor will have the form
    //
    //   X::X(X&)
    //
    // An implicitly-declared copy constructor is an inline public
    // member of its class."

    // dsw: I'm going to just always make it X::X(X const &) for now.
    // TODO: do it right.

    // create the effects of a declaration without making any AST or
    // a body; add a copy ctor declaration: Class(Class const &__other);
    FunctionType *ft = env.beginConstructorFunctionType(loc, ct);
    Variable *refToSelfParam =
      env.makeVariable(loc,
                       env.otherName,
                       env.makeReferenceType(loc,
                         env.makeCVAtomicType(loc, ct, CV_CONST)),
                       DF_PARAMETER);
    ft->addParam(refToSelfParam);
    env.doneParams(ft);
    Variable *v = env.makeVariable(loc, env.constructorSpecialName, ft,
                                   DF_MEMBER | DF_IMPLICIT);
    env.addVariableWithOload(ctor0, v);     // always overloaded; ctor0!=NULL
    env.madeUpVariables.push(v);
  }

  // **** implicit copy assignment operator: 12.8 para 10: "If the
  // class definition does not explicitly declare a copy assignment
  // operator, one is declared implicitly."
  Variable *assign_op0 = ct->getNamedField(env.operatorName[OP_ASSIGN],
                                           env, LF_INNER_ONLY);
  // is there a copy assign op?  I'm rolling my own here.
  if (!testAmongOverloadSet(isCopyAssignOp, assign_op0, ct)) {
    // 12.8 para 10: "The implicitly-declared copy assignment operator
    // for a class X will have the form
    //
    //   X& X::operator=(const X&)
    //
    // if [lots of complicated conditions about the superclasses have
    // const-parmeter copy assignment, etc.] ... Otherwise, the
    // implicitly-declared copy assignment operator [mistake in spec:
    // it says "copy constructor"] will have the form
    //
    //   X& X::operator=(X&)
    //
    // The implicitly-declared copy assignment
    // operator for class X has the return type X&; it returns the
    // object for which the assignment operator is invoked, that is,
    // the object assigned to.  An implicitly-declared copy assignment
    // operator is an inline public member of its class. ..."

    // dsw: I'm going to just always make the parameter const for now.
    // TODO: do it right.

    // add a copy assignment op declaration: Class& operator=(Class const &);
    Type *refToSelfType =
      env.makeReferenceType(loc, env.makeCVAtomicType(loc, ct, CV_NONE));
    Type *refToConstSelfType =
      env.makeReferenceType(loc, env.makeCVAtomicType(loc, ct, CV_CONST));

    FunctionType *ft = env.makeFunctionType(loc, refToSelfType);

    // receiver object
    ft->addReceiver(env.makeVariable(loc, env.receiverName,
                                     env.tfac.cloneType(refToSelfType),
                                     DF_PARAMETER));

    // source object parameter
    ft->addParam(env.makeVariable(loc,
                                  env.otherName,
                                  env.tfac.cloneType(refToConstSelfType),
                                  DF_PARAMETER));

    env.doneParams(ft);

    Variable *v = env.makeVariable(loc, env.operatorName[OP_ASSIGN], ft,
                                   DF_MEMBER | DF_IMPLICIT);
    env.addVariableWithOload(assign_op0, v);
    env.madeUpVariables.push(v);
  }

  // **** implicit dtor: declare a destructor if one wasn't declared
  // already; this allows the user to call the dtor explicitly, like
  // "a->~A();", since I treat that like a field lookup
  StringRef dtorName = env.str(stringc << "~" << ct->name);
  if (!ct->lookupVariable(dtorName, env, LF_INNER_ONLY)) {
    // add a dtor declaration: ~Class();
    FunctionType *ft = env.makeDestructorFunctionType(loc, ct);
    Variable *v = env.makeVariable(loc, dtorName, ft,
                                   DF_MEMBER | DF_IMPLICIT);
    env.addVariable(v);       // cannot be overloaded

    // put it on the list of made-up variables since there are no
    // (e.g.) $tainted qualifiers (since the user didn't even type
    // the dtor's name)
    env.madeUpVariables.push(v);
  }
}


// --------------------- Env -----------------

int throwClauseSerialNumber = 0; // don't make this a member of Env

Env::Env(StringTable &s, CCLang &L, TypeFactory &tf, TranslationUnit *tunit0)
  : ErrorList(),

    env(*this),
    scopes(),
    disambiguateOnly(false),
    anonTypeCounter(1),
    ctorFinished(false),

    disambiguationNestingLevel(0),
    checkFunctionBodies(true),
    secondPassTcheck(false),
    errors(*this),
    hiddenErrors(NULL),
    instantiationLocStack(),

    str(s),
    lang(L),
    tfac(tf),
    madeUpVariables(),

    // some special names; pre-computed (instead of just asking the
    // string table for it each time) because in certain situations
    // I compare against them frequently; the main criteria for
    // choosing the fake names is they have to be untypable by the C++
    // programmer (i.e. if the programmer types one it won't be
    // lexed as a single name)
    conversionOperatorName(str("conversion-operator")),
    constructorSpecialName(str("constructor-special")),
    functionOperatorName(str("operator()")),
    receiverName(str("__receiver")),
    otherName(str("__other")),
    quote_C_quote(str("\"C\"")),
    quote_C_plus_plus_quote(str("\"C++\"")),
    string__func__(str("__func__")),
    string__FUNCTION__(str("__FUNCTION__")),
    string__PRETTY_FUNCTION__(str("__PRETTY_FUNCTION__")),

    // these are done below because they have to be declared as functions too
    special_checkType(NULL),
    special_getStandardConversion(NULL),
    special_getImplicitConversion(NULL),
    special_testOverload(NULL),
    special_computeLUB(NULL),
    special_checkCalleeDefnLine(NULL),

    dependentScope(new Scope(SK_GLOBAL, 0, SL_INIT)),
    dependentTypeVar(NULL),
    dependentVar(NULL),
    errorTypeVar(NULL),
    errorVar(NULL),
    errorCompoundType(NULL),

    globalScopeVar(NULL),

    var__builtin_constant_p(NULL),

    // operatorName[] initialized below

    // builtin{Un,Bin}aryOperator[] start as arrays of empty
    // arrays, and then have things added to them below

    tunit(tunit0),

    doFunctionTemplateBodyInstantiation(!tracingSys("disableFBodyInst")),

    // this can be turned off with its own flag, or in C mode (since I
    // have not implemented any of the relaxed C rules)
    doCompareArgsToParams(!tracingSys("doNotCompareArgsToParams") && L.isCplusplus),

    // 2005-03-09: things are finally ready to turn strict checking
    // on by default
    doReportTemplateErrors(!tracingSys("permissive")),

    collectLookupResults("")
{
  // create first scope
  SourceLoc emptyLoc = SL_UNKNOWN;
  {
    // among other things, SK_GLOBAL causes Variables inserted into
    // this scope to acquire DF_GLOBAL
    Scope *s = new Scope(SK_GLOBAL, 0 /*changeCount*/, emptyLoc);
    scopes.prepend(s);
    
    // make a Variable for it
    globalScopeVar = makeVariable(SL_INIT, str("<globalScope>"), 
                                  NULL /*type*/, DF_NAMESPACE);
    globalScopeVar->scope = s;
  }

  dependentTypeVar = makeVariable(SL_INIT, str("<dependentTypeVar>"),
                                  getSimpleType(SL_INIT, ST_DEPENDENT), DF_TYPEDEF);

  dependentVar = makeVariable(SL_INIT, str("<dependentVar>"),
                              getSimpleType(SL_INIT, ST_DEPENDENT), DF_NONE);

  errorTypeVar = makeVariable(SL_INIT, str("<errorTypeVar>"),
                              getSimpleType(SL_INIT, ST_ERROR), DF_TYPEDEF);

  // this is *not* a typedef, because I use it in places that I
  // want something to be treated as a variable, not a type
  errorVar = makeVariable(SL_INIT, str("<errorVar>"),
                          getSimpleType(SL_INIT, ST_ERROR), DF_NONE);

  errorCompoundType = tfac.makeCompoundType(CompoundType::K_CLASS, str("<errorCompoundType>"));
  errorCompoundType->typedefVar = errorTypeVar;

  // create declarations for some built-in operators
  // [cppstd 3.7.3 para 2]
  Type *t_void = getSimpleType(SL_INIT, ST_VOID);
  Type *t_voidptr = makePtrType(SL_INIT, t_void);

  // note: my stddef.h typedef's size_t to be 'int', so I just use
  // 'int' directly here instead of size_t
  Type *t_size_t = getSimpleType(SL_INIT, ST_INT);

  // but I do need a class called 'bad_alloc'..
  //   class bad_alloc;
  //
  // 9/26/04: I had been *defining* bad_alloc here, but gcc defines
  // the class in new.h so I am following suit.
  CompoundType *dummyCt;
  Type *t_bad_alloc =
    makeNewCompound(dummyCt, scope(), str("bad_alloc"), SL_INIT,
                    TI_CLASS, true /*forward*/);

  // void* operator new(std::size_t sz) throw(std::bad_alloc);
  declareFunction1arg(t_voidptr, "operator new",
                      t_size_t, "sz",
                      FF_NONE, t_bad_alloc);

  // void* operator new[](std::size_t sz) throw(std::bad_alloc);
  declareFunction1arg(t_voidptr, "operator new[]",
                      t_size_t, "sz",
                      FF_NONE, t_bad_alloc);

  // void operator delete (void *p) throw();
  declareFunction1arg(t_void, "operator delete",
                      t_voidptr, "p",
                      FF_NONE, t_void);

  // void operator delete[] (void *p) throw();
  declareFunction1arg(t_void, "operator delete[]",
                      t_voidptr, "p",
                      FF_NONE, t_void);

  // 5/04/04: sm: I moved this out of the GNU_EXTENSION section because
  // the mozilla tests use it, and I won't want to make them only
  // active in gnu mode ..
  //
  // 9/23/04: I made the 'p' argument be '<any type>' instead of 'void *'
  // since in C++ not all types can be converted to 'void*'.
  //
  // for GNU compatibility
  // void *__builtin_next_arg(void const *p);
  declareFunction1arg(t_voidptr, "__builtin_next_arg",
                      getSimpleType(SL_INIT, ST_ANY_TYPE), "p");

  if (lang.predefined_Bool) {
    // sm: I found this; it's a C99 feature: 6.2.5, para 2.  It's
    // actually a keyword, so should be lexed specially, but I'll
    // leave it alone for now.
    //
    // typedef bool _Bool;
    Type *t_bool = getSimpleType(SL_INIT, ST_BOOL);
    addVariable(makeVariable(SL_INIT, str("_Bool"),
                             t_bool, DF_TYPEDEF | DF_BUILTIN | DF_GLOBAL));
  }

  #ifdef GNU_EXTENSION
    if (lang.declareGNUBuiltins) {
      addGNUBuiltins();
    }
  #endif // GNU_EXTENSION

  // for testing various modules
  special_checkType = declareSpecialFunction("__checkType")->name;
  special_getStandardConversion = declareSpecialFunction("__getStandardConversion")->name;
  special_getImplicitConversion = declareSpecialFunction("__getImplicitConversion")->name;
  special_testOverload = declareSpecialFunction("__testOverload")->name;
  special_computeLUB = declareSpecialFunction("__computeLUB")->name;
  special_checkCalleeDefnLine = declareSpecialFunction("__checkCalleeDefnLine")->name;

  setupOperatorOverloading();

  ctorFinished = true;
}


// slightly clever: iterate over an array, but look like
// the contents of the array, not the index
class OverloadableOpIter {
  int i;                          // current operator
  OverloadableOp const *ops;      // array
  int size;                       // array size

public:
  OverloadableOpIter(OverloadableOp const *o, int s)
    : i(0),
      ops(o),
      size(s)
  {}

  // look like the contents
  operator OverloadableOp ()   { return ops[i]; }

  // iterator interface
  bool isDone() const          { return i>=size; }
  void adv()                   { i++; }
};

#define FOREACH_OPERATOR(iterName, table) \
  for (OverloadableOpIter iterName(table, TABLESIZE(table)); !iterName.isDone(); iterName.adv())


void Env::setupOperatorOverloading()
{
  // fill in operatorName[]
  int i;
  for (i=0; i < NUM_OVERLOADABLE_OPS; i++) {
    // debugging hint: if this segfaults, then I forgot to add
    // something to operatorFunctionNames[]
    operatorName[i] = str(operatorFunctionNames[i]);
  }

  // the symbols declared here are prevented from being entered into
  // the environment by FF_BUILTINOP

  // this has to match the typedef in include/stddef.h
  SimpleTypeId ptrdiff_t_id = ST_INT;
  Type *t_ptrdiff_t = getSimpleType(SL_INIT, ptrdiff_t_id);

  // some other useful types
  Type *t_int = getSimpleType(SL_INIT, ST_INT);
  Type *t_bool = getSimpleType(SL_INIT, ST_BOOL);

  // Below, whenever the standard calls for 'VQ' (volatile or
  // nothing), I use two copies of the rule, one with volatile and one
  // without.  It would be nice to have a way of encoding this choice
  // directly, but all of the ways I've considered so far would do
  // more damage to the type system than I'm prepared to do for that
  // feature.

  // ------------ 13.6 para 3 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ARITHMETIC);
    Type *Tv = getSimpleType(SL_INIT, ST_ARITHMETIC, CV_VOLATILE);

    Type *Tr = tfac.makeReferenceType(SL_INIT, T);
    Type *Tvr = tfac.makeReferenceType(SL_INIT, Tv);

    // VQ T& operator++ (VQ T&);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_PLUSPLUS, Tr);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_PLUSPLUS, Tvr);

    // T operator++ (VQ T&, int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_PLUSPLUS, Tr, t_int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_PLUSPLUS, Tvr, t_int);
  }

  // ------------ 13.6 para 4 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ARITHMETIC_NON_BOOL);
    Type *Tv = getSimpleType(SL_INIT, ST_ARITHMETIC_NON_BOOL, CV_VOLATILE);

    Type *Tr = tfac.makeReferenceType(SL_INIT, T);
    Type *Tvr = tfac.makeReferenceType(SL_INIT, Tv);

    // VQ T& operator-- (VQ T&);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_MINUSMINUS, Tr);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_MINUSMINUS, Tvr);

    // T operator-- (VQ T&, int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_MINUSMINUS, Tr, t_int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_MINUSMINUS, Tvr, t_int);
  }

  // ------------ 13.6 para 5 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_OBJ_TYPE);

    Type *Tp = tfac.makePointerType(SL_INIT, CV_NONE, T);
    Type *Tpv = tfac.makePointerType(SL_INIT, CV_VOLATILE, T);

    Type *Tpr = tfac.makeReferenceType(SL_INIT, Tp);
    Type *Tpvr = tfac.makeReferenceType(SL_INIT, Tpv);

    // T* VQ & operator++ (T* VQ &);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_PLUSPLUS, Tpr);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_PLUSPLUS, Tpvr);

    // T* VQ & operator-- (T* VQ &);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_MINUSMINUS, Tpr);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_MINUSMINUS, Tpvr);

    // T* operator++ (T* VQ &, int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_PLUSPLUS, Tpr, t_int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_PLUSPLUS, Tpvr, t_int);

    // T* operator-- (T* VQ &, int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_MINUSMINUS, Tpr, t_int);
    addBuiltinBinaryOp(ST_PRET_STRIP_REF, OP_MINUSMINUS, Tpvr, t_int);
  }

  // ------------ 13.6 paras 6 and 7 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_NON_VOID);
    Type *Tp = makePtrType(SL_INIT, T);

    // T& operator* (T*);
    addBuiltinUnaryOp(ST_PRET_FIRST_PTR2REF, OP_STAR, Tp);
  }

  // ------------ 13.6 para 8 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_TYPE);
    Type *Tp = makePtrType(SL_INIT, T);

    // T* operator+ (T*);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_PLUS, Tp);
  }

  // ------------ 13.6 para 9 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);

    // T operator+ (T);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_PLUS, T);

    // T operator- (T);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_MINUS, T);
  }

  // ------------ 13.6 para 10 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);

    // T operator~ (T);
    addBuiltinUnaryOp(ST_PRET_FIRST, OP_BITNOT, T);
  }

  // ------------ 13.6 para 11 ------------
  // OP_ARROW_STAR is handled specially
  addBuiltinBinaryOp(OP_ARROW_STAR, new ArrowStarCandidateSet);

  // ------------ 13.6 para 12 ------------
  {
    Type *L = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);
    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);

    static OverloadableOp const ops1[] = {
      OP_STAR,         // LR operator* (L, R);
      OP_DIV,          // LR operator/ (L, R);
      OP_PLUS,         // LR operator+ (L, R);
      OP_MINUS,        // LR operator- (L, R);
    };
    FOREACH_OPERATOR(op1, ops1) {
      addBuiltinBinaryOp(ST_PRET_ARITH_CONV, op1, L, R);
    }

    static OverloadableOp const ops2[] = {
      OP_LESS,         // bool operator< (L, R);
      OP_GREATER,      // bool operator> (L, R);
      OP_LESSEQ,       // bool operator<= (L, R);
      OP_GREATEREQ,    // bool operator>= (L, R);
      OP_EQUAL,        // bool operator== (L, R);
      OP_NOTEQUAL      // bool operator!= (L, R);
    };
    FOREACH_OPERATOR(op2, ops2) {
      addBuiltinBinaryOp(ST_BOOL, op2, L, R);
    }
  }

  // ------------ 13.6 para 13 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_OBJ_TYPE);
    Type *Tp = makePtrType(SL_INIT, T);

    // T* operator+ (T*, ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_PLUS, Tp, t_ptrdiff_t);

    // T& operator[] (T*, ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST_PTR2REF, OP_BRACKETS, Tp, t_ptrdiff_t);

    // T* operator- (T*, ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_MINUS, Tp, t_ptrdiff_t);

    // T* operator+ (ptrdiff_t, T*);
    addBuiltinBinaryOp(ST_PRET_SECOND, OP_PLUS, t_ptrdiff_t, Tp);

    // T& operator[] (ptrdiff_t, T*);
    addBuiltinBinaryOp(ST_PRET_SECOND_PTR2REF, OP_BRACKETS, t_ptrdiff_t, Tp);
  }

  // ------------ 13.6 para 14 ------------
  // ptrdiff_t operator-(T,T);
  addBuiltinBinaryOp(ptrdiff_t_id, OP_MINUS, rvalIsPointer, pointerToObject);

  // summary of process:
  //   - enumerate pairs (U,V) such that left arg can convert
  //     (directly) to U, and right to V
  //   - pass U and V through 'rvalIsPointer': strip any reference,
  //     and insist the result be a pointer (otherwise discard pair);
  //     call resulting pair(U',V')
  //   - compute LUB(U',V'), then test with 'pointerToObject': if
  //     LUB is a pointer to object type, then use that type to
  //     instantiate the pattern; otherwise, reject the pair
  // see also: convertibility.txt

  // ------------ 13.6 para 15 ------------
  // bool operator< (T, T);
  addBuiltinBinaryOp(ST_BOOL, OP_LESS, rvalFilter, pointerOrEnum);

  // bool operator> (T, T);
  addBuiltinBinaryOp(ST_BOOL, OP_GREATER, rvalFilter, pointerOrEnum);

  // bool operator<= (T, T);
  addBuiltinBinaryOp(ST_BOOL, OP_LESSEQ, rvalFilter, pointerOrEnum);

  // bool operator>= (T, T);
  addBuiltinBinaryOp(ST_BOOL, OP_GREATEREQ, rvalFilter, pointerOrEnum);

  // ------------ 13.6 para 15 & 16 ------------
  // bool operator== (T, T);
  addBuiltinBinaryOp(ST_BOOL, OP_EQUAL, rvalFilter, pointerOrEnumOrPTM);

  // bool operator!= (T, T);
  addBuiltinBinaryOp(ST_BOOL, OP_NOTEQUAL, rvalFilter, pointerOrEnumOrPTM);

  // ------------ 13.6 para 17 ------------
  {
    Type *L = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);
    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);

    static OverloadableOp const ops[] = {
      OP_MOD,          // LR operator% (L,R);
      OP_BITAND,       // LR operator& (L,R);
      OP_BITXOR,       // LR operator^ (L,R);
      OP_BITOR,        // LR operator| (L,R);
    };
    FOREACH_OPERATOR(op, ops) {
      addBuiltinBinaryOp(ST_PRET_ARITH_CONV, op, L, R);
    }

    // L operator<< (L,R);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_LSHIFT, L, R);

    // L operator>> (L,R);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_RSHIFT, L, R);
  }

  // ------------ 13.6 para 18 ------------
  // assignment/arith to arithmetic types
  {
    Type *L = getSimpleType(SL_INIT, ST_ARITHMETIC);
    Type *Lv = getSimpleType(SL_INIT, ST_ARITHMETIC, CV_VOLATILE);

    Type *Lr = tfac.makeReferenceType(SL_INIT, L);
    Type *Lvr = tfac.makeReferenceType(SL_INIT, Lv);

    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);

    static OverloadableOp const ops[] = {
      OP_ASSIGN,       // VQ L& operator= (VQ L&, R);
      OP_MULTEQ,       // VQ L& operator*= (VQ L&, R);
      OP_DIVEQ,        // VQ L& operator/= (VQ L&, R);
      OP_PLUSEQ,       // VQ L& operator+= (VQ L&, R);
      OP_MINUSEQ       // VQ L& operator-= (VQ L&, R);
    };
    FOREACH_OPERATOR(op, ops) {
      if (lang.nonstandardAssignmentOperator && op==OP_ASSIGN) {
        // do not add this variant; see below
        continue;
      }

      addBuiltinBinaryOp(ST_PRET_FIRST, op, Lr, R);
      addBuiltinBinaryOp(ST_PRET_FIRST, op, Lvr, R);
    }
  }

  // ------------ 13.6 paras 19 and 20 ------------
  // 19: assignment to pointer type
  // T: any type
  // T* VQ & operator= (T* VQ &, T*);

  // 20: assignment to enumeration and ptr-to-member
  // T: enumeration or pointer-to-member
  // T VQ & operator= (T VQ &, T);

  // this pattern captures both:
  // T: pointer, ptr-to-member, or enumeration ('para19_20filter')
  // T VQ & operator= (T VQ &, T);
  {
    Type* (*filter)(Type *t, bool) = para19_20filter;
    if (lang.nonstandardAssignmentOperator) {
      // 9/25/04: as best I can tell, what is usually implemented is the
      //   T* VQ & operator= (T* VQ &, T*);
      // pattern where T can be pointer (para 19), enumeration (para 20),
      // pointer to member (para 20), or arithmetic (co-opted para 18)
      filter = para19_20_andArith_filter;
    }
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_ASSIGN, filter,
                       anyType, true /*isAssignment*/);
  }

  // ------------ 13.6 para 21 ------------
  // 21: +=, -= for pointer type
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_OBJ_TYPE);

    Type *Tp = tfac.makePointerType(SL_INIT, CV_NONE, T);
    Type *Tpv = tfac.makePointerType(SL_INIT, CV_VOLATILE, T);

    Type *Tpr = tfac.makeReferenceType(SL_INIT, Tp);
    Type *Tpvr = tfac.makeReferenceType(SL_INIT, Tpv);

    // T* VQ & operator+= (T* VQ &, ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_PLUSEQ, Tpr, t_ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_PLUSEQ, Tpvr, t_ptrdiff_t);

    // T* VQ & operator-= (T* VQ &, ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_MINUSEQ, Tpr, t_ptrdiff_t);
    addBuiltinBinaryOp(ST_PRET_FIRST, OP_MINUSEQ, Tpvr, t_ptrdiff_t);
  }

  // ------------ 13.6 para 22 ------------
  // 22: assignment/arith to integral type
  {
    Type *L = getSimpleType(SL_INIT, ST_INTEGRAL, CV_NONE);
    Type *Lv = getSimpleType(SL_INIT, ST_INTEGRAL, CV_VOLATILE);

    Type *Lr = tfac.makeReferenceType(SL_INIT, L);
    Type *Lvr = tfac.makeReferenceType(SL_INIT, Lv);
    
    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);

    static OverloadableOp const ops[] = {
      OP_MODEQ,        // VQ L& operator%= (VQ L&, R);
      OP_LSHIFTEQ,     // VQ L& operator<<= (VQ L&, R);
      OP_RSHIFTEQ,     // VQ L& operator>>= (VQ L&, R);
      OP_BITANDEQ,     // VQ L& operator&= (VQ L&, R);
      OP_BITXOREQ,     // VQ L& operator^= (VQ L&, R);
      OP_BITOREQ       // VQ L& operator|= (VQ L&, R);
    };
    FOREACH_OPERATOR(op, ops) {
      addBuiltinBinaryOp(ST_PRET_FIRST, op, Lr, R);
      addBuiltinBinaryOp(ST_PRET_FIRST, op, Lvr, R);
    }
  }

  // ------------ 13.6 para 23 ------------
  // bool operator! (bool);
  addBuiltinUnaryOp(ST_BOOL, OP_NOT, t_bool);

  // bool operator&& (bool, bool);
  addBuiltinBinaryOp(ST_BOOL, OP_AND, t_bool, t_bool);

  // bool operator|| (bool, bool);
  addBuiltinBinaryOp(ST_BOOL, OP_OR, t_bool, t_bool);

  // ------------ 13.6 para 24 ------------
  // 24: ?: on arithmetic types
  {
    Type *L = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);
    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);

    // NOTE: For the '?:' operator, I pretend it is binary because the
    // first argument plays no role in overload resolution, and the
    // caller will have already ensured it can be converted to 'bool'.
    // Thus, I only include information for the second and third args.

    // LR operator?(bool, L, R);
    addBuiltinBinaryOp(ST_PRET_ARITH_CONV, OP_QUESTION, L, R);
  }

  // ------------ 13.6 para 25 ------------
  // 25: ?: on pointer and ptr-to-member types
  // T operator?(bool, T, T);
  addBuiltinBinaryOp(ST_PRET_FIRST, OP_QUESTION, rvalFilter, pointerOrPTM);


  // the default constructor for ArrayStack will have allocated 10
  // items in each array, which will then have grown as necessary; go
  // back and resize them to their current length (since that won't
  // change after this point)
  for (i=0; i < NUM_OVERLOADABLE_OPS; i++) {
    builtinUnaryOperator[i].consolidate();
  }
  for (i=0; i < NUM_OVERLOADABLE_OPS; i++) {
    builtinBinaryOperator[i].consolidate();
  }
}


Env::~Env()
{
  // delete the scopes one by one, so we can skip any
  // which are in fact not owned
  while (scopes.isNotEmpty()) {
    Scope *s = scopes.removeFirst();
    if (s->curCompound || s->namespaceVar || s->isGlobalScope()) {
      // this isn't one we own
      // 8/06/04: don't delete the global scope, just leak it,
      // because I now let my Variables point at it ....
    }
    else {
      // we do own this one
      delete s;
    }
  }
  
  delete dependentScope;
}


Variable *Env::makeVariable(SourceLoc L, StringRef n, Type *t, DeclFlags f)
{
  if (!ctorFinished) {
    // the 'tunit' is NULL for the Variables introduced before analyzing
    // the user's input
    Variable *v = tfac.makeVariable(L, n, t, f, NULL);
    
    // such variables are entered into a special list, as long as
    // they're not function parameters (since parameters are reachable
    // from other made-up variables)
    if (!(f & DF_PARAMETER)) {
      madeUpVariables.push(v);
    }
    
    return v;
  }          
  
  else {
    // usual case
    return tfac.makeVariable(L, n, t, f, tunit);
  }
}


Variable *Env::declareFunctionNargs(
  Type *retType, char const *funcName,
  Type **argTypes, char const **argNames, int numArgs,
  FunctionFlags flags,
  Type * /*nullable*/ exnType)
{
  FunctionType *ft = makeFunctionType(SL_INIT, tfac.cloneType(retType));
  ft->flags |= flags;

  for (int i=0; i < numArgs; i++) {
    Variable *p = makeVariable(SL_INIT, str(argNames[i]),
                               tfac.cloneType(argTypes[i]), DF_PARAMETER);
    ft->addParam(p);
  }

  if (exnType) {
    ft->exnSpec = new FunctionType::ExnSpec;

    // slightly clever hack: say "throw()" by saying "throw(void)"
    if (!exnType->isSimple(ST_VOID)) {
      ft->exnSpec->types.append(tfac.cloneType(exnType));
    }
  }

  doneParams(ft);

  Variable *var = makeVariable(SL_INIT, str(funcName), ft, DF_NONE);
  if (flags & FF_BUILTINOP) {
    // don't add built-in operator functions to the environment
  }
  else {
    addVariable(var);
  }

  return var;
}


// this declares a function that accepts any # of arguments, and
// returns 'int'; this is for Elsa's internal testing hook functions
Variable *Env::declareSpecialFunction(char const *name)
{
  return declareFunction0arg(getSimpleType(SL_INIT, ST_INT), name, FF_VARARGS);
}


Variable *Env::declareFunction0arg(Type *retType, char const *funcName,
                                   FunctionFlags flags,
                                   Type * /*nullable*/ exnType)
{
  return declareFunctionNargs(retType, funcName,
                              NULL /*argTypes*/, NULL /*argNames*/, 0 /*numArgs*/,
                              flags, exnType);
}


Variable *Env::declareFunction1arg(Type *retType, char const *funcName,
                                   Type *arg1Type, char const *arg1Name,
                                   FunctionFlags flags,
                                   Type * /*nullable*/ exnType)
{
  return declareFunctionNargs(retType, funcName,
                              &arg1Type, &arg1Name, 1 /*numArgs*/,
                              flags, exnType);
}


Variable *Env::declareFunction2arg(Type *retType, char const *funcName,
                                   Type *arg1Type, char const *arg1Name,
                                   Type *arg2Type, char const *arg2Name,
                                   FunctionFlags flags,
                                   Type * /*nullable*/ exnType)
{
  Type *types[2] = { arg1Type, arg2Type };
  char const *names[2] = { arg1Name, arg2Name };
  return declareFunctionNargs(retType, funcName,
                              types, names, 2 /*numArgs*/,
                              flags, exnType);
}


Variable *Env::declareFunction4arg(Type *retType, char const *funcName,
                                   Type *arg1Type, char const *arg1Name,
                                   Type *arg2Type, char const *arg2Name,
                                   Type *arg3Type, char const *arg3Name,
                                   Type *arg4Type, char const *arg4Name,
                                   FunctionFlags flags,
                                   Type * /*nullable*/ exnType)
{
  Type *types[4] = { arg1Type, arg2Type, arg3Type, arg4Type };
  char const *names[4] = { arg1Name, arg2Name, arg3Name, arg4Name };
  return declareFunctionNargs(retType, funcName,
                              types, names, 4 /*numArgs*/,
                              flags, exnType);
}


FunctionType *Env::makeImplicitDeclFuncType()
{
  // don't need a clone type here as getSimpleType() calls
  // makeCVAtomicType() which in oink calls new.
  Type *ftRet = tfac.getSimpleType(loc(), ST_INT, CV_NONE);
  FunctionType *ft = makeFunctionType(loc(), ftRet);
  ft->flags |= FF_NO_PARAM_INFO;
  doneParams(ft);
  return ft;
}


Variable *Env::makeImplicitDeclFuncVar(StringRef name)
{
  return createDeclaration
    (loc(), name,
     makeImplicitDeclFuncType(), DF_FORWARD,
     globalScope(), NULL /*enclosingClass*/,
     NULL /*prior*/, NULL /*overloadSet*/);
}


FunctionType *Env::beginConstructorFunctionType(SourceLoc loc, CompoundType *ct)
{
  FunctionType *ft = makeFunctionType(loc, makeType(loc, ct));
  ft->flags |= FF_CTOR;
  // asymmetry with makeDestructorFunctionType(): this must be done by the client
//    doneParams(ft);
  return ft;
}


FunctionType *Env::makeDestructorFunctionType(SourceLoc loc, CompoundType *ct)
{
  FunctionType *ft = makeFunctionType(loc, getSimpleType(loc, ST_VOID));

  if (!ct) {
    // there's a weird case in E_fieldAcc::itcheck_x where we're making
    // a destructor type but there is no class... so just skip making
    // the receiver
  }
  else {
    ft->addReceiver(receiverParameter(loc, ct, CV_NONE));
  }

  ft->flags |= FF_DTOR;
  doneParams(ft);
  return ft;
}


Scope *Env::createScope(ScopeKind sk)
{
  return new Scope(sk, getChangeCount(), loc());
}


Scope *Env::enterScope(ScopeKind sk, char const *forWhat)
{
  // propagate the 'curFunction' field
  Function *f = scopes.first()->curFunction;
  Scope *newScope = createScope(sk);
  setParentScope(newScope);
  scopes.prepend(newScope);
  newScope->curFunction = f;

  TRACE("scope", locStr() << ": entered " << newScope->desc() << " for " << forWhat);

  // this is actually a no-op since there can't be any edges for
  // a new scope, but I do it for uniformity
  newScope->openedScope(*this);

  return newScope;
}

// set the 'parentScope' field of a scope about to be pushed
void Env::setParentScope(Scope *s)
{
  // 'parentScope' has to do with (qualified) lookup, for which the
  // template scopes are supposed to be transparent
  setParentScope(s, nonTemplateScope());
}

void Env::setParentScope(Scope *s, Scope *parent)
{
  if (parent->isPermanentScope()) {
    s->parentScope = parent;
  }
}

void Env::exitScope(Scope *s)
{
  s->closedScope();

  TRACE("scope", locStr() << ": exited " << s->desc());
  Scope *f = scopes.removeFirst();
  xassert(s == f);
  delete f;
}


void Env::extendScope(Scope *s)
{
  TRACE("scope", locStr() << ": extending " << s->desc());

  Scope *prevScope = scope();
  scopes.prepend(s);
  s->curLoc = prevScope->curLoc;

  s->openedScope(*this);
}

void Env::retractScope(Scope *s)
{
  TRACE("scope", locStr() << ": retracting " << s->desc());
       
  // don't do this here b/c I need to re-enter the scope when
  // tchecking a Function, but the association only gets established
  // once, when tchecking the declarator *name*
  #if 0
  // if we had been delegating to another scope, disconnect
  if (s->curCompound &&
      s->curCompound->parameterizingScope) {
    Scope *other = s->curCompound->parameterizingScope;
    xassert(other->parameterizedEntity->type->asCompoundType() == s->curCompound);

    TRACE("templateParams", s->desc() << " removing delegation to " << other->desc());

    other->parameterizedEntity = NULL;
    s->curCompound->parameterizingScope = NULL;
  }
  #endif // 0

  s->closedScope();

  Scope *first = scopes.removeFirst();
  xassert(first == s);
  // we don't own 's', so don't delete it
}


void Env::gdbScopes()
{
  cout << "Scopes and variables (no __-names), beginning with innermost" << endl;
  for (int i=0; i<scopes.count(); ++i) {
    Scope *s = scopes.nth(i);
    cout << "scope " << i << ", " << s->desc() << endl;
    
    if (s->isDelegated()) {
      cout << "  (DELEGATED)" << endl;
    }

    for (StringRefMap<Variable>::Iter iter = s->getVariableIter();
         !iter.isDone();
         iter.adv()) {
      // suppress __builtins, etc.
      if (prefixEquals(iter.key(), "__")) continue;

      Variable *value = iter.value();
      if (value->hasFlag(DF_NAMESPACE)) {
        cout << "  " << iter.key() << ": " << value->scope->desc() << endl;
      }
      else {
        cout << "  " << iter.key()
             << ": " << value->toString()
             << stringf(" (%p)", value);
  //        cout << "value->serialNumber " << value->serialNumber;
        cout << endl;
      }
    }

    if (s->curCompound &&
        s->curCompound->parameterizingScope) {
      cout << "  DELEGATES to " << s->curCompound->parameterizingScope->desc() << endl;
    }
  }
}


// this should be a rare event
void Env::refreshScopeOpeningEffects()
{
  TRACE("scope", "refreshScopeOpeningEffects");

  // tell all scopes they should consider themselves closed,
  // starting with the innermost
  {
    FOREACH_OBJLIST_NC(Scope, scopes, iter) {
      iter.data()->closedScope();
    }
  }

  // tell all scopes they should consider themselves opened,
  // starting with the outermost
  scopes.reverse();
  {
    FOREACH_OBJLIST_NC(Scope, scopes, iter) {
      iter.data()->openedScope(*this);
    }
  }

  // all done
  scopes.reverse();
}


#if 0   // does this even work?
CompoundType *Env::getEnclosingCompound()
{
  MUTATE_EACH_OBJLIST(Scope, scopes, iter) {
    if (iter.data()->curCompound) {
      return iter.data()->curCompound;
    }
  }
  return NULL;
}
#endif // 0


void Env::setLoc(SourceLoc loc)
{
  TRACE("loc", "setLoc: " << toString(loc));

  // only set it if it's a valid location; I get invalid locs
  // from abstract declarators..
  if (loc != SL_UNKNOWN) {
    Scope *s = scope();
    s->curLoc = loc;
  }
}

SourceLoc Env::loc() const
{
  return scopeC()->curLoc;
}


string Env::locationStackString() const
{
  stringBuilder sb;

  FOREACH_OBJLIST(Scope, scopes, iter) {
    sb << "  " << toString(iter.data()->curLoc) << "\n";
  }

  return sb;
}


string Env::instLocStackString() const
{
  // build a string that describes the instantiation context
  if (instantiationLocStack.isEmpty()) {
    return "";
  }
  else {
    stringBuilder sb;
    for (int i = instantiationLocStack.length()-1; i >= 0; i--) {
      sb << " (inst from " << instantiationLocStack[i] << ")";
    }

    return sb;
  }
}


// -------- insertion --------
Scope *Env::acceptingScope(DeclFlags df)
{
  if (lang.noInnerClasses && 
      (df & (DF_TYPEDEF | DF_ENUMERATOR))) {
    // C mode: typedefs and enumerators go into the outer scope
    return outerScope();
  }

  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();
    if (s->canAcceptNames) {
      return s;
    }
  }

  // 8/07/04: There used to be code here that asserted that an
  // accepting scope would never be more than two away from the top,
  // but that is wrong because with member templates (and in fact the
  // slightly broken way that bindings are inserted for member
  // functions of template classes) it can be arbitrarily deep.

  xfailure("could not find accepting scope");
  return NULL;    // silence warning
}


Scope *Env::outerScope()
{
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();
    if (s->isTemplateScope() ||              // skip template scopes
        s->isClassScope() ||                 // skip class scopes
        s->isParameterScope()) {             // skip parameter list scopes
      continue;
    }

    return s;
  }

  xfailure("couldn't find the outer scope!");
  return NULL;    // silence warning
}


Scope *Env::enclosingScope()
{
  // inability to accept names doesn't happen arbitrarily,
  // and we shouldn't run into it here

  Scope *s = scopes.nth(0);
  xassert(s->canAcceptNames);

  s = scopes.nth(1);
  if (!s->canAcceptNames) {
    error("did you try to make a templatized anonymous union (!), "
          "or am I confused?");
    return scopes.nth(2);   // error recovery..
  }

  return s;
}


Scope *Env::enclosingKindScope(ScopeKind k)
{
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    if (iter.data()->scopeKind == k) {
      return iter.data();
    }
  }
  return NULL;    // not found
}


CompoundType *Env::enclosingClassScope()
{
  Scope *s = enclosingKindScope(SK_CLASS);
  if (s) {
    return s->curCompound;
  }
  else {
    return NULL;
  }
}


Scope *Env::nonTemplateScope()
{
  ObjListIterNC<Scope> iter(scopes);
  while (iter.data()->scopeKind == SK_TEMPLATE_PARAMS ||
         iter.data()->scopeKind == SK_TEMPLATE_ARGS) {
    iter.adv();
  }

  return iter.data();
}

bool Env::currentScopeAboveTemplEncloses(Scope const *s)
{
  // Skip over template binding and inst-eating scopes for the
  // purpose of this check.
  //
  // We used to use somewhat more precise scope-skipping code, but
  // when a function instantiates its forward-declared instances it
  // happens to leave its parameter scope on the stack, which makes
  // perfectly precise skipping code complicated.
  return nonTemplateScope()->encloses(s);
}


bool Env::currentScopeEncloses(Scope const *s)
{
  return scope()->encloses(s);
}


Scope *Env::findEnclosingScope(Scope *target)
{
  // walk up stack of scopes, looking for first one that
  // encloses 'target'
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    if (iter.data()->encloses(target)) {
      return iter.data();
    }
  }

  xfailure("findEnclosingScope: no scope encloses target!");
  return NULL;
}


bool Env::addVariable(Variable *v, bool forceReplace)
{
  if (disambErrorsSuppressChanges()) {
    // the environment is not supposed to be modified by an ambiguous
    // alternative that fails
    TRACE("env",    "not adding variable `" << v->name
                 << "' because there are disambiguating errors");
    return true;    // don't cause further errors; pretend it worked
  }

  Scope *s = acceptingScope(v->flags);
  return addVariableToScope(s, v, forceReplace);
}

bool Env::addVariableToScope(Scope *s, Variable *v, bool forceReplace)
{
  s->registerVariable(v);
  if (s->addVariable(v, forceReplace)) {
    addedNewVariable(s, v);
    return true;
  }
  else {
    return false;
  }
}


void Env::addVariableWithOload(Variable *prevLookup, Variable *v) 
{
  if (prevLookup) {
    prevLookup->getOrCreateOverloadSet()->addMember(v);
    registerVariable(v);

    TRACE("ovl",    "overloaded `" << prevLookup->name
                 << "': `" << prevLookup->type->toString()
                 << "' and `" << v->type->toString() << "'");
  }
  else {
    if (!addVariable(v)) {
      // since the caller is responsible for passing in the result of
      // looking up v->name, if it passes NULL, then this addVariable
      // call must succeed
      xfailure("collision in addVariableWithOload");
    }
  }
}


void Env::registerVariable(Variable *v)
{
  Scope *s = acceptingScope(v->flags);
  s->registerVariable(v);
}


bool Env::addCompound(CompoundType *ct)
{
  // like above
  if (disambErrorsSuppressChanges()) {
    TRACE("env",    "not adding compound `" << ct->name
                 << "' because there are disambiguating errors");
    return true;
  }

  return typeAcceptingScope()->addCompound(ct);
}


bool Env::addEnum(EnumType *et)
{
  // like above
  if (disambErrorsSuppressChanges()) {
    TRACE("env",    "not adding enum `" << et->name
                 << "' because there are disambiguating errors");
    return true;
  }

  return typeAcceptingScope()->addEnum(et);
}


bool Env::addTypeTag(Variable *tag)
{
  // like above
  if (disambErrorsSuppressChanges()) {
    TRACE("env",    "not adding type tag `" << tag->name
                 << "' because there are disambiguating errors");
    return true;
  }

  return typeAcceptingScope()->addTypeTag(tag);
}


Type *Env::declareEnum(SourceLoc loc /*...*/, EnumType *et)
{
  Type *ret = makeType(loc, et);
  if (et->name) {
    // make the implicit typedef
    Variable *tv = makeVariable(loc, et->name, ret, DF_TYPEDEF | DF_IMPLICIT);
    et->typedefVar = tv;

    if (!addEnum(et)) {
      error(stringc << "multiply defined enum `" << et->name << "'");
    }

    if (lang.tagsAreTypes && !addVariable(tv)) {
      // this isn't really an error, because in C it would have
      // been allowed, so C++ does too [ref?]
      //return env.error(stringc
      //  << "implicit typedef associated with enum " << et->name
      //  << " conflicts with an existing typedef or variable",
      //  true /*disambiguating*/);
    }
  }

  return ret;
}


// -------- lookup --------
Scope *Env::lookupQualifiedScope(PQName const *name)
{
  bool dummy1, dummy2;
  return lookupQualifiedScope(name, dummy1, dummy2);
}

Scope *Env::lookupQualifiedScope(PQName const *name,
                                 bool &dependent, bool &anyTemplates)
{ 
  ScopeSeq scopes;
  if (!getQualifierScopes(scopes, name, dependent, anyTemplates)) {
    return NULL;
  }
  else {
    return scopes.pop();     // last one added
  }
}

                                                                 
// select either the primary or one of the specializations, based
// on the supplied arguments; these arguments will likely contain
// type variables; e.g., given "T*", select specialization C<T*>;
// return NULL if no matching definition found
Variable *Env::getPrimaryOrSpecialization
  (TemplateInfo *tinfo, ObjList<STemplateArgument> const &sargs)
{
  // check that the template scope from which the (otherwise) free
  // variables of 'sargs' are drawn is all the same scope, and
  // associate it with 'tinfo' accordingly
  
          
  // primary?
  //
  // I can't just test for 'sargs' equalling 'this->arguments',
  // because for the primary, the latter is always empty.
  //
  // So I'm just going to test that 'sargs' consists entirely
  // of toplevel type variables, which isn't exactly right ...
  bool allToplevelVars = true;
  FOREACH_OBJLIST(STemplateArgument, sargs, argIter) {
    STemplateArgument const *arg = argIter.data();

    if (arg->isType()) {
      if (!arg->getType()->isTypeVariable()) {
        allToplevelVars = false;
      }
    }
    else if (arg->isDepExpr()) {
      if (!arg->getDepExpr()->isE_variable()) {
        allToplevelVars = false;
      }
    }
  }
  if (allToplevelVars) {
    return tinfo->var;
  }

  // specialization?
  SFOREACH_OBJLIST_NC(Variable, tinfo->specializations, iter) {
    Variable *spec = iter.data();
    if (spec->templateInfo()->equalArguments(tfac, objToSObjListC(sargs))) {
      return spec;
    }
  }

  // no matching specialization
  return NULL;
}


// This returns the scope named by the qualifier portion of
// 'qualifier', or NULL if there is an error.  In the latter
// case, an error message is also inserted, unless the lookup
// failed becaue of dependence on a template parameter, in
// which case 'dependent' is set to true and no error message
// is inserted.
Scope *Env::lookupOneQualifier(
  Scope *startingScope,          // where does search begin?  NULL means current environment
  PQ_qualifier const *qualifier, // will look up the qualifier on this name
  bool &dependent,               // set to true if we have to look inside a TypeVariable
  bool &anyTemplates,            // set to true if we look in uninstantiated templates
  LookupFlags lflags)
{                                       
  // the other call site to lookupOneQualifier_useArgs has a certain
  // DF_SELFNAME processing that would have to be repeated if this
  // code is reachable
  //
  // I will delete this function when I strip out the last vestiges of
  // the old lookup mechanism; but I can't do that until
  // declarator/type-tag lookup is rewritten too
  xfailure("this function is obsolete");

  // lookup the name
  Variable *qualVar = lookupOneQualifier_bareName(startingScope, qualifier, lflags);
  if (!qualVar) {
    return NULL;     // error already reporeted
  }

  // refine using the arguments, and yield a Scope
  return lookupOneQualifier_useArgs(qualVar, qualifier->sargs,
                                    dependent, anyTemplates, lflags);
}


// lookup just the name part of a qualifier; template arguments
// (if any) are dealt with later
Variable *Env::lookupOneQualifier_bareName(
  Scope *startingScope,          // where does search begin?  NULL means current environment
  PQ_qualifier const *qualifier, // will look up the qualifier on this name
  LookupFlags lflags)
{
  // get the first qualifier
  StringRef qual = qualifier->qualifier;
  if (!qual) {      // i.e. "::" qualifier
    // should be syntactically impossible to construct bare "::"
    // with template arguments
    xassert(qualifier->sargs.isEmpty());

    // this is a reference to the global scope
    return globalScopeVar;
  }

  // look for a class called 'qual' in 'startingScope'; look in the
  // *variables*, not the *compounds*, because it is legal to make
  // a typedef which names a scope, and use that typedef'd name as
  // a qualifier
  //
  // however, this still is not quite right, see cppstd 3.4.3 para 1
  //
  // update: LF_TYPES_NAMESPACES now gets it right, I think
  lflags |= LF_TYPES_NAMESPACES | LF_SELFNAME;
  Variable *qualVar = startingScope==NULL?
    lookupVariable(qual, lflags) :
    startingScope->lookupVariable(qual, *this, lflags);
  if (!qualVar) {
    // 10/02/04: I need to suppress this error for t0329.cc.  It's not
    // yet clear whether LF_SUPPRESS_ERROR should suppress other errors
    // as well ...
    if (!( lflags & LF_SUPPRESS_ERROR )) {
      // I'd like to include some information about which scope
      // we were looking in, but I don't want to be computing
      // intermediate scope names for successful lookups; also,
      // I am still considering adding some kind of scope->name()
      // functionality, which would make this trivial.
      //
      // alternatively, I could just re-traverse the original name;
      // I'm lazy for now
      error(stringc
        << "cannot find scope name `" << qual << "'",
        EF_DISAMBIGUATES);
    }
    return NULL;
  }

  // this is what LF_TYPES_NAMESPACES means
  xassert(qualVar->hasFlag(DF_TYPEDEF) || qualVar->hasFlag(DF_NAMESPACE));

  return qualVar;
}


// Second half of 'lookupOneQualifier': use the template args.
Scope *Env::lookupOneQualifier_useArgs(
  Variable *qualVar,                // bare name scope Variable
  ObjList<STemplateArgument> const &sargs, // template args to apply
  bool &dependent,                  // set to true if we have to look inside a TypeVariable
  bool &anyTemplates,               // set to true if we look in uninstantiated templates
  LookupFlags lflags)
{
  if (!qualVar) {
    // just propagating earlier error
    return NULL;
  }

  StringRef qual = qualVar->name;

  // case 1: qualifier refers to a type
  if (qualVar->hasFlag(DF_TYPEDEF)) {
    // check for a special case: a qualifier that refers to
    // a template parameter
    if (qualVar->type->isTypeVariable() ||
        qualVar->type->isPseudoInstantiation() ||    // in/t0426.cc
        qualVar->type->isDependentQType()) {         // in/t0443.cc
      // we're looking inside an uninstantiated template parameter
      // type; the lookup fails, but no error is generated here
      dependent = true;     // tell the caller what happened
      return NULL;
    }

    if (!qualVar->type->isCompoundType()) {
      error(stringc
        << "typedef'd name `" << qual << "' doesn't refer to a class, "
        << "so it can't be used as a scope qualifier");
      return NULL;
    }
    CompoundType *ct = qualVar->type->asCompoundType();

    if (ct->isTemplate()) {
      anyTemplates = true;
    }

    // This is the same check as below in lookupPQVariable; why
    // is the mechanism duplicated?  It seems my scope lookup
    // code should be a special case of general variable lookup..
    if (qualVar->hasFlag(DF_SELFNAME) && sargs.isEmpty()) {
      // it's a reference to the class I'm in (testcase: t0168.cc)
    }

    // check template argument compatibility
    else if (sargs.isNotEmpty()) {
      if (!ct->isTemplate()) {
        error(stringc
          << "class `" << qual << "' isn't a template");
        // recovery: use the scope anyway
      }   

      else {
        if ((lflags & LF_DECLARATOR) &&
            containsVariables(sargs)) {    // fix t0185.cc?  seems to...
          // Since we're in a declarator, the template arguments are
          // being supplied for the purpose of denoting an existing
          // template primary or specialization, *not* to cause
          // instantiation.  For example, we're looking up "C<T>" in
          //
          //   template <class T>
          //   int C<T>::foo() { ... }
          //
          // So, as 'ct' is the primary, look in it to find the proper
          // specialization (or the primary).
          Variable *primaryOrSpec =
            getPrimaryOrSpecialization(ct->templateInfo(), sargs);
          if (!primaryOrSpec) {
            // I'm calling this disambiguating because I do so above,
            // when looking up just 'qual'; it's not clear that this
            // is the right thing to do.
            //
            // update: t0185.cc triggers this error.. so as a hack I'm
            // going to make it non-disambiguating, so that in
            // template code it will be ignored.  The right solution
            // is to treat member functions of template classes as
            // being independent template entities.
            //
            // Unfortunately, since this is *only* used in template
            // code, it's always masked.  Therefore, use the 'error'
            // tracing flag to see it when desired.  Hopefully a
            // proper fix for t0185.cc will land soon and we can make
            // this visible.
            //
            // 8/07/04: I think the test above, 'containsVariables',
            // has resolved this issue.
            error(stringc << "cannot find template primary or specialization `"
                          << qual << sargsToString(sargs) << "'",
                  EF_DISAMBIGUATES);
            return ct;     // recovery: use the primary
          }

          return primaryOrSpec->type->asCompoundType();
        }

        // if the template arguments are not concrete, then this is
        // a dependent name, e.g. "C<T>::foo"
        if (containsVariables(sargs)) {
          // 2005-03-07: I think this little mechanism might obviate
          // a number of DF_SELFNAME hacks running around... but I am
          // not going to try removing them just yet.
          CompoundType *t = getMatchingTemplateInScope(ct, sargs);
          if (t) {
            // this C<T> is referring to a known C<T>
            return t;
          }

          // some unknown dependent type
          dependent = true;
          return NULL;
        }

        Variable *inst = instantiateClassTemplate(loc(), qualVar, sargs);
        if (!inst) {
          return NULL;     // error already reported
        }

        // instantiate the body too, since we want to look inside it
        ensureCompleteType("use as qualifier", inst->type);

        ct = inst->type->asCompoundType();
      }
    }

    else if (ct->isTemplate()) {
      error(stringc
        << "class `" << qual
        << "' is a template, you have to supply template arguments");
      // recovery: use the scope anyway
    }

    // TODO: actually check that there are the right number
    // of arguments, of the right types, etc.

    // now that we've found it, that's our active scope
    return ct;
  }

  // case 2: qualifier refers to a namespace
  else /*DF_NAMESPACE*/ {
    if (sargs.isNotEmpty()) {
      error(stringc << "namespace `" << qual << "' can't accept template args");
    }

    // the namespace becomes the active scope
    xassert(qualVar->scope);
    return qualVar->scope;
  }
}


bool Env::getQualifierScopes(ScopeSeq &scopes, PQName const *name)
{
  bool dummy1, dummy2;
  return getQualifierScopes(scopes, name, dummy1, dummy2);
}

// 4/22/04: After now implementing and testing this code, I'm
// convinced it is wrong, because it only creates the sequence of
// scopes syntactically present.  But, both typedefs and namespace
// aliases can create ways of naming an inner scope w/o syntactically
// naming all the enclosing scopes, and hence this code would not get
// those enclosing scopes either.  I think perhaps a simple fix would
// be to dig down to the bottom scope and then traverse the
// 'parentScope' links to fill in the 'scopes' sequence.. but where to
// stop?  I'll wait until I see some failing code before exploring
// this more.
bool Env::getQualifierScopes(ScopeSeq &scopes, PQName const *name,
  bool &dependent, bool &anyTemplates)
{
  if (!name) {
    // this code evolved from code that behaved this way, and
    // there seems little harm in allowing it to continue
    return true;    // add nothing to 'scopes'
  }

  // examine all the qualifiers in sequence
  while (name->hasQualifiers()) {
    PQ_qualifier const *qualifier = name->asPQ_qualifierC();

    if (!qualifier->denotedScopeVar) {
      // error was found & reported earlier; stop and use what we have
      return false;
    }
    else if (qualifier->denotedScopeVar->hasFlag(DF_NAMESPACE)) {
      // save this namespace scope
      scopes.push(qualifier->denotedScopeVar->scope);
    }
    else if (qualifier->denotedScopeVar->type->isDependent()) {
      // report attempt to look in dependent scope
      dependent = true;
      return false;
    }
    else {
      // save this class scope
      Type *dsvType = qualifier->denotedScopeVar->type;
      xassert(dsvType->isCompoundType());
      scopes.push(dsvType->asCompoundType());
    }

    // advance to the next name in the sequence
    name = qualifier->rest;
  }

  return true;
}


void Env::extendScopeSeq(ScopeSeq const &scopes)
{
  for (int i=0; i < scopes.length(); i++) {
    extendScope(scopes[i]);
  }
}

void Env::retractScopeSeq(ScopeSeq const &scopes)
{
  // do this one backwards, to reverse the effects
  // of 'extendScopeSeq'
  for (int i = scopes.length()-1; i>=0; i--) {
    retractScope(scopes[i]);
  }
}


Variable *Env::lookupPQVariable(PQName const *name, LookupFlags flags)
{
  Scope *dummy = NULL;     // paranoia; 'scope' is an OUT parameter
  return lookupPQVariable(name, flags, dummy);
}

Variable *Env::lookupPQVariable_set
  (LookupSet &candidates, PQName const *name, LookupFlags flags)
{
  Scope *dummy;
  return lookupPQVariable_internal(candidates, name, flags, dummy);
}

// NOTE: It is *not* the job of this function to do overload
// resolution!  If the client wants that done, it must do it itself,
// *after* doing the lookup.
Variable *Env::lookupPQVariable_internal
  (LookupSet &candidates, PQName const *name, LookupFlags flags, Scope *&scope)
{
  // get the final component of the name, so we can inspect
  // whether it has template arguments
  PQName const *final = name->getUnqualifiedNameC();

  Variable *var;

  if (name->hasQualifiers()) {
    // TODO: I am now doing the scope lookup twice, once while
    // tchecking 'name' and once here.  Replace what's here with code
    // that re-uses the work already done.

    // look up the scope named by the qualifiers
    bool dependent = false, anyTemplates = false;
    scope = lookupQualifiedScope(name, dependent, anyTemplates);
    if (!scope) {
      if (dependent) {
        TRACE("dependent", name->toString());

        // tried to look into a template parameter
        if (flags & (LF_TYPENAME | LF_ONLY_TYPES)) {
          return dependentTypeVar;    // user claims it's a type
        }
        else {
          return dependentVar;        // user makes no claim, so it's a variable
        }
      }
      else if (anyTemplates) {
        // maybe failed due to incompleteness of specialization implementation
        return errorVar;
      }
      else {
        // error has already been reported
        return NULL;
      }
    }

    var = scope->lookupVariable_set(candidates, name->getName(), 
                                    *this, flags | LF_QUALIFIED);
    if (!var) {
      if (anyTemplates) {
        // maybe failed due to incompleteness of specialization implementation;
        // since I'm returning an error variable, it means I'm guessing that
        // this thing names a variable and not a type
        return errorVar;
      }

      error(stringc
        << name->qualifierString() << " has no member called `"
        << name->getName() << "'",
        EF_NONE);
      return NULL;
    }
  }

  else {
    // 14.6.1 para 1: if the name is not qualified, and has no
    // template arguments, then the selfname is visible
    //
    // ... I'm not sure about the "not qualified" part ... is there
    // something like 14.6.1 for non-template classes?  If so I can't
    // find it.. anyway, my t0052.cc (which gcc doesn't like..) wants
    // LF_SELFNAME even for qualified lookup, and that's how it's been
    // for a while, so...
    //
    // err... t0052.cc still doesn't work with this above, so now
    // I'm back to requiring no qualifiers and I nerfed that testcase.
    //
    // 8/13/04: For what it's worth, 9 para 2 is selfname for
    // non-templates, and it seems to suggest that such selfnames
    // *can* be found by qualified lookup...
    if (!final->isPQ_template()) {
      flags |= LF_SELFNAME;
    }

    var = lookupVariable_set(candidates, name->getName(), flags, scope);
  }

  // apply template arguments in 'name'
  return applyPQNameTemplateArguments(var, final, flags);
}

// given something like "C<T>", where "C" has been looked up to yield
// 'var' and "<T>" is still attached to 'final', combine them
Variable *Env::applyPQNameTemplateArguments
  (Variable *var,         // (nullable) may or may not name a template primary
   PQName const *final,   // final part of name, may or may not have template arguments
   LookupFlags flags)
{
  if (!var) {
    // caller already had an error and reported it; just propagate
    return NULL;
  }

  // reference to a class in whose scope I am?
  if (var->hasFlag(DF_SELFNAME)) {
    xassert(!final->isPQ_template());    // otherwise how'd LF_SELFNAME get set?

    // cppstd 14.6.1 para 1: if the name refers to a template
    // in whose scope we are, then it need not have arguments
    TRACE("template", "found bare reference to enclosing template: " << var->name);
    return var;
  }

  // 2005-02-18: Do this regardless of whether 'var->isTemplate()',
  // since it could be overloaded, making that test meaningless.
  // See, e.g., in/t0372.cc.
  if (flags & LF_TEMPL_PRIMARY) {
    return var;
  }

  // compare the name's template status to whether template
  // arguments were supplied; note that you're only *required*
  // to pass arguments for template classes, since template
  // functions can infer their arguments
  if (var->isTemplate(false /*considerInherited*/)) {
    if (!final->isPQ_template()
        && var->isTemplateClass() // can be inferred for template functions, so we duck
        ) {
      // this disambiguates
      //   new Foo< 3 > +4 > +5;
      // which could absorb the '3' as a template argument or not,
      // depending on whether Foo is a template
      error(stringc
        << "`" << var->name << "' is a class template, but template "
        << "arguments were not supplied",
        EF_DISAMBIGUATES);
      return lookupErrorObject(flags);
    }

    if (var->isTemplateClass()) {
      // apply the template arguments to yield a new type based
      // on the template

      // if the template arguments are not concrete, then create
      // a PseudoInstantiation
      PQ_template const *tqual = final->asPQ_templateC();
      if (containsVariables(tqual->sargs)) {
        PseudoInstantiation *pi =
          createPseudoInstantiation(var->type->asCompoundType(), tqual->sargs);
        xassert(pi->typedefVar);
        return pi->typedefVar;
      }

      return instantiateClassTemplate(loc(), var, tqual->sargs);
    }
    else {                    // template function
      xassert(var->isTemplateFunction());

      // it's quite likely that this code is not right...

      if (!final->isPQ_template()) {
        // no template arguments supplied... just return the primary
        return var;
      }
      else {
        // hope that all of the arguments have been supplied
        xassert(var->templateInfo()->isPrimary());
        return instantiateFunctionTemplate(loc(), var, final->asPQ_templateC()->sargs);
      }
    }
  }
  else if (!var->isTemplate() &&
           final->isPQ_template()) {
    // disambiguates the same example as above, but selects
    // the opposite interpretation
    error(stringc
      << "`" << var->name << "' is not a template, but template arguments were supplied",
      EF_DISAMBIGUATES);
    return NULL;
  }

  return var;
}

Variable *Env::lookupPQVariable(PQName const *name, LookupFlags flags,
                                Scope *&scope)
{                                          
  LookupSet dummy;
  Variable *var = lookupPQVariable_internal(dummy, name, flags, scope);

  // in normal typechecking, nothing should look up to a variable from
  // template-definition never-never land
  if ( !(flags & LF_TEMPL_PRIMARY) &&
       var &&
       !var->hasFlag(DF_NAMESPACE)
       ) {
    xassert(var->type);
    // FIX: I think this assertion should never fail, but it fails in
    // in/t0079.cc for reasons that seem to have something to do with
    // DF_SELFNAME
    //      xassert(!var->type->containsVariables());
  }

  return var;
}

Variable *Env::lookupVariable(StringRef name, LookupFlags flags)
{
  Scope *dummy;
  return lookupVariable(name, flags, dummy);
}

Variable *Env::lookupVariable(StringRef name, LookupFlags flags,
                              Scope *&foundScope)
{
  LookupSet candidates;
  return lookupVariable_set(candidates, name, flags, foundScope);
}

Variable *Env::lookupVariable_set(LookupSet &candidates,
                                  StringRef name, LookupFlags flags,
                                  Scope *&foundScope)
{
  if (flags & LF_INNER_ONLY) {
    // here as in every other place 'innerOnly' is true, I have
    // to skip non-accepting scopes since that's not where the
    // client is planning to put the name
    foundScope = acceptingScope();
    return foundScope->lookupVariable_set(candidates, name, *this, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();
    if ((flags & LF_SKIP_CLASSES) && s->isClassScope()) {
      continue;
    }   
    
    if (s->isDelegated()) {
      // though 's' appears physically here, it is searched in a different order
      continue;
    }

    // look in 's'
    Variable *v = s->lookupVariable_set(candidates, name, *this, flags);
    if (v) {
      foundScope = s;
      return v;
    }

    // is 's' connected to a delegated scope?
    Scope *delegated = s->getDelegationPointer();
    if (delegated) {
      v = delegated->lookupVariable_set(candidates, name, *this, flags);
      if (v) {
        foundScope = s;     // not 'delegated'
        return v;
      }
    }
  }
  return NULL;    // not found
}

CompoundType *Env::lookupPQCompound(PQName const *name, LookupFlags flags)
{
  CompoundType *ret;
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    ret = scope->lookupCompound(name->getName(), env, flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no class/struct/union called `"
        << name->getName() << "'",
        EF_DISAMBIGUATES);
      return NULL;
    }
  }
  else {
    ret = lookupCompound(name->getName(), flags);
  }

  // apply template arguments if any
  if (ret) {
    PQName const *final = name->getUnqualifiedNameC();
    Variable *var = applyPQNameTemplateArguments(ret->typedefVar, final, flags);
    if (var && var->type->isCompoundType()) {
      ret = var->type->asCompoundType();
    }
  }

  return ret;
}

CompoundType *Env::lookupCompound(StringRef name, LookupFlags flags)
{
  if (flags & LF_INNER_ONLY) {
    // 10/17/04: dsw/sm: Pass DF_TYPEDEF here so that in C mode we
    // will be looking at 'outerScope'.
    return acceptingScope(DF_TYPEDEF)->lookupCompound(name, env, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    CompoundType *ct = iter.data()->lookupCompound(name, env, flags);
    if (ct) {
      return ct;
    }
  }
  return NULL;    // not found
}

EnumType *Env::lookupPQEnum(PQName const *name, LookupFlags flags)
{
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    EnumType *ret = scope->lookupEnum(name->getName(), *this, flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no enum called `"
        << name->getName() << "'",
        EF_DISAMBIGUATES);
      return NULL;
    }

    return ret;
  }

  return lookupEnum(name->getName(), flags);
}

EnumType *Env::lookupEnum(StringRef name, LookupFlags flags)
{
  if (flags & LF_INNER_ONLY) {
    return acceptingScope()->lookupEnum(name, *this, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    EnumType *et = iter.data()->lookupEnum(name, *this, flags);
    if (et) {
      return et;
    }
  }
  return NULL;    // not found
}


StringRef Env::getAnonName(TypeIntr keyword)
{
  // construct a name
  string name = stringc << "Anon_" << toString(keyword) 
                        << "_" << anonTypeCounter++;
                     
  // map to a stringref
  StringRef sr = str(name);

  // any chance this name already exists?
  if (lookupVariable(sr)) {
    return getAnonName(keyword);    // try again
  }
  else {
    return sr;
  }
}


TemplateInfo * /*owner*/ Env::takeFTemplateInfo()
{
  // for now, difference is that function TemplateInfos have
  // NULL baseNames        
  //
  // and in fact that difference is now gone too...
  return takeCTemplateInfo();
}

TemplateInfo * /*owner*/ Env::takeCTemplateInfo()
{
  // delay building a TemplateInfo until it is sure to be needed
  TemplateInfo *ret = NULL;

  // could this be a member of complete specialization class?
  // if I do nothing it will get a degenerate TemplateInfo, so
  // I need to detect that situation and throw the tinfo away
  bool couldBeCompleteSpecMember = true;

  // do the enclosing scopes other than the closest have params?
  // if so, find and attach all inherited parameter lists
  ObjListIter<Scope> iter(scopes);
  for (; !iter.isDone(); iter.adv()) {
    Scope const *s = iter.data();

    // template parameters are only inherited across some kinds of
    // scopes (this is a little bit of a hack as I try to guess the
    // right rules)
    if (!( s->scopeKind == SK_CLASS ||
           s->scopeKind == SK_NAMESPACE ||
           s->scopeKind == SK_TEMPLATE_PARAMS )) {
      break;
    }

    if (s->isTemplateParamScope()) {
      // found some parameters that we need to record in a TemplateInfo
      if (!ret) {
        ret = new TemplateInfo(loc());
      }

      // are these as-yet unassociated params?
      if (!s->parameterizedEntity) {
        if (ret->params.isNotEmpty()) {
          // there was already one unassociated, and now we see another
          error("too many template <...> declarations");
        }
        else {
          ret->params.appendAll(s->templateParams);
          couldBeCompleteSpecMember = false;

          TRACE("templateParams", "main params: " << ret->paramsToCString());
        }
      }

      else {
        // record info about these params and where they come from
        InheritedTemplateParams *itp
          = new InheritedTemplateParams(s->parameterizedEntity->type->asCompoundType());
        itp->params.appendAll(s->templateParams);

        if (s->templateParams.isNotEmpty()) {
          couldBeCompleteSpecMember = false;
        }

        // stash them in 'ret', prepending so as we work from innermost
        // to outermost, so the last one prepended will be the outermost,
        // and hence first in the list when we're done
        ret->inheritedParams.prepend(itp);

        TRACE("templateParams", "inherited " << itp->paramsToCString() <<
                                " from " << s->parameterizedEntity->name);
      }
    }
  }

  if (ret && couldBeCompleteSpecMember) {
    // throw away this tinfo; the thing is fully concrete and
    // not a specialization of any template
    //
    // TODO: I need a testcase for this in Elsa.. right now there
    // is only an Oink testcase.
    TRACE("templateParams", "discarding TemplateInfo for member of complete specialization");
    delete ret;
    ret = NULL;
  }

  return ret;
}


Type *Env::makeNewCompound(CompoundType *&ct, Scope * /*nullable*/ scope,
                           StringRef name, SourceLoc loc,
                           TypeIntr keyword, bool forward)
{
  // make the type itself
  ct = tfac.makeCompoundType((CompoundType::Keyword)keyword, name);
  if (scope) {
    setParentScope(ct, scope);
  }

  // make the implicit typedef
  Type *ret = makeType(loc, ct);
  Variable *tv = makeVariable(loc, name, ret, DF_TYPEDEF | DF_IMPLICIT);
  ct->typedefVar = tv;

  // add the tag to the scope
  ct->forward = forward;
  if (name && scope) {
    bool ok = scope->addCompound(ct);
    xassert(ok);     // already checked that it was ok
  }

  // cppstd section 9, para 2: add to class' scope itself, too
  #if 0
  // It turns out this causes infinite loops elsewhere, and is in
  // fact unnecessary, because any lookup that would find 'ct' in
  // 'ct' itself would *also* find it in 'scope', since in the
  // space of compound types we can't get the kind of hiding that
  // can occur in the variable namespace.
  if (name) {
    bool ok = ct->addCompound(ct);
    xassert(ok);
  }
  #endif // 0

  // transfer template parameters
  { 
    TemplateInfo *ti = takeCTemplateInfo();
    if (ti) {
      ct->setTemplateInfo(ti);
      
      // it turns out any time I pass in a non-NULL scope, I'm in
      // the process of making a primary
      if (scope) {
        if (this->scope()->isTemplateParamScope()) {
          this->scope()->setParameterizedEntity(ct->typedefVar);
        }

        // set its defnScope, which is used during instantiation
        ti->defnScope = scope;
      }
    }
  }

  if (name && scope) {
    scope->registerVariable(tv);
    if (lang.tagsAreTypes) {
      if (!scope->addVariable(tv)) {
        // this isn't really an error, because in C it would have
        // been allowed, so C++ does too [ref?]
        //return env.error(stringc
        //  << "implicit typedef associated with " << ct->keywordAndName()
        //  << " conflicts with an existing typedef or variable",
        //  true /*disambiguating*/);
      }
      else {
        addedNewVariable(scope, tv);
      }
    }
    else {
      // dsw: I found that it interfered to put the implicit typedef
      // into the space in C, and as far as I understand, it doesn't
      // exist in C anyway.  See in/c/dC0012.c
      //
      // sm: yes, that is right
    }
  }

  // also add the typedef to the class' scope
  if (name && lang.compoundSelfName) {
    if (tv->templateInfo() && tv->templateInfo()->hasParameters()) {
      // the self type should be a PseudoInstantiation, not the raw template
      //                                                             
      // 2005-03-05: This always fills in arguments corresponding to
      // the template primary.  If this is a specialization, the caller
      // will modify the selfname later.
      ct->selfType = pseudoSelfInstantiation(ct, CV_NONE);
    }
    else {
      // just the class' type
      ct->selfType = tfac.cloneType(ret);
    }
    Variable *selfVar = makeVariable(loc, name, ct->selfType,
                                     DF_TYPEDEF | DF_SELFNAME);
    ct->addUniqueVariable(selfVar);
    addedNewVariable(ct, selfVar);
  }

  return ret;
}


bool Env::setDisambiguateOnly(bool newVal)
{
  bool ret = disambiguateOnly;
  disambiguateOnly = newVal;
  return ret;
}


Type *Env::implicitReceiverType()
{
  Variable *receiver = lookupVariable(receiverName);
  if (!receiver) {
    return NULL;
  }
  else {
    return receiver->type;
  }
}


Variable *Env::receiverParameter(SourceLoc loc, NamedAtomicType *nat, CVFlags cv,
                                 D_func *syntax)
{
  // For templatized classes, do something a little different.  It's
  // unfortunate that whether we call into tfac depends on whether 'nat'
  // is a template class.  I think 'makeTypeOf_receiver' should simply
  // be removed from the tfac.
  Type *recType;
  if (nat->isCompoundType() &&
      nat->typedefVar->isTemplate()) {
    // get the basic selfType; we re-use the one from the CompoundType
    // instead of calling Env::pseudoSelfInstantiation because we want
    // all the receiver parameters of methods in the uninstantiated
    // class to be the same (in/t0410.cc)
    Type *selfType = nat->asCompoundType()->selfType;
    xassert(selfType);
    
    // apply 'cv'
    selfType = tfac.applyCVToType(SL_UNKNOWN, cv, selfType, NULL /*syntax*/);

    // build a reference
    recType = makeReferenceType(loc, selfType);
  }
  else {
    // this doesn't use 'implicitReceiverType' because of the warnings above
    recType = tfac.makeTypeOf_receiver(loc, nat, cv, syntax);
  }

  return makeVariable(loc, receiverName, recType, DF_PARAMETER);
}


// cppstd 5 para 8
Type *Env::operandRval(Type *t)
{
  // 4.1: lval to rval
  if (t->isReferenceType()) {
    t = t->asRval();

    // non-compounds have their constness stripped at this point too,
    // but I think that would not be observable in my implementation
  }
  
  // 4.2: array to pointer
  if (t->isArrayType()) {
    t = makePointerType(SL_UNKNOWN, CV_NONE, t->getAtType());
  }

  // 4.2: function to pointer
  if (t->isFunctionType()) {
    t = makePointerType(SL_UNKNOWN, CV_NONE, t);
  }

  return t;
}


// Get the typeid type (5.2.8, 18.5.1).  The program is required to
// #include <typeinfo> before using it, so we don't need to define
// this class (since that header does).
Type *Env::type_info_const_ref()
{
  // does the 'std' namespace exist?
  Scope *scope = globalScope();
  Variable *stdNS = scope->lookupVariable(str("std"), *this);
  if (stdNS && stdNS->isNamespace()) {
    // use that instead of the global scope
    scope = stdNS->scope;
  }

  // look for 'type_info'
  Variable *ti = scope->lookupVariable(str("type_info"), *this, LF_ONLY_TYPES);
  if (ti && ti->hasFlag(DF_TYPEDEF)) {
    // make a const reference
    return makeReferenceType(loc(),
             tfac.applyCVToType(loc(), CV_CONST, ti->type, NULL /*syntax*/));
  }
  else {
    return error("must #include <typeinfo> before using typeid");
  }
}


void Env::addBuiltinUnaryOp(SimpleTypeId retId, OverloadableOp op, Type *x)
{
  Type *retType = getSimpleType(SL_INIT, retId);
  builtinUnaryOperator[op].push(createBuiltinUnaryOp(retType, op, x));
}

Variable *Env::createBuiltinUnaryOp(Type *retType, OverloadableOp op, Type *x)
{
  Variable *v = declareFunction1arg(
    retType, operatorName[op],
    x, "x",
    FF_BUILTINOP);
  v->setFlag(DF_BUILTIN);

  return v;
}


void Env::addBuiltinBinaryOp(SimpleTypeId retId, OverloadableOp op,
                             Type *x, Type *y)
{
  Type *retType = getSimpleType(SL_INIT, retId);
  addBuiltinBinaryOp(op, new PolymorphicCandidateSet(
    createBuiltinBinaryOp(retType, op, x, y)));
}

void Env::addBuiltinBinaryOp(SimpleTypeId retId, OverloadableOp op,
                             PredicateCandidateSet::PreFilter pre,
                             PredicateCandidateSet::PostFilter post,
                             bool isAssignment)
{
  if (isAssignment) {
    addBuiltinBinaryOp(op, new AssignmentCandidateSet(retId, pre, post));
  }
  else {
    addBuiltinBinaryOp(op, new PredicateCandidateSet(retId, pre, post));
  }
}

void Env::addBuiltinBinaryOp(OverloadableOp op, CandidateSet * /*owner*/ cset)
{
  builtinBinaryOperator[op].push(cset);
}


Variable *Env::createBuiltinBinaryOp(Type *retType, OverloadableOp op,
                                     Type *x, Type *y)
{
  // PLAN:  Right now, I just leak a bunch of things.  To fix
  // this, I want to have the Env maintain a pool of Variables
  // that represent built-in operators during overload
  // resolution.  I ask the Env for operator-(T,T) with a
  // specific T, and it rewrites an existing one from the pool
  // for me.  Later I give all the pool elements back.

  Variable *v = declareFunction2arg(
    retType, operatorName[op],
    x, "x", y, "y",
    FF_BUILTINOP);
  v->setFlag(DF_BUILTIN);

  return v;
}


// comparing types for equality, *except* we allow array types
// to match even when one of them is missing a bound and the
// other is not; I cannot find where in the C++ standard this
// exception is specified, so I'm just guessing about where to
// apply it and exactly what the rule should be
//
// note that this is *not* the same rule that allows array types in
// function parameters to vary similarly, see
// 'normalizeParameterType()'
//
// Like 'equalOrIsomorphic', I've weakened constness ...
bool Env::almostEqualTypes(Type /*const*/ *t1, Type /*const*/ *t2,
                           Type::EqFlags eqFlags)
{
  if (t1->isArrayType() &&
      t2->isArrayType()) {
    ArrayType /*const*/ *at1 = t1->asArrayType();
    ArrayType /*const*/ *at2 = t2->asArrayType();

    if ((at1->hasSize() && !at2->hasSize()) ||
        (at2->hasSize() && !at1->hasSize())) {
      // the exception kicks in
      return at1->eltType->equals(at2->eltType);
    }
  }

  // strict equality (well, toplevel param cv can differ)
  eqFlags |= Type::EF_IGNORE_PARAM_CV;
    
  return equivalentTypes(t1, t2, eqFlags);
}


// check if multiple definitions of a global symbol are ok; also
// updates some data structures so that future checks can be made
bool multipleDefinitionsOK(Env &env, Variable *prior, DeclFlags dflags)
{
  if (!env.lang.uninitializedGlobalDataIsCommon) {
    return false;
  }

  // dsw: I think the "common symbol exception" only applies to
  // globals.
  if (!prior->hasFlag(DF_GLOBAL)) {
    return false;
  }

  // can't be initialized more than once
  if (dflags & DF_INITIALIZED) {
    if (prior->hasFlag(DF_INITIALIZED)) {
      return false; // can't both be initialized
    }
    else {
      prior->setFlag(DF_INITIALIZED); // update for future reference
    }
  }
  
  // don't allow this for functions!
  if (prior->type->isFunctionType()) {
    return false;
  }

  return true;
}


// little hack: Variables don't store pointers to the global scope,
// they store NULL instead, so canonize the pointers by changing
// global scope pointers to NULL before comparison
//
// dsw: 8/10/04: the above is no longer true, so this function is
// now trivial
bool sameScopes(Scope *s1, Scope *s2)
{
  return s1 == s2;
}


void Env::makeUsingAliasFor(SourceLoc loc, Variable *origVar)
{
  Type *type = origVar->type;
  StringRef name = origVar->name;
  Scope *scope = this->scope();
  CompoundType *enclosingClass = scope->curCompound;

  // TODO: check that 'origVar' is accessible (w.r.t. access control)
  // in the current context

  // 7.3.3 paras 4 and 6: the original and alias must either both
  // be class members or neither is class member
  if (scope->isClassScope() !=
      (origVar->scope? origVar->scope->isClassScope() : false)) {
    error(stringc << "bad alias `" << name
                  << "': alias and original must both be class members or both not members");
    return;
  }

  if (scope->isClassScope()) {
    // 7.3.3 para 13: overload resolution for imported class members
    // needs to treat them as accepting a derived-class receiver argument,
    // so we'll make the alias have a type consistent with its new home
    if (type->isMethod()) {
      FunctionType *oldFt = type->asFunctionType();

      // everything the same but empty parameter list
      FunctionType *newFt =
        tfac.makeSimilarFunctionType(SL_UNKNOWN, oldFt->retType, oldFt);

      // add the receiver parameter
      CompoundType *ct = scope->curCompound;
      newFt->addReceiver(receiverParameter(SL_UNKNOWN, ct, oldFt->getReceiverCV()));

      // copy the other parameters
      SObjListIterNC<Variable> iter(oldFt->params);
      iter.adv();     // skip oldFt's receiver
      for (; !iter.isDone(); iter.adv()) {
        newFt->addParam(iter.data());    // share parameter objects
      }
      doneParams(newFt);

      // treat this new type as the one to declare from here out
      type = newFt;
    }

    // 7.3.3 para 4: the original member must be in a base class of
    // the class where the alias is put
    if (!enclosingClass->hasBaseClass(origVar->scope->curCompound)) {
      error(stringc << "bad alias `" << name
                    << "': original must be in a base class of alias' scope");
      return;
    }
  }

  // check for existing declarations
  Variable *prior = lookupVariableForDeclaration(scope, name, type,
    type->isFunctionType()? type->asFunctionType()->getReceiverCV() : CV_NONE);

  // 10/20/04: 7.3.3 para 8 says duplicate aliases are allowed wherever
  // duplicate declarations are normally allowed.  I don't quite understand
  // where they are normally allowed; the example suggests they are ok
  // at global/namespace scope, so that is what I will allow...
  // (in/d0109.cc) (in/t0289.cc) (in/t0161.cc) (in/std/7.3.3f.cc)
  if (prior &&
      prior->getUsingAlias() == origVar &&
      (scope->isGlobalScope() || scope->isNamespace())) {
    return;
  }

  // check for overloading
  OverloadSet *overloadSet = getOverloadForDeclaration(prior, type);

  // are we heading for a conflict with an alias?
  if (prior &&
      prior->getUsingAlias() &&
      !prior->hasFlag(DF_TYPEDEF) &&
      !sameScopes(prior->skipAlias()->scope, scope)) {
    // 7.3.3 para 11 says it's ok for two aliases to conflict when
    // not in class scope; I assume they have to be functions for
    // this to be allowed
    if (!enclosingClass &&
        prior->type->isFunctionType()) {
      // turn it into an overloading situation
      overloadSet = prior->getOrCreateOverloadSet();
      prior = NULL;
    }

    // if we are in class scope, call it an error now; that way I know
    // down in 'createDeclaration' that if 'prior' is an alias, then
    // we are *not* trying to create another alias
    else if (enclosingClass) {
      // this error message could be more informative...
      error(stringc << "alias `" << name << "' conflicts with another alias");
      return;
    }
  }

  // make new declaration, taking to account various restrictions
  Variable *newVar = createDeclaration(loc, name, type, origVar->flags,
                                       scope, enclosingClass, prior, overloadSet);
  #if 0     // 2005-02-23: share the referent's templateInfo
  if (origVar->templateInfo()) {
    newVar->setTemplateInfo(new TemplateInfo(*origVar->templateInfo()));
  }
  #endif // 0

  if (newVar == prior) {
    // found that the name/type named an equivalent entity
    return;
  }

  // hook 'newVar' up as an alias for 'origVar'
  newVar->setUsingAlias(origVar->skipAlias());    // don't make long chains

  TRACE("env", "made alias `" << name <<
               "' for origVar at " << toString(origVar->loc));
}


// lookup in inner, plus signature matching in overload set; the
// const/volatile flags on the receiver parameter are given by this_cv
// since in D_name_tcheck they aren't yet attached to the type (and it
// isn't easy to do so because of ordering issues)
Variable *Env::lookupVariableForDeclaration
  (Scope *scope, StringRef name, Type *type, CVFlags this_cv)
{
  // does this name already have a declaration in this scope?
  //
  // 8/13/04: It seems like I need LF_SELFNAME (d0087.cc).  I think
  // the callers generally have names w/o template arguments..
  Variable *prior = scope->lookupVariable(name, *this, LF_INNER_ONLY | LF_SELFNAME);
  if (prior &&
      prior->overload &&
      type->isFunctionType()) {
    // set 'prior' to the previously-declared member that has
    // the same signature, if one exists
    FunctionType *ft = type->asFunctionType();
    Variable *match = findInOverloadSet(prior->overload, ft, this_cv);
    if (match) {
      prior = match;
    }
  }

  return prior;
}


// Search in an overload set for a specific element, given its type.
// This is like the obsolete OverloadSet::findByType, except it works
// when elements of the set are templatized.
Variable *Env::findInOverloadSet(OverloadSet *oset,
                                 FunctionType *ft, CVFlags receiverCV)
{
  SFOREACH_OBJLIST_NC(Variable, oset->set, iter) {
    FunctionType *iterft = iter.data()->type->asFunctionType();

    // check the parameters other than '__receiver'
    Type::EqFlags eflags = Type::EF_STAT_EQ_NONSTAT | Type::EF_IGNORE_IMPLICIT;

    if (inUninstTemplate()) {
      // do not check the return types; we can diagnose a mismatch
      // when the template is instantiated, and it is sometimes
      // hard to tell when they match (in/t0290.cc)
      eflags |= Type::EF_IGNORE_RETURN;
    }

    if (!equivalentTypes(iterft, ft, eflags)) {
      continue;    // not the one we want
    }

    // if 'this' exists, it must match 'receiverCV'
    if (iterft->getReceiverCV() != receiverCV) continue;

    // ok, this is the right one
    return iter.data();
  }
  return NULL;    // not found
}

Variable *Env::findInOverloadSet(OverloadSet *oset, FunctionType *ft)
{
  return findInOverloadSet(oset, ft, ft->getReceiverCV());
}


// true if two function types have equivalent signatures, meaning
// if their names are the same then they refer to the same function,
// not two overloaded instances  
bool Env::equivalentSignatures(FunctionType *ft1, FunctionType *ft2)
{   
  return equivalentTypes(ft1, ft2, Type::EF_SIGNATURE);
}


#if 0     // not needed right now
class ContainsUTP : public TypeVisitor {
public:
  bool anyUTP;

public:
  ContainsUTP() : anyUTP(false) {}
  virtual bool visitAtomicType(AtomicType *obj);
};

bool ContainsUTP::visitAtomicType(AtomicType *obj)
{
  if (obj->isTypeVariable() &&
      !obj->asTypeVariableC()->isAssociated()) {
    anyUTP = true;
    return false;     // no need to check children
  }
  return true;
}

bool containsUnassociatedTemplateParams(Type *t)
{
  ContainsUTP cutp;
  t->traverse(cutp);
  return cutp.anyUTP;
}
#endif // 0

// 2005-03-03: This function is taking over some of the duties of
// 'equalOrIsomorphic', in particular those that involve checking
// function signature equivalence for the purpose of matching
// declarations and definitions.  It distinguishes template parameters
// by whether they have been associated with a template: those that
// have been associated are essentially regarded as concrete, while
// those that have not are regarded as arbitrary type variables, and
// therefore subject to unification by MatchTypes.
bool Env::equivalentTypes(Type *a, Type *b, Type::EqFlags eflags)
{        
  // In the future, I may be able to restrict using MatchTypes to just
  // those cases that have unassociated tparams; but right now, in
  // addition to doing matching per se, MatchTypes is implementing
  // somewhat more liberal equality rules (particularly for
  // ST_DEPENDENT), which I need (e.g., for in/t0268.cc).  Therefore I
  // will continue to use it whenever there are any tparams,
  // regardless of whether they are associated or not.
  #if 0
    bool aVars = containsUnassociatedTemplateParams(a);
    bool bVars = containsUnassociatedTemplateParams(b);
  #else
    bool aVars = a->containsVariables();
    bool bVars = b->containsVariables();
  #endif

  if (!aVars && !bVars) {
    // normal case
    return a->equals(b, eflags);
  }
  else {
    // the 'a' type refers to the already-existing function template
    // declaration, wherein the parameters *have* been associated, and
    // the 'b' type refers to the new declaration we are trying to
    // match up, so its parameters have *not* been associated
    MatchTypes match(tfac, MatchTypes::MM_ISO, eflags | Type::EF_UNASSOC_TPARAMS);
    return match.match_Type(a, b);
  }
}


// if the types are concrete, compare for equality; if at least one
// is not, compare for isomorphism
//
// The parameters here should be const, but MatchTypes doesn't
// make claims about constness so I just dropped const from here ...
bool equalOrIsomorphic(TypeFactory &tfac, Type *a, Type *b, Type::EqFlags eflags)
{
  bool aVars = a->containsVariables();
  bool bVars = b->containsVariables();

  if (!aVars && !bVars) {
    // normal case: concrete signature comparison
    return a->equals(b, eflags);
  }
  else {
    // non-concrete comparison
    //
    // 8/07/04: I had been saying that if aVars!=bVars then the
    // types aren't equivalent, but that is wrong if we are skipping
    // the receiver parameter and that's the place that one has
    // a type var and the other does not.  (t0028.cc)
    MatchTypes match(tfac, MatchTypes::MM_ISO, eflags);
    return match.match_Type(a, b);
  }
}


// if 'prior' refers to an overloaded set, and 'type' could be the type
// of a new (not existing) member of that set, then return that set
// and nullify 'prior'; otherwise return NULL
OverloadSet *Env::getOverloadForDeclaration(Variable *&prior, Type *type)
{
  OverloadSet *overloadSet = NULL;    // null until valid overload seen
  if (lang.allowOverloading &&
      prior &&
      prior->type->isFunctionType() &&
      type->isFunctionType()) {
    // potential overloading situation
    
    // get the two function types
    FunctionType *priorFt = prior->type->asFunctionType();
    FunctionType *specFt = type->asFunctionType();
    bool sameType = 
      (prior->name == conversionOperatorName?
        equivalentTypes(priorFt, specFt) :       // conversion: equality check
        equivalentSignatures(priorFt, specFt));  // non-conversion: signature check

    // figure out if either is (or will be) a template
    bool priorIsTemplate = prior->templateInfo() &&
                           prior->templateInfo()->hasParameters();
    bool thisIsTemplate = scope()->isTemplateParamScope();
    bool sameTemplateNess = (priorIsTemplate == thisIsTemplate);

    // can only be an overloading if their signatures differ
    if (!sameType || !sameTemplateNess) {
      // ok, allow the overload
      TRACE("ovl",    "overloaded `" << prior->name
                   << "': `" << prior->type->toString()
                   << "' and `" << type->toString() << "'");
      overloadSet = prior->getOrCreateOverloadSet();
      prior = NULL;    // so we don't consider this to be the same
    }
  }

  return overloadSet;
}


// dsw: is this function of the type that would be created as an implicit
// type in K and R C at the call site to a function that has not been
// declared; this seems too weird to make a method on FunctionType, but
// feel free to move it
static bool isImplicitKandRFuncType(FunctionType *ft)
{
  // is the return type an int?
  Type *retType = ft->retType;
  if (!retType->isCVAtomicType()) return false;
  if (!retType->asCVAtomicType()->isSimpleType()) return false;
  if (retType->asCVAtomicType()->asSimpleTypeC()->type != ST_INT) return false;

  // does it accept lack parameter information?
  if (ft->params.count() != 0) return false;
  if (!(ft->flags & FF_NO_PARAM_INFO)) return false;

  return true;
}


// possible outcomes:
//   - error, make up a dummy variable
//   - create new declaration
//   - re-use existing declaration 'prior'
// caller shouldn't have to distinguish first two
Variable *Env::createDeclaration(
  SourceLoc loc,            // location of new declaration
  StringRef name,           // name of new declared variable
  Type *type,               // type of that variable
  DeclFlags dflags,         // declaration flags for new variable
  Scope *scope,             // scope into which to insert it
  CompoundType *enclosingClass,   // scope->curCompound, or NULL for a hack that is actually wrong anyway (!)
  Variable *prior,          // pre-existing variable with same name and type, if any
  OverloadSet *overloadSet  // set into which to insert it, if that's what to do
) {
  // if this gets set, we'll replace a conflicting variable
  // when we go to do the insertion
  bool forceReplace = false;

  // is there a prior declaration?
  if (prior) {
    // check for exception given by [cppstd 7.1.3 para 2]:
    //   "In a given scope, a typedef specifier can be used to redefine
    //    the name of any type declared in that scope to refer to the
    //    type to which it already refers."
    if (prior->hasFlag(DF_TYPEDEF) &&
        (dflags & DF_TYPEDEF)) {
      // let it go; the check below will ensure the types match
    }

    else {
      // check for violation of the One Definition Rule
      if (prior->hasFlag(DF_DEFINITION) &&
          (dflags & DF_DEFINITION) &&
          !multipleDefinitionsOK(*this, prior, dflags)) {
        // HACK: if the type refers to type variables, then let it slide
        // because it might be Foo<int> vs. Foo<float> but my simple-
        // minded template implementation doesn't know they're different
        //
        // actually, I just added TypeVariables to 'Type::containsErrors',
        // so the error message will be suppressed automatically
        //
        // 8/15/04: removing 'prior->type' so I can detect duplicate
        // definitions of template functions (t0258.cc errors 1 and
        // 2); I think the right soln is to remove the
        // type-vars-suppress-errors thing altogether, but I want to
        // minimize churn for the moment
        error(/*prior->type,*/ stringc
          << "duplicate definition for `" << name
          << "' of type `" << prior->type->toString()
          << "'; previous at " << toString(prior->loc),
          maybeEF_STRONG());

      makeDummyVar:
        // the purpose of this is to allow the caller to have a workable
        // object, so we can continue making progress diagnosing errors
        // in the program; this won't be entered in the environment, even
        // though the 'name' is not NULL
        Variable *ret = makeVariable(loc, name, type, dflags);

        // set up the variable's 'scope' field as if it were properly
        // entered into the scope; this is for error recovery, in particular
        // for going on to check the bodies of methods
        scope->registerVariable(ret);

        return ret;
      }

      // check for violation of rule disallowing multiple
      // declarations of the same class member; cppstd sec. 9.2:
      //   "A member shall not be declared twice in the
      //   member-specification, except that a nested class or member
      //   class template can be declared and then later defined."
      //
      // I have a specific exception for this when I do the second pass
      // of typechecking for inline members (the user's code doesn't
      // violate the rule, it only appears to because of the second
      // pass); this exception is indicated by DF_INLINE_DEFN.
      //
      // 8/15/04: Now using 'secondPassTcheck' instead of DF_INLINE_DEFN.
      if (enclosingClass &&
          !secondPassTcheck &&
          !prior->isImplicitTypedef()) {    // allow implicit typedef to be hidden
        if (prior->getUsingAlias()) {
          // The declaration we're trying to make conflicts with an alias
          // imported from a base class.  7.3.3 para 12 says that's ok,
          // that the new declaration effectively replaces the old.
          TRACE("env", "hiding imported alias " << prior->name);
          
          // Go in and change the variable to update what it means.
          // This isn't the cleanest thing in the world, but the
          // alternative involves pulling the old Variable ('prior')
          // out of the scope (easy) and the overload sets (hard), and
          // then substituting the new one for it.
          prior->loc = loc;
          // name remains unchanged
          prior->type = type;
          prior->setFlagsTo(dflags);
          prior->funcDefn = NULL;
          // overload can stay the same
          prior->setUsingAlias(NULL);
          scope->registerVariable(prior);
          return prior;
        }
        else if (lang.allowMemberWithClassName &&
                 prior->hasAllFlags(DF_SELFNAME | DF_TYPEDEF)) {
          // 9/22/04: Special case for 'struct ip_opts': allow the member
          // to replace the class name, despite 9.2 para 13.
          TRACE("env", "allowing member `" << prior->name <<
                       "' with same name as class");
          forceReplace = true;
          goto noPriorDeclaration;
        }
        else {
          error(stringc
            << "duplicate member declaration of `" << name
            << "' in " << enclosingClass->keywordAndName()
            << "; previous at " << toString(prior->loc),
            maybeEF_STRONG());    // weakened for t0266.cc
          goto makeDummyVar;
        }
      }
    }

    // check that the types match, and either both are typedefs
    // or neither is a typedef
    if (almostEqualTypes(prior->type, type) &&
        (prior->flags & DF_TYPEDEF) == (dflags & DF_TYPEDEF)) {
      // same types, same typedef disposition, so they refer
      // to the same thing
    }
    else if (prior->isImplicitTypedef() &&
             !(dflags & DF_TYPEDEF)) {
      // if the previous guy was an implicit typedef, then as a
      // special case allow it, and arrange for the environment
      // to replace the implicit typedef with the variable being
      // declared here                
      //
      // 2005-03-01: this is only legal if the new declaration
      // is for a non-type (7.1.3p3)

      TRACE("env",    "replacing implicit typedef of " << prior->name
                   << " at " << prior->loc << " with new decl at "
                   << loc);
      forceReplace = true;

      goto noPriorDeclaration;
    }
    else if (lang.allowExternCThrowMismatch &&
             prior->hasFlag(DF_EXTERN_C) &&
             (prior->flags & DF_TYPEDEF) == (dflags & DF_TYPEDEF) &&
             prior->type->isFunctionType() &&
             almostEqualTypes(prior->type, type, Type::EF_IGNORE_EXN_SPEC)) {
      // 10/01/04: allow the nonstandard variation in exception specs
      // for extern-C decls (since they usually don't throw exceptions
      // at all)
      //
      // Note that by regarding these decls as compatible, the second
      // exn spec will be ignored in favor of the first, which will be
      // unsound if the second allows more exceptions and the function
      // really can throw an exception.
      warning(stringc << "allowing nonstandard variation in exception specs "
                      << "(conflicting decl at " << prior->loc
                      << ") due to extern-C");
    }
    else if (lang.allowImplicitFunctionDecls &&
             prior->hasFlag(DF_FORWARD) &&
             prior->type->isFunctionType() &&
             isImplicitKandRFuncType(prior->type->asFunctionType())) {
      // dsw: in K&R C sometimes what happens is that a function is
      // called and then later declared; at the function call site a
      // declaration is invented with type 'int (...)' but the real
      // declaration is likely to collide with that here.  We don't
      // try to back-patch and do anything clever or sound, we just
      // turn the error into a warning so that the file can go
      // through; this is an unsoundness
      warning(stringc
              << "prior declaration of function `" << name
              << "' at " << prior->loc
              << " had type `" << prior->type->toString()
              << "', but this one uses `" << type->toString() << "'."
              << " This is most likely due to the prior declaration being implied "
              << "by a call to a function before it was declared.  "
              << "Keeping the implied, weaker declaration; THIS IS UNSOUND.");
    }
    else {
      // this message reports two declarations which declare the same
      // name, but their types are different; we only jump here *after*
      // ruling out the possibility of function overloading

      string msg = stringc
        << "prior declaration of `" << name
        << "' at " << prior->loc
        << " had type `" << prior->type->toString()
        << "', but this one uses `" << type->toString() << "'";

      if (!lang.isCplusplus &&
          prior->type->isFunctionType() &&
          type->isFunctionType() &&
          prior->type->asFunctionType()->params.count() == type->asFunctionType()->params.count()) {
        // 10/08/04: In C, the rules for function type declaration
        // compatibility are more complicated, relying on "default
        // argument promotions", a determination of whether the
        // previous declaration came from a prototype (and whether it
        // had param info), and a notion of type "compatibility" that
        // I don't see defined.  (C99 6.7.5.3 para 15)  So, I'm just
        // going to make this a warning and go on.  I think more needs
        // to be done for an analysis, though, since an argument
        // passed as an 'int' but treated as a 'char' by the function
        // definition (as in in/c/t0016.c) is being implicitly
        // truncated, and this might be relevant to the analysis.
        //
        // Actually, compatibility is introduced in 6.2.7, though
        // pieces of its actual definition appear in 6.7.2 (6.7.2.3
        // only?), 6.7.3 and 6.7.5.  I still need to track down what
        // argument promotions are.
        //
        // TODO: tighten all this down; I don't like leaving such big
        // holes in what is being checked....
        warning(msg);
      }
      else {
        // usual error message
        error(type, msg);
        goto makeDummyVar;
      }
    }

    // if the prior declaration refers to a different entity than the
    // one we're trying to declare here, we have a conflict (I'm trying
    // to implement 7.3.3 para 11); I try to determine whether they
    // are the same or different based on the scopes in which they appear
    if (!prior->skipAlias()->scope &&
        !scope->isPermanentScope()) {
      // The previous decl is not in a named scope.. I *think* that
      // means that we could only have found it by looking in the same
      // scope that 'scope' refers to, which is also non-permanent, so
      // just allow this.  I don't know if it's really right.  In any
      // case, d0097.cc is a testcase.
    }
    else if (!sameScopes(prior->skipAlias()->scope, scope)) {
      error(type, stringc
            << "prior declaration of `" << name
            << "' at " << prior->loc
            << " refers to a different entity, so it conflicts with "
            << "the one being declared here");
      goto makeDummyVar;
    }

    // ok, use the prior declaration, but update the 'loc'
    // if this is the definition
    if (dflags & DF_DEFINITION) {
      TRACE("odr",    "def'n of " << name
                   << " at " << toString(loc)
                   << " overrides decl at " << toString(prior->loc));
      prior->loc = loc;
      prior->setFlag(DF_DEFINITION);
      prior->clearFlag(DF_EXTERN);
      prior->clearFlag(DF_FORWARD); // dsw: I added this
    }

    // dsw: if one has a size and the other doesn't, use the size of
    // the one that is defined
    if (prior->type->isArrayType()
        && prior->type->asArrayType()->size == ArrayType::NO_SIZE) {
      prior->type->asArrayType()->size = type->asArrayType()->size;
    }

    // prior is a ptr to the previous decl/def var; type is the
    // type of the alias the user wanted to introduce, but which
    // was found to be equivalent to the previous declaration

    // 10/03/04: merge default arguments
    if (lang.isCplusplus && type->isFunctionType()) {
      mergeDefaultArguments(loc, prior, type->asFunctionType());
    }

    // TODO: enforce restrictions on successive declarations'
    // DeclFlags; see cppstd 7.1.1, around para 7

    // it's an allowed, repeated declaration
    return prior;
  }

noPriorDeclaration:
  // no prior declaration, make a new variable and put it
  // into the environment (see comments in Declarator::tcheck
  // regarding point of declaration)
  Variable *newVar = makeVariable(loc, name, type, dflags);

  // set up the variable's 'scope' field
  scope->registerVariable(newVar);

  if (overloadSet) {
    // don't add it to the environment (another overloaded version
    // is already in the environment), instead add it to the overload set
    overloadSet->addMember(newVar);
    newVar->overload = overloadSet;
    TRACE("ovl", "overload set for " << name << " now has " <<
                 overloadSet->count() << " elements");
  }
  else if (!type->isError()) {       
    // 8/09/04: add error-typed vars anyway?            
    //
    // No, they get ignored by Scope::addVariable anyway, because
    // it's part of ensuring that erroneous interpretations don't
    // modify the environment, during disambiguation.

    if (disambErrorsSuppressChanges()) {
      TRACE("env", "not adding D_name `" << name <<
            "' because there are disambiguating errors");
    }
    else {
      scope->addVariable(newVar, forceReplace);
      addedNewVariable(scope, newVar);
    }
  }

  return newVar;
}


// if 'type' refers to a function type, and it has some default
// arguments supplied, then:
//   - it should only be adding new defaults, not overriding
//     any from a previous declaration
//   - the new defaults should be merged into the type retained
//     in 'prior->type', so that further uses in this translation
//     unit will have the benefit of the default arguments
//   - the resulting type should have all the default arguments
//     contiguous, and at the end of the parameter list
// reference: cppstd, 8.3.6
void Env::mergeDefaultArguments(SourceLoc loc, Variable *prior, FunctionType *type)
{
  SObjListIterNC<Variable> priorParam(prior->type->asFunctionType()->params);
  SObjListIterNC<Variable> newParam(type->params);

  bool seenSomeDefaults = false;

  int paramCt = 1;
  for(; !priorParam.isDone() && !newParam.isDone();
      priorParam.adv(), newParam.adv(), paramCt++) {
    Variable *p = priorParam.data();
    Variable *n = newParam.data();

    if (n->value) {
      seenSomeDefaults = true;
      
      if (p->value) {
        error(loc, stringc
          << "declaration of `" << prior->name
          << "' supplies a redundant default argument for parameter "
          << paramCt);
      }
      else {
        // augment 'p' with 'n->value'
        //
        // TODO: what are the scoping issues w.r.t. evaluating
        // default arguments?
        p->value = n->value;
      }
    }
    else if (!p->value && seenSomeDefaults) {
      error(loc, stringc
        << "declaration of `" << prior->name
        << "' supplies some default arguments, but no default for later parameter "
        << paramCt << " has been supplied");
    }
  }

  // both parameter lists should end simultaneously, otherwise why
  // did I conclude they are declaring the same entity?
  xassert(priorParam.isDone() && newParam.isDone());
}


Variable *Env::storeVar(Variable *var)
{
  // this is my point of global de-aliasing; it's the obvious place to
  // potentially implement a more flexible scheme where the analysis
  // could request that dealiasing not be done

  // this works even when 'var' is NULL because 'skipAlias' is not a
  // virtual function, and it internally checks for a NULL 'this'

  return var->skipAlias();
}

Variable *Env::storeVarIfNotOvl(Variable *var)
{
  // NULL and non-aliases go through fine
  if (!var ||
      !var->getUsingAlias()) {
    return var;
  }

  if (!var->overload && !var->getUsingAlias()->overload) {
    // neither the alias nor the aliased entity are overloaded, can
    // safely skip aliases now
    return storeVar(var);
  }

  // since 'var' or its alias is overloaded, we'll need to wait for
  // overload resolution, which must act on the aliases; when it is
  // finished it will skip remaining aliases
  return var;
}


void Env::checkFuncAnnotations(FunctionType *, D_func *)
{}

void Env::addedNewCompound(CompoundType *)
{}

int Env::countInitializers(SourceLoc loc, Type *type, IN_compound const *cpd)
{
  return cpd->inits.count();
}

void Env::addedNewVariable(Scope *, Variable *)
{}


E_intLit *Env::buildIntegerLiteralExp(int i)
{
  StringRef text = str(stringc << i);
  E_intLit *ret = new E_intLit(text);
  ret->i = i;
  return ret;
}


// create a PseudoInstantiation, bind its arguments, and create
// the associated typedef variable
PseudoInstantiation *Env::createPseudoInstantiation
  (CompoundType *ct, ObjList<STemplateArgument> const &args)
{
  TRACE("pseudo", "creating " << ct->name << sargsToString(args));

  // make the object itself
  PseudoInstantiation *pi = new PseudoInstantiation(ct);

  // attach the template arguments
  FOREACH_OBJLIST(STemplateArgument, args, iter) {
    pi->args.prepend(new STemplateArgument(*(iter.data())));
  }
  pi->args.reverse();

  // make the typedef var; do *not* add it to the environment
  pi->typedefVar = makeVariable(loc(), ct->name, makeType(loc(), pi),
                                DF_TYPEDEF | DF_IMPLICIT);

  return pi;
}


// Push onto the scope stack the scopes that contain the declaration
// of 'v', stopping (as we progress to outer scopes) at 'stop' (do
// not push 'stop').
//
// Currently, this is only used by template instantiation code, but
// I want to eventually replace uses of 'getQualifierScopes' with this.
void Env::pushDeclarationScopes(Variable *v, Scope *stop)
{
  ObjListMutator<Scope> mut(scopes);
  Scope *s = v->scope;

  while (s != stop) {
    mut.insertBefore(s);     // insert 's' before where 'mut' points
    mut.adv();               // advance 'mut' past 's', so it points at orig obj
    s = s->parentScope;
    if (!s) {
      // 'v->scope' must not have been inside 'stop'
      xfailure("pushDeclarationScopes: missed 'stop' scope");
    }
  }
}

// undo the effects of 'pushDeclarationScopes'
void Env::popDeclarationScopes(Variable *v, Scope *stop)
{
  Scope *s = v->scope;
  while (s != stop) {
    Scope *tmp = scopes.removeFirst();
    xassert(tmp == s);
    s = s->parentScope;
    xassert(s);
  }
}


Variable *getParameterizedPrimary(Scope *s)
{
  Variable *entity = s->parameterizedEntity;
  TemplateInfo *tinfo = entity->templateInfo();
  xassert(tinfo);
  Variable *ret = tinfo->getPrimary()->var;
  xassert(ret);
  return ret;
}

// We are in a declarator, processing a qualifier that has template
// arguments supplied.  The question is, which template arg/param
// scope is the one that goes with this qualifier?
Scope *Env::findParameterizingScope(Variable *bareQualifierVar)
{
  // keep track of the last unassociated template scope seen
  Scope *lastUnassoc = NULL;

  // search from inside out
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();

    if (s->isTemplateScope()) {
      if (!s->parameterizedEntity) {
        lastUnassoc = s;
      }
      else if (getParameterizedPrimary(s) == bareQualifierVar) {
        // found it!  this scope is already marked as being associated
        // with this variable
        return s;
      }
    }
    else if (s->isClassScope()) {
      // keep searching..
    }
    else {
      // some kind of scope other than a template or class scope; I don't
      // think I should look any higher
      break;
    }
  }

  // didn't find a scope already associated with the qualifier, so I
  // think the outermost unassociated is the right one (this is
  // something of a guess.. the spec isn't terribly clear on corner
  // conditions regarding template params..)

  if (!lastUnassoc) {
    error(stringc << "no template parameter list supplied for `"
                  << bareQualifierVar->name << "'");
  }

  return lastUnassoc;
}


// Remove all scopes that are inside 'bound' from 'scopes' and put them
// into 'dest' instead, in reverse order (to make it easy to undo).
void Env::removeScopesInside(ObjList<Scope> &dest, Scope *bound)
{
  xassert(bound);
  while (scopes.first() != bound) {
    Scope *s = scopes.removeFirst();
    TRACE("scope", "temporarily removing " << s->desc());
    dest.prepend(s);
  }
}

// Undo the effects of 'removeScopesInside'.
void Env::restoreScopesInside(ObjList<Scope> &src, Scope *bound)
{
  xassert(scopes.first() == bound);
  while (src.isNotEmpty()) {
    Scope *s = src.removeFirst();
    TRACE("scope", "restoring " << s->desc());
    scopes.prepend(s);
  }
}


bool Env::ensureCompleteType(char const *action, Type *type)
{
  if (type->isCompoundType()) {
    CompoundType *ct = type->asCompoundType();
    return ensureCompleteCompound(action, ct);
  }

  if (type->isArrayType() &&
      type->asArrayType()->size == ArrayType::NO_SIZE) {
    if (lang.assumeNoSizeArrayHasSizeOne) {
      warning(stringc << "array of no size assumed to be a complete type");
      return true;
    } else {
      // 8.3.4 para 1: this is an incomplete type
      error(stringc << "attempt to " << action <<
            " incomplete type `" << type->toString() << "'");
      return false;
    }
  }

  return true;
}


bool Env::ensureCompleteCompound(char const *action, CompoundType *ct)
{
  // maybe it's a template we can instantiate?
  if (!ct->isComplete() && ct->isInstantiation()) {
    instantiateClassBody(ct->typedefVar);
  }

  if (!ct->isComplete()) {
    error(stringc << "attempt to " << action <<
                     " incomplete class `" << ct->name << "'");
    return false;
  }

  return true;
}


// if 'e' is the name of a function, or an address of one, which is
// overloaded, get the Variable denoting the overload set; otherwise,
// return NULL
Variable *Env::getOverloadedFunctionVar(Expression *e)
{
  if (e->isE_addrOf()) {
    e = e->asE_addrOf()->expr;
  }

  if (e->isE_variable()) {
    Variable *ret = e->asE_variable()->var;
    if (ret && ret->isOverloaded()) {
      return ret;
    }
  }

  return NULL;
}


// having selected 'selVar' from the set of overloaded names denoted
// by 'e', modify 'e' to reflect that selection, by modifying the
// 'type' and 'var' annotations; this function mirrors
// 'getOverloadedFunctionVar' in structure, as we assume that 'e'
// already went through that one and returned non-NULL
void Env::setOverloadedFunctionVar(Expression *e, Variable *selVar)
{
  if (e->isE_grouping()) {
    E_grouping *g = e->asE_grouping();
    setOverloadedFunctionVar(g->expr, selVar);
    g->type = tfac.cloneType(g->expr->type);
    return;
  }

  if (e->isE_addrOf()) {
    // TODO: In some cases, we may need to form a pointer-to-member;
    // see ArgExpression::mid_tcheck.  At the moment there is no
    // mechanism in Elsa that can reveal this bug.  When I implement
    // initialization compatibility checking then it will be
    // revealed...

    E_addrOf *a = e->asE_addrOf();
    setOverloadedFunctionVar(a->expr, selVar);
    a->type = makePointerType(loc(), CV_NONE, a->expr->type);
    return;
  }

  if (e->isE_variable()) {
    E_variable *ev = e->asE_variable();

    ev->type = selVar->type;
    ev->var = selVar;
  }
  
  else if (e->isE_fieldAcc()) {
    E_fieldAcc *eacc = e->asE_fieldAcc();
    
    eacc->type = selVar->type;
    eacc->field = selVar;
  }
  
  else {
    xfailure("setOverloadedFunctionVar: bad expression kind");
  }
}


// given an overload set, and the type to which the overloaded name is
// being converted, select the element that matches that type, if any
// [cppstd 13.4 para 1]
Variable *Env::pickMatchingOverloadedFunctionVar(LookupSet &set, Type *type)
{
  // normalize 'type' to just be a FunctionType
  type = type->asRval();
  if (type->isPointerType() || type->isPointerToMemberType()) {
    type = type->getAtType();
  }
  if (!type->isFunctionType()) {
    return NULL;     // no matching element if not converting to function
  }

  // as there are no standard conversions for function types or
  // pointer to function types [cppstd 13.4 para 7], simply find an
  // element with an equal type
  SFOREACH_OBJLIST_NC(Variable, set, iter) {
    Variable *v = iter.data();

    if (v->isTemplate()) {
      // TODO: cppstd 13.4 paras 2,3,4
      unimp("address of overloaded name, with a templatized element");
    }

    if (v->type->equals(type)) {
      OVERLOADTRACE("13.4: selected `" << v->toString() << "'");
      return v;     // found it
    }
  }

  OVERLOADTRACE("13.4: no match for type `" << type->toString() << "'");
  return NULL;      // no matching element
}


// given an expression that more or less begins with a name,
// find the name
PQName const *getExprName(Expression const *e)
{
  ASTSWITCHC(Expression, e) {
    ASTCASEC(E_variable, v)
      return v->name;

    ASTNEXTC(E_fieldAcc, f)
      return f->fieldName;

    ASTNEXTC(E_addrOf, a)
      return getExprName(a->expr);

    ASTNEXTC(E_grouping, g)
      return getExprName(g->expr);

    ASTDEFAULTC
      xfailure("getExprName: does not begin with a name");
      return NULL;      // silence warning
      
    ASTENDCASEC
  }
}

// given an expression that more or less begins with a name,
// find its location
SourceLoc getExprNameLoc(Expression const *e)
{
  return getExprName(e)->loc;
}


// 'expr/info' is being passed to 'paramType'; if 'expr' is the
// name of an overloaded function, resolve it
void Env::possiblySetOverloadedFunctionVar(Expression *expr, Type *paramType,
                                           LookupSet &set)
{
  if (set.count() >= 2) {
    // pick the one that matches the target type
    Variable *ovlVar =
      env.pickMatchingOverloadedFunctionVar(set, paramType);
    if (ovlVar) {
      // modify 'arg' accordingly
      env.setOverloadedFunctionVar(expr, ovlVar);
    }
    else {
      env.error(getExprNameLoc(expr), stringc
        << "failed to resolve address-of of overloaded function `"
        << *(getExprName(expr)) << "' assigned to type `"
        << paramType->toString() << "'; candidates:\n"
        << chomp(set.asString()));
    }
  }
}


// get scopes associated with 'type'; cppstd 3.4.2 para 2
//
// this could perhaps be made faster by using a hash table set
void Env::getAssociatedScopes(SObjList<Scope> &associated, Type *type)
{
  switch (type->getTag()) {
    default:
      xfailure("bad type tag");

    case Type::T_ATOMIC: {
      AtomicType *atomic = type->asCVAtomicType()->atomic;
      switch (atomic->getTag()) {
        default:
          // other cases: nothing
          return;

        case AtomicType::T_SIMPLE:
          // bullet 1: nothing
          return;

        case AtomicType::T_COMPOUND: {
          CompoundType *ct = atomic->asCompoundType();
          if (ct->keyword != CompoundType::K_UNION) {
            if (!ct->isInstantiation()) {
              // bullet 2: the class, all base classes, and definition scopes

              // class + bases
              SObjList<BaseClassSubobj const> bases;
              ct->getSubobjects(bases);

              // put them into 'associated'
              SFOREACH_OBJLIST(BaseClassSubobj const, bases, iter) {
                CompoundType *base = iter.data()->ct;
                associated.prependUnique(base);

                // get definition namespace too
                if (base->parentScope) {
                  associated.prependUnique(base->parentScope);
                }
                else {
                  // parent is not named.. I'm pretty sure in this case
                  // any names that should be found will be found by
                  // ordinary lookup, so it will not matter whether the
                  // parent scope of 'base' gets added to 'associated'
                }
              }
            }
            else {
              // bullet 7: template instantiation: definition scope plus
              // associated scopes of template type arguments
              
              // definition scope
              if (ct->parentScope) {
                associated.prependUnique(ct->parentScope);
              } 

              // look at template arguments
              TemplateInfo *ti = ct->templateInfo();
              xassert(ti);
              FOREACH_OBJLIST(STemplateArgument, ti->arguments, iter) {
                STemplateArgument const *arg = iter.data();
                if (arg->isType()) {
                  getAssociatedScopes(associated, arg->getType());
                }
                else if (arg->isTemplate()) {
                  // TODO: implement this
                }
              }
            }
          }
          else {
            // bullet 3 (union): definition scope
            if (ct->parentScope) {
              associated.prependUnique(ct->parentScope);
            }
          }
          break;
        }

        case AtomicType::T_ENUM: {
          // bullet 3 (enum): definition scope
          EnumType *et = atomic->asEnumType();
          if (et->typedefVar &&        // ignore anonymous enumerations...
              et->typedefVar->scope) {
            associated.prependUnique(et->typedefVar->scope);
          }
          break;
        }
      }
      break;
    }
    
    case Type::T_REFERENCE:
      // implicitly skipped as being an lvalue
    case Type::T_POINTER:
    case Type::T_ARRAY:
      // bullet 4: skip to atType
      getAssociatedScopes(associated, type->getAtType());
      break;

    case Type::T_FUNCTION: {
      // bullet 5: recursively look at param/return types
      FunctionType *ft = type->asFunctionType();
      getAssociatedScopes(associated, ft->retType);
      SFOREACH_OBJLIST(Variable, ft->params, iter) {
        getAssociatedScopes(associated, iter.data()->type);
      }
      break;
    }

    case Type::T_POINTERTOMEMBER: {
      // bullet 6/7: the 'inClassNAT', plus 'atType'
      PointerToMemberType *ptm = type->asPointerToMemberType();
      if (ptm->inClassNAT->isCompoundType()) {
        associated.prependUnique(ptm->inClassNAT->asCompoundType());
      }
      getAssociatedScopes(associated, ptm->atType);
      break;
    }
  }
}


// cppstd 3.4.2; returns entries for 'name' in scopes
// that are associated with the types in 'args'
void Env::associatedScopeLookup(LookupSet &candidates, StringRef name, 
                                ArrayStack<Type*> const &argTypes, LookupFlags flags)
{
  // let me disable this entire mechanism, to measure its performance
  //
  // 2005-02-11: On in/big/nsHTMLEditRules, I see no measurable difference
  static bool enabled = !tracingSys("disableArgDepLookup");
  if (!enabled) {
    return;
  }

  // union over all arguments of "associated" namespaces and classes
  SObjList<Scope> associated;
  for (int i=0; i < argTypes.length(); i++) {
    getAssociatedScopes(associated, argTypes[i]);
  }

  // 3.4.2 para 3: ignore 'using' directives for these lookups
  flags |= LF_IGNORE_USING;

  // get candidates from the lookups in the "associated" scopes
  SFOREACH_OBJLIST_NC(Scope, associated, scopeIter) {
    Scope *s = scopeIter.data();
    if ((flags & LF_SKIP_CLASSES) && s->isClassScope()) {
      continue;
    }

    // lookup
    Variable *v = s->lookupVariable(name, env, flags);

    // toss them into the set
    if (v) {
      if (!v->type->isFunctionType()) {
        env.error(loc(), stringc
          << "during argument-dependent lookup of `" << name
          << "', found non-function of type `" << v->type->toString()
          << "' in " << s->desc());
      }
      else {
        addCandidates(candidates, v);
      }
    }
  }
}


// some lookup yielded 'var'; add its candidates to 'candidates'
void Env::addCandidates(LookupSet &candidates, Variable *var)
{
  candidates.adds(var);
}


// This could be parameterized on the kind of AtomicType
// transformation being done, though we do not process the parameters
// of a FunctionType.
//
// If 't' includes any function or array types, and 't' contains DQTs
// that need to be resolved, they must be in the partially-built state
// that arises during declarator parsing.
//
// Returns NULL if no change is needed.
Type *Env::resolveDQTs(SourceLoc loc, Type *t)
{
  switch (t->getTag()) {
    default: xfailure("bad tag");

    case Type::T_ATOMIC: {
      CVAtomicType *at = t->asCVAtomicType();
      if (at->atomic->isNamedAtomicType()) {
        Type *resolved = resolveDQTs_atomic(loc, at->atomic);
        if (resolved) {
          // must combine the 'resolved' type with 'at->cv'
          return tfac.applyCVToType(loc, at->cv, resolved, NULL /*syntax*/);
        }
      }
      return NULL;
    }

    case Type::T_POINTER: {
      PointerType *pt = t->asPointerType();
      Type *resolved = resolveDQTs(loc, pt->atType);
      if (resolved) {
        return tfac.makePointerType(loc, pt->cv, resolved);
      }
      return NULL;
    }

    case Type::T_REFERENCE: {
      ReferenceType *rt = t->asReferenceType();
      Type *resolved = resolveDQTs(loc, rt->atType);
      if (resolved) {
        return tfac.makeReferenceType(loc, resolved);
      }
      return NULL;
    }

    case Type::T_FUNCTION: {
      FunctionType *ft = t->asFunctionType();
      Type *resolved = resolveDQTs(loc, ft->retType);
      if (resolved) {
        // 'finish' ft, since we are about to discard it
        xassert(ft->params.isEmpty());    // partially built
        tfac.doneParams(ft);

        // return a new unfinished function type
        return tfac.makeFunctionType(loc, resolved);
      }
      return NULL;
    }

    case Type::T_ARRAY: {
      ArrayType *at = t->asArrayType();
      Type *resolved = resolveDQTs(loc, at->eltType);
      if (resolved) {
        return tfac.makeArrayType(loc, resolved, at->size);
      }
      return NULL;
    }

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType *ptm = t->asPointerToMemberType();
      Type *resolvedAtType = resolveDQTs(loc, ptm->atType);
      Type *resolvedNAT = resolveDQTs_atomic(loc, ptm->inClassNAT);
      if (resolvedAtType || resolvedNAT) {
        if (!resolvedAtType) {
          resolvedAtType = ptm->atType;
        }

        // map 'resolvedNAT' down to a true NamedAtomicType
        NamedAtomicType *nat;
        if (!resolvedNAT) {
          nat = ptm->inClassNAT;
        }
        else {
          breaker();      // need to test
          if (!resolvedNAT->isCVAtomicType() ||
              !resolvedNAT->asCVAtomicType()->atomic->isNamedAtomicType()) {
            error(loc, stringc << "`" << t->toString() << "' is `"
                               << resolvedNAT->toString() << "', but that "
                               << "is not allowed as a ptm-qualifier");
            return NULL;
          }
          nat = resolvedNAT->asCVAtomicType()->atomic->asNamedAtomicType();
          
          // TODO: there are still more conditions; essentially, 'nat'
          // should be a class, or some dependent type that might be a
          // class in the right circumstances
        }

        return tfac.makePointerToMemberType(loc, nat, ptm->cv, resolvedAtType);
      }
      return NULL;
    }
  }
}


Type *Env::resolveDQTs_atomic(SourceLoc loc, AtomicType *t)
{
  if (!t->isDependentQType()) {
    return NULL;
  }
  DependentQType *dqt = t->asDependentQType();

  if (!dqt->first->isPseudoInstantiation()) {
    return NULL;
  }
  PseudoInstantiation *firstPI = dqt->first->asPseudoInstantiation();

  // Is there a template definition whose primary is
  // 'firstPI->primary' somewhere on the scope stack?
  CompoundType *scopeCt = 
    getMatchingTemplateInScope(firstPI->primary, firstPI->args);
  if (scopeCt) {
    // re-start lookup from 'scopeCt'
    LookupSet set;
    lookupPQ_withScope(set, dqt->rest, LF_QUALIFIED, scopeCt);
    Variable *v = set.isEmpty()? NULL : set.first();

    if (!v || !v->isType()) {
      error(loc, stringc << "no such type `" << t->toCString()
                         << "' (while resolving DQTs)");
      return NULL;     // just keep using what we already have
    }

    return v->type;
  }

  // negative, just keep using what we have
  return NULL;
}


// given 'C' and '<T>', check if it 'C<T>' is already in scope
CompoundType *Env::getMatchingTemplateInScope
  (CompoundType *primary, ObjList<STemplateArgument> const &sargs)
{
  FOREACH_OBJLIST(Scope, scopes, iter) {
    // filter for scopes that are template definitions
    if (!iter.data()->curCompound) continue;
    CompoundType *scopeCt = iter.data()->curCompound;
    TemplateInfo *scopeTI = scopeCt->templateInfo();
    if (!scopeTI) continue;

    // same template+arguments?
    if (!scopeTI->matchesPI(tfac, primary, sargs)) {
      continue;
    }

    // found it!
    return scopeCt;
  }

  // not found
  return NULL;
}


// This function produces a string describing the set of dependent
// base classes that were not searched during unqualified lookup.
string Env::unsearchedDependentBases()
{   
  stringBuilder sb;
  int ct=0;

  FOREACH_OBJLIST(Scope, scopes, iter) {
    // filter for scopes that are template definitions
    if (!iter.data()->curCompound) continue;
    CompoundType *scopeCt = iter.data()->curCompound;
    TemplateInfo *scopeTI = scopeCt->templateInfo();
    if (!scopeTI) continue;

    // list unsearched bases    
    SFOREACH_OBJLIST(Type, scopeTI->dependentBases, baseIter) {
      if (ct++ == 0) {
        sb << " (due to nondependent lookup, did not search bases: ";
      }
      else {
        sb << ", ";
      }
      sb << baseIter.data()->toString();
    }
  }
  
  if (ct) {
    sb << ")";
  }
  
  return sb;
}


// ---------------- new lookup mechanism --------------------
bool sameArguments(ObjList<STemplateArgument> const &args1,
                   ObjList<STemplateArgument> const &args2)
{
  ObjListIter<STemplateArgument> iter1(args1);
  ObjListIter<STemplateArgument> iter2(args2);

  for (; !iter1.isDone() && !iter2.isDone(); iter1.adv(), iter2.adv()) {
    STemplateArgument const &sarg1 = *(iter1.data());
    STemplateArgument const &sarg2 = *(iter2.data());

    if (!sarg1.equals(sarg2)) {
      return false;
    }
  }

  return iter1.isDone() && iter2.isDone();
}


void Env::lookupPQ(LookupSet &set, PQName *name, LookupFlags flags)
{
  // this keeps track of where the next lookup will occur; NULL means
  // "in the current scope stack", and non-NULL means "in the named
  // scope"
  Scope *scope = NULL;

  // ---- BEGIN: second entry point ---
  // I need to jump in to the algorithm right here for 3.4.5p4, so
  // break the function and expose an entry.  When reading
  // Env::lookupPQ, one can effectively ignore the entire 'second
  // entry point' section.
  lookupPQ_withScope(set, name, flags, scope);
}
void Env::lookupPQ_withScope(LookupSet &set, PQName *name, LookupFlags flags,
                             Scope *scope)    // 'scope' is as stated above
{
  // ---- END: second entry point ---

  // keep track of the scope we found just before 'scope'
  Scope *prevScope = NULL;

  // this is so legacy calls will honor 'set' (I plan to eventually
  // get rid of LF_LOOKUP_SET entirely)
  flags |= LF_LOOKUP_SET;

  // lookup along the chain of qualifiers
  while (name->isPQ_qualifier()) {
    PQ_qualifier *qual = name->asPQ_qualifier();
    prevScope = scope;

    if (!qual->qualifier) {
      scope = globalScope();
    }

    else {
      // lookup this qualifier in 'scope'
      Variable *svar = lookupScopeVar(scope, qual->qualifier, flags);

      // 2005-03-04: referring to the self-name but passing args:
      // regard the arguments as being applied to the primary
      // (in/t0425.cc) (in/0428.cc)
      if (svar &&
          svar->hasFlag(DF_SELFNAME) &&
          qual->sargs.isNotEmpty()) {
        if (svar->type->isCompoundType()) {
          // have to go via the CompoundType because the DF_SELFNAME
          // may not have templateInfo
          CompoundType *ct = svar->type->asCompoundType();
          if (ct->templateInfo()) {
            svar = ct->templateInfo()->getPrimary()->var;
          }
        }
        else if (svar->type->isPseudoInstantiation()) {
          PseudoInstantiation *pi = svar->type->asPseudoInstantiation();
          
          // (in/t0433.cc) if the template arguments match those of
          // 'pi', then the qualifier is referring to a known
          // template, not a generic dependent type; this is important
          // because we may need to get a more precise type for
          // decl-defn matching
          if (sameArguments(qual->sargs, pi->args)) {
            scope = svar->scope;    // selfname -> container
            goto bottom_of_loop;    // ...
          }

          svar = svar->type->asCVAtomicType()->atomic->
                   asPseudoInstantiation()->primary->typedefVar;
        }
      }
    
      // interpret 'var', apply template args, etc. (legacy call)
      bool dependent=false, anyTemplates=false;
      scope = lookupOneQualifier_useArgs(svar, qual->sargs, dependent,
                                         anyTemplates, flags);

      // dependent qualified name: if a variable, just yield
      // 'dependentVar', but if a type, we need to construct a more
      // detailed record so we can match declarations with definitions
      if (dependent) {
        DependentQType *dqt;

        if (!( flags & (LF_TYPENAME | LF_ONLY_TYPES) )) {
          set.add(dependentVar);    // user makes no claim, so it's a variable
          dqt = NULL;               // don't need a DQT
        }

        else if (svar->type->isCompoundType()) {
          xassert(containsVariables(qual->sargs));  // otherwise why dependent?

          // build DependentQType PseudoInstantiation(svar, qual->targs)::...
          CompoundType *ct = svar->type->asCompoundType();
          dqt = new DependentQType(createPseudoInstantiation(ct, qual->sargs));
        }

        else if (svar->type->isNamedAtomicType()) {
          if (qual->sargs.isNotEmpty()) {
            error(name->loc, stringc     // t0265.cc error 1
              << "cannot apply template arguments to `" << qual->qualifier << "'",
              EF_STRONG);
            return;
          }

          // build DependentQType on top of 'svar->type'; could be
          // a TypeVariable, a PseudoInstantiation (in/t0430.cc), or
          // another DependentQType (in/t0433.cc)
          dqt = new DependentQType(svar->type->asNamedAtomicType());
        }

        else {
          xfailure("unhandled case of dependent qualified type in lookupPQ");
        }

        // examine the names further down
        finishDependentQType(set, dqt, qual->rest);
        return;
      }

      if (!scope) {
        return;      // error already reported
      }

      // a scope used as a qualifier must be a complete type; I cannot
      // find anyplace in the standard that explicitly requires this,
      // though it seems to be clearly true (t0245.cc)
      if (scope->curCompound) {
        env.ensureCompleteCompound("use as qualifier", scope->curCompound);
      }
    }

  bottom_of_loop:
    flags |= LF_QUALIFIED;            // subsequent lookups are qualified
    name = qual->rest;
  }
                                             
  // 3.4.3p5: qualified dtor name?
  if (scope && name->getName()[0]=='~') {
    // get a pointer to the StringRef in 'name'
    StringRef *nameStringRef;
    if (name->isPQ_name()) {
      nameStringRef = &(name->asPQ_name()->name);
    }
    else /*PQ_template*/ {
      nameStringRef = &(name->asPQ_template()->name);
    }

    // we want to lookup the type; temporarily remove the '~'
    StringRef origStringRef = *nameStringRef;
    *nameStringRef = str(origStringRef+1);

    // original name is of form
    //   prevScope :: scope :: ~ class-name
    Scope *lookupScope;
    if (scope->isNamespace()) {
      lookupScope = scope;              // lookup class-name in 'scope'
    }
    else {
      xassert(scope->isClassScope());
      lookupScope = prevScope;          // lookup class-name in 'prevScope'
    }
    Variable *className =
      unqualifiedFinalNameLookup_one(lookupScope, name, flags);

    // repair 'name'
    *nameStringRef = origStringRef;

    // get the destructor
    if (!className || !className->isClass()) {
      string scopeName;
      if (lookupScope) {
        scopeName = stringc << "in scope `" << lookupScope->fullyQualifiedCName() << "'";
      }
      env.error(name->loc, stringc
        << "no such class `" << *name << "'" << scopeName);
    }
    else {
      lookupClassDestructor(set, className->type->asCompoundType(), flags);
    }

    return;
  }

  // unqualified lookup in 'scope'
  unqualifiedFinalNameLookup(set, scope, name, flags);
}

void Env::unqualifiedFinalNameLookup(LookupSet &set, Scope *scope,
                                     PQName *name, LookupFlags flags)
{
  unqualifiedLookup(set, scope, name->getName(), flags);
  if (set.isEmpty()) {
    return;
  }

  // TODO: BUG: If 'name' is ON_conversion, then we need to further
  // filter the resulting set for those that convert to the right
  // type.  Also, we need to check that looking up the type name in
  // 'scope' yields the same results as its current setting, which
  // came from tchecking in the current scope.

  // consider template arguments?
  if (flags & LF_TEMPL_PRIMARY) {
    return;    // no
  }

  // if we are even considering applying template arguments, the
  // name must be unambiguous
  if (set.count() > 1) {
    env.error(name->loc, stringc << "ambiguous use of `" << *name
                                 << "' in non-overloadable context");
    set.removeAllButOne();    // error recovery
    return;
  }

  // apply template arguments, if any (legacy call)
  Variable *var = set.removeFirst();
  var = applyPQNameTemplateArguments(var, name, flags);
  if (var) {
    set.add(var);
  }
}


// starting from 'scope', lookup scope name 'name' to get
// the corresponding Variable (NULL if (reported) error);
// in the case of a template class, this returns the primary,
// so template args may still have to be applied
Variable *Env::lookupScopeVar(Scope * /*nullable*/ scope, StringRef name,
                              LookupFlags flags)
{
  // lookup 'name' in 'scope'
  flags |= LF_QUALIFIER_LOOKUP;
  flags &= ~LF_ONLY_TYPES;      // only-types is only for the final component, not the qualifiers
  LookupSet set;
  unqualifiedLookup(set, scope, name, flags);
  if (set.isEmpty()) {
    return NULL;                // error already reported
  }
  xassert(set.count() == 1);    // 'flags' should ensure this
  return set.first();
}


void Env::unqualifiedLookup(LookupSet &set, Scope * /*nullable*/ scope,
                            StringRef name, LookupFlags flags)
{
  if (scope) {
    // lookup in a specific scope
    scope->lookup(set, name, env, flags);
    return;
  }

  if (flags & LF_INNER_ONLY) {
    // look in innermost accepting scope
    scope = acceptingScope();
    scope->lookup(set, name, env, flags);
    return;
  }

  // look in current scope stack
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    scope = iter.data();
    if ((flags & LF_SKIP_CLASSES) && scope->isClassScope()) {
      continue;
    }

    if (scope->isDelegated()) {
      // though 'scope' appears physically here, it is searched in a different order
      continue;
    }

    // look in 'scope'
    scope->lookup(set, name, env, flags);
    if (set.isNotEmpty()) {
      return;
    }

    // is 'scope' connected to a delegated scope?
    Scope *delegated = scope->getDelegationPointer();
    if (delegated) {
      delegated->lookup(set, name, env, flags);
      if (set.isNotEmpty()) {
        return;
      }
    }
  }
}


void Env::lookupClassDestructor(LookupSet &set, CompoundType *ct,
                                LookupFlags flags)
{
  // make the destructor name (perhaps this should be saved somewhere?)
  StringRef dtorName = env.str(stringc << "~" << ct->name);

  // look it up
  ct->lookup(set, dtorName, env, flags);
}


// caller promises that it will reject function names anyway
Variable *Env::lookupPQ_one(PQName *name, LookupFlags flags)
{
  LookupSet set;
  lookupPQ(set, name, flags);
  return set.isEmpty()? NULL : set.first();
}

Variable *Env::unqualifiedLookup_one(StringRef name, LookupFlags flags)
{
  LookupSet set;
  unqualifiedLookup(set, NULL /*scope*/, name, flags);
  return set.isEmpty()? NULL : set.first();
}

Variable *Env::unqualifiedFinalNameLookup_one(Scope *scope, PQName *name,
                                              LookupFlags flags)
{
  LookupSet set;
  unqualifiedFinalNameLookup(set, scope, name, flags);
  return set.isEmpty()? NULL : set.first();
}


// given a partially-constructed DQT, with trailing name components
// 'name', finish building it, and then put its typedef Variable
// into 'set'
void Env::finishDependentQType(LookupSet &set, DependentQType * /*nullable*/ dqt,
                               PQName *name)
{
  PQName *origName = name;

  // finish checking for proper use of 'template' keyword
  while (name->isPQ_qualifier()) {
    PQ_qualifier *qual = name->asPQ_qualifier();
    checkTemplateKeyword(qual);
    name = qual->rest;
  }
  checkTemplateKeyword(name);

  // finish building the DQT
  if (dqt) {
    // attach the remainder of the name to the DQT
    dqt->rest = origName;

    // slap a typedefVar on 'dqt' so we can put it into 'set'
    dqt->name = name->getName();
    dqt->typedefVar = makeVariable(name->loc, dqt->name,
                                   makeType(name->loc, dqt),
                                   DF_TYPEDEF | DF_IMPLICIT);
    set.add(dqt->typedefVar);
  }
}

// given 'name' that appears below a dependent-type qualifier,
// check that the 'template' keyword is used properly
void Env::checkTemplateKeyword(PQName *name)
{
  bool templateUsed = name->templateUsed();
  bool hasTemplArgs = name->isPQ_template() ||
                      (name->isPQ_qualifier() && name->asPQ_qualifier()->sargs.isNotEmpty());

  // we only need to check for one kind of mismatch, because the
  // other kind is rejected by the parser
  if (!templateUsed && hasTemplArgs) {
    // without the "template" keyword, the dependent context may give
    // rise to ambiguity, so reject it
    env.error("dependent template scope name requires 'template' keyword",
              EF_DISAMBIGUATES | EF_STRONG);
  }
}


// A lookup has caused an error, and we want to return an error object
// to distinguish this from a lookup that simply fails to find a name.
//
// Currently, lookup functions always return NULL when something is not
// found, but have an ad-hoc and inconsistent mix of behaviors when a
// lookup is erroneous, including:
//   - returning NULL
//   - returning a related object, like a template primary
//   - returning error[Type]Var
// I would like to transition to a scheme that consistently uses error
// objects for erroneous lookups, but my plan for the moment is to do
// so incrementally.
Variable *Env::lookupErrorObject(LookupFlags flags)
{
  if (flags & (LF_ONLY_TYPES | LF_TYPENAME | LF_QUERY_TAGS)) {
    return errorTypeVar;
  }
  else {
    return errorVar;
  }
}


// ----------------------- makeQualifiedName -----------------------
// prepend to 'name' all possible qualifiers, starting with 's'
PQName *Env::makeFullyQualifiedName(Scope *s, PQName *name)
{
  if (!s || s->scopeKind == SK_GLOBAL) {
    // cons the global-scope qualifier on front
    return new PQ_qualifier(SL_UNKNOWN, NULL /*qualifier*/,
                            NULL /*targs*/, name);
  }

  // add 's'
  name = makeQualifiedName(s, name);

  // now find additional qualifiers needed to name 's'
  return makeFullyQualifiedName(s->parentScope, name);
}


// prepend to 'name' information about 's', the scope that contains it
PQName *Env::makeQualifiedName(Scope *s, PQName *name)
{
  // should only be called for named scopes
  Variable *typedefVar = s->getTypedefName();
  xassert(typedefVar);

  // is this scope an instantiated template class?
  TemplateInfo *ti = typedefVar->templateInfo();
  if (ti) {
    // construct the list of template arguments; we must rebuild them
    // instead of using templateInfo->arguments directly, because the
    // TS_names that are used in the latter might not be in scope here
    /*fakelist*/TemplateArgument *targs = makeTemplateArgs(ti);

    // cons a template-id qualifier on to the front
    return new PQ_qualifier(loc(), typedefVar->name, targs, name);
  }
  else {
    // cons an ordinary qualifier in from
    return new PQ_qualifier(loc(), typedefVar->name, NULL /*targs*/, name);
  }
}


// construct the list of template arguments
/*fakelist*/TemplateArgument *Env::makeTemplateArgs(TemplateInfo *ti)
{
  // will build the reversed list first
  TemplateArgument *targs = NULL;

  FOREACH_OBJLIST(STemplateArgument, ti->arguments, iter) {
    STemplateArgument const *sarg = iter.data();
    if (sarg->kind == STemplateArgument::STA_TYPE) {
      // pull out the Type, then build a new ASTTypeId for it
      targs = new TA_type(buildASTTypeId(sarg->getType()), targs);
    }
    else {
      // will make an Expr node; put it here
      Expression *e = NULL;
      switch (sarg->kind) {
        default: xfailure("bad kind");

        case STemplateArgument::STA_REFERENCE:
        case STemplateArgument::STA_POINTER:
        case STemplateArgument::STA_MEMBER:
        case STemplateArgument::STA_TEMPLATE:
          // I want to find a place where someone is mapping a Variable
          // into a PQName, so I can re-use/abstract that mechanism.. but
          // I can't even find a place where the present code is even
          // called!  So for now....
          xfailure("unimplemented");

        case STemplateArgument::STA_INT:
          // synthesize an AST node for the integer
          e = buildIntegerLiteralExp(sarg->getInt());
          break;
      }

      // put the expr in a TA_nontype
      targs = new TA_nontype(e, targs);
    }
  }

  // get the un-reversed version
  FakeList<TemplateArgument> *ret = FakeList<TemplateArgument>::makeList(targs);
  ret = ret->reverse();

  return ret->first();
}


ASTTypeId *Env::buildASTTypeId(Type *type)
{
  // there used to be a big function here that built the full syntax
  // of a type, but I am going to try to use TS_type instead
  return new ASTTypeId(new TS_type(loc(), type),
                       new Declarator(new D_name(loc(), NULL /*name*/),
                                      NULL /*init*/));
}


// ------------------------ SavedScopePair -------------------------
SavedScopePair::SavedScopePair(Scope *s)
  : scope(s),
    parameterizingScope(NULL)
{}

SavedScopePair::~SavedScopePair()
{
  if (scope) {
    delete scope;
  }
}


// ------------------- DefaultArgumentChecker -----------------
bool DefaultArgumentChecker::visitIDeclarator(IDeclarator *obj)
{
  // I do this from D_func to be sure I am only getting Declarators
  // that represent parameters, as opposed to random other uses
  // of Declarators.
  if (obj->isD_func()) {
    FAKELIST_FOREACH(ASTTypeId, obj->asD_func()->params, iter) {
      Declarator *d = iter->decl;
      if (d->init) {
        if (!isInstantiation) {
          // usual thing
          d->tcheck_init(env);
        }
        else {
          // if this is an instantiation, then we want to postpone
          // instantiating the default arguments until the method
          // is called (cppstd 14.7.1 para 11); we remove the default
          // arg from the AST and attach it to the Variable
          xassert(d->init->isIN_expr());    // should be parse failure otherwise, I think
          IN_expr *ie = d->init->asIN_expr();
          d->var->value = ie->e;
          ie->e = NULL;
        }
      }
    }
  }

  return true;    // traverse into children
}


// ----------------- DisambiguationErrorTrapper ---------------------
DisambiguationErrorTrapper::DisambiguationErrorTrapper(Env &e)
  : env(e),
    existingErrors()
{
  // grab the existing list of error messages
  existingErrors.takeMessages(env.errors);

  // tell the environment about this hidden list of errors, so that
  // if an error needs to be added that has nothing to do with this
  // disambiguation, it can be
  if (env.hiddenErrors == NULL) {     // no hidden yet, I'm the first
    env.hiddenErrors = &existingErrors;
  }

  // having stolen the existing errors, we now tell the environment
  // we're in a disambiguation pass so it knows that any disambiguating
  // errors are participating in an active disambiguation
  env.disambiguationNestingLevel++;
}

DisambiguationErrorTrapper::~DisambiguationErrorTrapper()
{
  // we're about to put the pre-existing errors back into env.errors
  env.disambiguationNestingLevel--;

  if (env.hiddenErrors == &existingErrors) {     // I'm the first
    env.hiddenErrors = NULL;          // no more now
  }

  // put all the original errors in
  env.errors.takeMessages(existingErrors);
}


// -------- diagnostics --------
Type *Env::errorType()
{
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}

Type *Env::dependentType()
{
  return getSimpleType(SL_UNKNOWN, ST_DEPENDENT);
}

Type *Env::error(rostring msg, ErrorFlags eflags)
{
  return error(loc(), msg, eflags);
}


Type *Env::warning(rostring msg)
{
  return warning(loc(), msg);
}

Type *Env::warning(SourceLoc loc, rostring msg)
{
  string instLoc = instLocStackString();
  TRACE("error", "warning: " << msg << instLoc);
  if (!disambiguateOnly) {
    errors.addError(new ErrorMsg(loc, msg, EF_WARNING, instLoc));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}


Type *Env::unimp(rostring msg)
{
  string instLoc = instLocStackString();

  // always print this immediately, because in some cases I will
  // segfault (typically deref'ing NULL) right after printing this
  cout << toString(loc()) << ": unimplemented: " << msg << instLoc << endl;

  breaker();
  errors.addError(new ErrorMsg(
    loc(), stringc << "unimplemented: " << msg, EF_NONE, instLoc));
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}


Type *Env::error(Type *t, SourceLoc loc, rostring msg)
{
  if (t->isSimple(ST_DEPENDENT)) {
    // no report, propagate dependentness
    return t;
  }

  if (t->containsErrors() ||
      (!doReportTemplateErrors && t->containsTypeVariables())) {
    // no report
    return getSimpleType(SL_UNKNOWN, ST_ERROR);
  }
  else {
    // report; clashes never disambiguate
    return error(loc, msg, EF_NONE);
  }
}

Type *Env::error(Type *t, rostring msg)
{
  return error(t, loc(), msg);
}


ErrorFlags Env::maybeEF_STRONG() const
{
  if (disambiguateOnly && !doReportTemplateErrors) {
    return EF_STRONG_WARNING;
  }
  else {
    return EF_STRONG;
  }
}


// 2005-03-11: removed Env::doOverload


bool Env::doOperatorOverload() const
{
  static bool disabled = tracingSys("doNotOperatorOverload");
  if (disabled) { return false; }

  if (!lang.allowOverloading) { return false; }

  // 10/09/04: *Do* overload resolution even in template bodies, as
  // long as the arguments are not dependent (checked elsewhere).
  //if (disambiguateOnly) { return false; }

  return true;
}


void Env::addError(ErrorMsg * /*owner*/ obj)
{
  string instLoc = instLocStackString();
  if (instLoc.length()) {
    obj->instLoc = instLoc;
  }

  ErrorList::addError(obj);
}


// I want this function to always be last in this file, so I can easily
// find it to put a breakpoint in it.
Type *Env::error(SourceLoc L, rostring msg, ErrorFlags eflags)
{
  bool report =
    (eflags & EF_DISAMBIGUATES) ||
    (eflags & EF_STRONG) ||
    (eflags & EF_STRONG_WARNING) ||
    (!disambiguateOnly);

  if (env.doReportTemplateErrors) {
    report = true;    // override the above
  }

  TRACE("error", ((eflags & EF_DISAMBIGUATES)? "[d] " :
                  (eflags & (EF_WARNING | EF_STRONG_WARNING))? "[w] " : "")
              << (report? "" : "(suppressed) ")
              << toString(L) << ": " << msg << instLocStackString());

  if (report) {
    addError(new ErrorMsg(L, msg, eflags));
  }

  return errorType();
}


// Don't add any more functions below 'Env::error'.  Instead, put them
// above the divider line that says "diagnostics".

// EOF
