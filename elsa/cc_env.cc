// cc_env.cc            see license.txt for copyright and terms of use
// code for cc_env.h

#include "cc_env.h"        // this module
#include "trace.h"         // tracingSys
#include "ckheap.h"        // heapCheck
#include "strtable.h"      // StringTable
#include "cc_lang.h"       // CCLang
#include "strutil.h"       // suffixEquals


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
  if (!ct->getNamedFieldC(env.constructorSpecialName, env, LF_INNER_ONLY)) {
    // add a no-arg ctor declaration: "Class();".  For now we just
    // add the variable to the scope and don't construct the AST, in
    // order to be symmetric with what is going on with the dtor
    // below.
    FunctionType *ft = env.beginConstructorFunctionType(loc, ct);
    ft->doneParams();
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
    ft->doneParams();
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

    ft->doneParams();

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
  : scopes(),
    disambiguateOnly(false),
    anonTypeCounter(1),
    ctorFinished(false),

    disambiguationNestingLevel(0),
    errors(),
    hiddenErrors(NULL),
    instantiationLocStack(),

    str(s),
    lang(L),
    tfac(tf),
    madeUpVariables(),

    // filled in below; initialized for definiteness
    type_info_const_ref(NULL),

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

    dependentTypeVar(NULL),
    dependentVar(NULL),
    errorTypeVar(NULL),
    errorVar(NULL),
    globalScopeVar(NULL),

    var__builtin_constant_p(NULL),

    // operatorName[] initialized below

    // builtin{Un,Bin}aryOperator[] start as arrays of empty
    // arrays, and then have things added to them below

    tunit(tunit0),

    currentLinkage(quote_C_plus_plus_quote),
    
    // 7/30/04: I'm now making this default to true, with a tracing
    // flag to turn it back off, since I think our overload
    // implementation is now mature enough.
    doOverload(!tracingSys("doNotOverload") && lang.allowOverloading),

    doOperatorOverload(tracingSys("doOperatorOverload") && lang.allowOverloading),
    collectLookupResults(NULL),
    
    tcheckMode(TTM_1NORMAL)
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

  // create the typeid type
  CompoundType *ct = tfac.makeCompoundType(CompoundType::K_CLASS, str("type_info"));
  // TODO: put this into the 'std' namespace
  // TODO: fill in the proper fields and methods
  type_info_const_ref =
    tfac.makeReferenceType(SL_INIT, makeCVAtomicType(SL_INIT, ct, CV_CONST));

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

  // create declarations for some built-in operators
  // [cppstd 3.7.3 para 2]
  Type *t_void = getSimpleType(SL_INIT, ST_VOID);
  Type *t_voidptr = makePtrType(SL_INIT, t_void);

  // note: my stddef.h typedef's size_t to be 'int', so I just use
  // 'int' directly here instead of size_t
  Type *t_size_t = getSimpleType(SL_INIT, ST_INT);

  // but I do need a class called 'bad_alloc'..
  //   class bad_alloc {};
  CompoundType *dummyCt;
  Type *t_bad_alloc =
    makeNewCompound(dummyCt, scope(), str("bad_alloc"), SL_INIT,
                    TI_CLASS, false /*forward*/);

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
  // for GNU compatibility
  // void *__builtin_next_arg(void *p);
  declareFunction1arg(t_voidptr, "__builtin_next_arg",
                      t_voidptr, "p");

  if (lang.isC99) {
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
    Type *t_int = getSimpleType(SL_INIT, ST_INT);
    //Type *t_unsigned_int = getSimpleType(SL_INIT, ST_UNSIGNED_INT);
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
    declareFunction2arg(t_void, "__builtin_stdarg_start",
                        var__builtin_va_list->type, "__list",
                        t_charconstptr, "__format",
                        FF_NONE, NULL);

    // void __builtin_va_end(__builtin_va_list __list);
    declareFunction1arg(t_void, "__builtin_va_end",
                        var__builtin_va_list->type, "__list");

    // void *__builtin_alloca(int __len);
    declareFunction1arg(t_voidptr, "__builtin_alloca",
                        t_int, "__len");

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
  for (OverloadableOpIter op(table, TABLESIZE(table)); !op.isDone(); op.adv())


void Env::setupOperatorOverloading()
{
  // fill in operatorName[]
  int i;
  for (i=0; i < NUM_OVERLOADABLE_OPS; i++) {
    // debugging hint: if this segfaults, then I forgot to add
    // something to operatorFunctionNames[]
    operatorName[i] = str(operatorFunctionNames[i]);
  }

  // I want to declare some functions, but I don't want them entered
  // into the environment for name lookup; so I'll make a throwaway
  // Scope for them to go into; NOTE that this assumes that Scopes do
  // not delete the Variables they contain (if at some point they
  // acquire that behavior, I can have a method in Scope to clear
  // the Variables without deleting them)
  //
  // update: built-in operators are now suppressed from the
  // environment using FF_BUILTINOP
  //Scope *dummyScope = enterScope(SK_GLOBAL /*close enough*/,
  //  "dummy scope for built-in operator functions");

  // this has to match the typedef in include/stddef.h
  Type *t_ptrdiff_t = getSimpleType(SL_INIT, ST_INT);

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
    addBuiltinUnaryOp(OP_PLUSPLUS, Tr);
    addBuiltinUnaryOp(OP_PLUSPLUS, Tvr);

    // T operator++ (VQ T&, int);
    addBuiltinBinaryOp(OP_PLUSPLUS, Tr, t_int);
    addBuiltinBinaryOp(OP_PLUSPLUS, Tvr, t_int);
  }

  // ------------ 13.6 para 4 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ARITHMETIC_NON_BOOL);
    Type *Tv = getSimpleType(SL_INIT, ST_ARITHMETIC_NON_BOOL, CV_VOLATILE);

    Type *Tr = tfac.makeReferenceType(SL_INIT, T);
    Type *Tvr = tfac.makeReferenceType(SL_INIT, Tv);

    // VQ T& operator-- (VQ T&);
    addBuiltinUnaryOp(OP_MINUSMINUS, Tr);
    addBuiltinUnaryOp(OP_MINUSMINUS, Tvr);

    // T operator-- (VQ T&, int);
    addBuiltinBinaryOp(OP_MINUSMINUS, Tr, t_int);
    addBuiltinBinaryOp(OP_MINUSMINUS, Tvr, t_int);
  }

  // ------------ 13.6 para 5 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_OBJ_TYPE);

    Type *Tp = tfac.makePointerType(SL_INIT, CV_NONE, T);
    Type *Tpv = tfac.makePointerType(SL_INIT, CV_VOLATILE, T);

    Type *Tpr = tfac.makeReferenceType(SL_INIT, Tp);
    Type *Tpvr = tfac.makeReferenceType(SL_INIT, Tpv);

    // T* VQ & operator++ (T* VQ &);
    addBuiltinUnaryOp(OP_PLUSPLUS, Tpr);
    addBuiltinUnaryOp(OP_PLUSPLUS, Tpvr);

    // T* VQ & operator-- (T* VQ &);
    addBuiltinUnaryOp(OP_MINUSMINUS, Tpr);
    addBuiltinUnaryOp(OP_MINUSMINUS, Tpvr);

    // T* operator++ (T* VQ &, int);
    addBuiltinBinaryOp(OP_PLUSPLUS, Tpr, t_int);
    addBuiltinBinaryOp(OP_PLUSPLUS, Tpvr, t_int);

    // T* operator-- (T* VQ &, int);
    addBuiltinBinaryOp(OP_MINUSMINUS, Tpr, t_int);
    addBuiltinBinaryOp(OP_MINUSMINUS, Tpvr, t_int);
  }

  // ------------ 13.6 paras 6 and 7 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_NON_VOID);
    Type *Tp = makePtrType(SL_INIT, T);

    // T& operator* (T*);
    addBuiltinUnaryOp(OP_STAR, Tp);
  }

  // ------------ 13.6 para 8 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_TYPE);
    Type *Tp = makePtrType(SL_INIT, T);

    // T* operator+ (T*);
    addBuiltinUnaryOp(OP_PLUS, Tp);
  }

  // ------------ 13.6 para 9 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);

    // T operator+ (T);
    addBuiltinUnaryOp(OP_PLUS, T);

    // T operator- (T);
    addBuiltinUnaryOp(OP_MINUS, T);
  }

  // ------------ 13.6 para 10 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);
    
    // T operator~ (T);
    addBuiltinUnaryOp(OP_BITNOT, T);
  }

  // ------------ 13.6 para 11 ------------
  // OP_ARROW_STAR is handled specially
  addBuiltinBinaryOp(OP_ARROW_STAR, new ArrowStarCandidateSet);

  // ------------ 13.6 para 12 ------------
  {
    Type *L = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);
    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_ARITHMETIC);

    static OverloadableOp const ops[] = {
      OP_STAR,         // LR operator* (L, R);
      OP_DIV,          // LR operator/ (L, R);
      OP_PLUS,         // LR operator+ (L, R);
      OP_MINUS,        // LR operator- (L, R);
      OP_LESS,         // bool operator< (L, R);
      OP_GREATER,      // bool operator> (L, R);
      OP_LESSEQ,       // bool operator<= (L, R);
      OP_GREATEREQ,    // bool operator>= (L, R);
      OP_EQUAL,        // bool operator== (L, R);
      OP_NOTEQUAL      // bool operator!= (L, R);
    };
    FOREACH_OPERATOR(op, ops) {
      addBuiltinBinaryOp(op, L, R);
    }
  }

  // ------------ 13.6 para 13 ------------
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_OBJ_TYPE);
    Type *Tp = makePtrType(SL_INIT, T);

    // T* operator+ (T*, ptrdiff_t);
    addBuiltinBinaryOp(OP_PLUS, Tp, t_ptrdiff_t);

    // T& operator[] (T*, ptrdiff_t);
    addBuiltinBinaryOp(OP_BRACKETS, Tp, t_ptrdiff_t);

    // T* operator- (T*, ptrdiff_t);
    addBuiltinBinaryOp(OP_MINUS, Tp, t_ptrdiff_t);

    // T* operator+ (ptrdiff_t, T*);
    addBuiltinBinaryOp(OP_PLUS, t_ptrdiff_t, Tp);

    // T& operator[] (ptrdiff_t, T*);
    addBuiltinBinaryOp(OP_BRACKETS, t_ptrdiff_t, Tp);
  }

  // ------------ 13.6 para 14 ------------
  // ptrdiff_t operator-(T,T);
  addBuiltinBinaryOp(OP_MINUS, rvalIsPointer, pointerToObject);

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
  addBuiltinBinaryOp(OP_LESS, rvalFilter, pointerOrEnum);

  // bool operator> (T, T);
  addBuiltinBinaryOp(OP_GREATER, rvalFilter, pointerOrEnum);

  // bool operator<= (T, T);
  addBuiltinBinaryOp(OP_LESSEQ, rvalFilter, pointerOrEnum);

  // bool operator>= (T, T);
  addBuiltinBinaryOp(OP_GREATEREQ, rvalFilter, pointerOrEnum);

  // ------------ 13.6 para 15 & 16 ------------
  // bool operator== (T, T);
  addBuiltinBinaryOp(OP_EQUAL, rvalFilter, pointerOrEnumOrPTM);

  // bool operator!= (T, T);
  addBuiltinBinaryOp(OP_NOTEQUAL, rvalFilter, pointerOrEnumOrPTM);

  // ------------ 13.6 para 17 ------------
  {
    Type *L = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);
    Type *R = getSimpleType(SL_INIT, ST_PROMOTED_INTEGRAL);

    static OverloadableOp const ops[] = {
      OP_MOD,          // LR operator% (L,R);
      OP_BITAND,       // LR operator& (L,R);
      OP_BITXOR,       // LR operator^ (L,R);
      OP_BITOR,        // LR operator| (L,R);
      OP_LSHIFT,       // L operator<< (L,R);
      OP_RSHIFT        // L operator>> (L,R);
    };
    FOREACH_OPERATOR(op, ops) {
      addBuiltinBinaryOp(op, L, R);
    }
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
      addBuiltinBinaryOp(op, Lr, R);
      addBuiltinBinaryOp(op, Lvr, R);
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
  addBuiltinBinaryOp(OP_ASSIGN, para19_20filter, anyType, true /*assignment*/);

  // ------------ 13.6 para 21 ------------
  // 21: +=, -= for pointer type
  {
    Type *T = getSimpleType(SL_INIT, ST_ANY_OBJ_TYPE);

    Type *Tp = tfac.makePointerType(SL_INIT, CV_NONE, T);
    Type *Tpv = tfac.makePointerType(SL_INIT, CV_VOLATILE, T);

    Type *Tpr = tfac.makeReferenceType(SL_INIT, Tp);
    Type *Tpvr = tfac.makeReferenceType(SL_INIT, Tpv);

    // T* VQ & operator+= (T* VQ &, ptrdiff_t);
    addBuiltinBinaryOp(OP_PLUSEQ, Tpr, t_ptrdiff_t);
    addBuiltinBinaryOp(OP_PLUSEQ, Tpvr, t_ptrdiff_t);

    // T* VQ & operator-= (T* VQ &, ptrdiff_t);
    addBuiltinBinaryOp(OP_MINUSEQ, Tpr, t_ptrdiff_t);
    addBuiltinBinaryOp(OP_MINUSEQ, Tpvr, t_ptrdiff_t);
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
      addBuiltinBinaryOp(op, Lr, R);
      addBuiltinBinaryOp(op, Lvr, R);
    }
  }

  // ------------ 13.6 para 23 ------------
  // bool operator! (bool);
  addBuiltinUnaryOp(OP_NOT, t_bool);

  // bool operator&& (bool, bool);
  addBuiltinBinaryOp(OP_AND, t_bool, t_bool);

  // bool operator|| (bool, bool);
  addBuiltinBinaryOp(OP_OR, t_bool, t_bool);

  // ------------ 13.6 para 24 ------------
  // 24: ?: on arithmetic types

  // ------------ 13.6 para 25 ------------
  // 25: ?: on pointer and ptr-to-member types

  //exitScope(dummyScope);

  // the default constructor for ArrayStack will have allocated 10
  // items in each array; go back and resize them to their current
  // length (since that won't change after this point)
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

  ft->doneParams();

  Variable *var = makeVariable(SL_INIT, str(funcName), ft, DF_NONE);
  if (flags & FF_BUILTINOP) {
    // don't add built-in operator functions to the environment
  }
  else {
    addVariable(var);
  }

  return var;
}


// this declares a function that accepts any # of arguments,
// and returns 'int'
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


FunctionType *Env::makeUndeclFuncType()
{
  // don't need a clone type here as getSimpleType() calls
  // makeCVAtomicType() which in oink calls new.
  Type *ftRet = tfac.getSimpleType(loc(), ST_INT, CV_NONE);
  FunctionType *ft = makeFunctionType(loc(), ftRet);
  Variable *p = makeVariable(loc(),
                             NULL /*name*/,
                             tfac.getSimpleType(loc(), ST_ELLIPSIS, CV_NONE),
                             DF_PARAMETER);
  ft->addParam(p);
  ft->doneParams();
  return ft;
}


Variable *Env::makeUndeclFuncVar(StringRef name)
{
  return createDeclaration
    (loc(), name,
     makeUndeclFuncType(), DF_FORWARD,
     globalScope(), NULL /*enclosingClass*/,
     NULL /*prior*/, NULL /*overloadSet*/);
}


FunctionType *Env::beginConstructorFunctionType(SourceLoc loc, CompoundType *ct)
{
  FunctionType *ft = makeFunctionType(loc, makeType(loc, ct));
  ft->flags |= FF_CTOR;
  // asymmetry with makeDestructorFunctionType(): this must be done by the client
//    ft->doneParams();
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
  ft->doneParams();
  return ft;
}


Scope *Env::enterScope(ScopeKind sk, char const *forWhat)
{
  // propagate the 'curFunction' field
  Function *f = scopes.first()->curFunction;
  Scope *newScope = new Scope(sk, getChangeCount(), loc());
  setParentScope(newScope);
  scopes.prepend(newScope);
  newScope->curFunction = f;

  TRACE("env", locStr() << ": entered " << newScope->desc() << " for " << forWhat);

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

  TRACE("env", locStr() << ": exited " << s->desc());
  Scope *f = scopes.removeFirst();
  xassert(s == f);
  delete f;
}


void Env::extendScope(Scope *s)
{
  TRACE("env", locStr() << ": extending " << s->desc());

  Scope *prevScope = scope();
  scopes.prepend(s);
  s->curLoc = prevScope->curLoc;

  s->openedScope(*this);
}

void Env::retractScope(Scope *s)
{
  TRACE("env", locStr() << ": retracting " << s->desc());

  s->closedScope();

  Scope *first = scopes.removeFirst();
  xassert(first == s);
  // we don't own 's', so don't delete it
}


void Env::debugPrintScopes()
{
  cout << "Scopes and variables, beginning with innermost" << endl;
  for (int i=0; i<scopes.count(); ++i) {
    Scope *s = scopes.nth(i);
    cout << "scope " << i << ", " << s->desc() << endl;
    for (StringSObjDict<Variable>::IterC iter = s->getVariableIter();
         !iter.isDone();
         iter.next()) {
      Variable *value = iter.value();
      cout << "  " << iter.key()
           << ": " << value->toString()
           << stringf(" (%p)", value);
//        cout << "value->serialNumber " << value->serialNumber;
      cout << endl;
    }
  }
}


// this should be a rare event
void Env::refreshScopeOpeningEffects()
{
  TRACE("env", "refreshScopeOpeningEffects");

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
    if (s->isTemplateParamScope() ||         // skip template scopes
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


Type *Env::declareEnum(SourceLoc loc /*...*/, EnumType *et)
{
  Type *ret = makeType(loc, et);
  if (et->name) {
    if (!addEnum(et)) {
      error(stringc << "multiply defined enum `" << et->name << "'");
    }

    // make the implicit typedef
    Variable *tv = makeVariable(loc, et->name, ret, DF_TYPEDEF | DF_IMPLICIT);
    et->typedefVar = tv;
    if (!addVariable(tv)) {
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

                                                                 
// return true if the semantic template arguments in 'args' are not
// all concrete
bool containsTypeVariables(ASTList<TemplateArgument> const &args)
{
  FOREACH_ASTLIST(TemplateArgument, args, iter) {
    if (iter.data()->sarg.containsVariables()) {
      return true;
    }
  }
  return false;     // all are concrete
}


// select either the primary or one of the specializations, based
// on the supplied arguments; these arguments will likely contain
// type variables; e.g., given "T*", select specialization C<T*>;
// return NULL if no matching definition found
Variable *Env::getPrimaryOrSpecialization
  (TemplateInfo *tinfo, SObjList<STemplateArgument> const &sargs)
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
  SFOREACH_OBJLIST(STemplateArgument, sargs, argIter) {
    STemplateArgument const *arg = argIter.data();

    if (arg->isType()) {
      if (!arg->getType()->isTypeVariable()) {
        allToplevelVars = false;
      }
    }
    else {
      // we do not (yet) have a proper way of distinguishing object
      // placeholders from true concrete objects in template
      // arguments..... but our hacky approach is that STA_REFERENCE
      // is abstract and others are concrete...
      if (arg->kind != STemplateArgument::STA_REFERENCE) {
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
    if (spec->templateInfo()->equalArguments(tfac, sargs)) {
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
  // lookup the name
  Variable *qualVar = lookupOneQualifier_bareName(startingScope, qualifier, lflags);
  if (!qualVar) {
    return NULL;     // error already reporeted
  }

  // refine using the arguments, and yield a Scope
  return lookupOneQualifier_useArgs(qualVar, qualifier, dependent, anyTemplates, lflags);
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
    xassert(qualifier->targs.isEmpty());

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
  lflags |= LF_TYPES_NAMESPACES;
  Variable *qualVar = startingScope==NULL?
    lookupVariable(qual, lflags) :
    startingScope->lookupVariable(qual, *this, lflags);
  if (!qualVar) {
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
    return NULL;
  }

  // this is what LF_TYPES_NAMESPACES means
  xassert(qualVar->hasFlag(DF_TYPEDEF) || qualVar->hasFlag(DF_NAMESPACE));

  return qualVar;
}


// Second half of 'lookupOneQualifier': use the template args.
//
// TODO: The caller has already put the template arguments into an
// SObjList<STemplateArgument>; accept a reference to those instead of
// making another list myself.
Scope *Env::lookupOneQualifier_useArgs(
  Variable *qualVar,             // bare name scope Variable
  PQ_qualifier const *qualifier, // will look up the qualifier on this name
  bool &dependent,               // set to true if we have to look inside a TypeVariable
  bool &anyTemplates,            // set to true if we look in uninstantiated templates
  LookupFlags lflags)
{
  StringRef qual = qualifier->qualifier;

  if (!qualVar) {
    // just propagating earlier error
    return NULL;
  }

  // case 1: qualifier refers to a type
  if (qualVar->hasFlag(DF_TYPEDEF)) {
    // check for a special case: a qualifier that refers to
    // a template parameter
    if (qualVar->type->isTypeVariable()) {
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
    if (qualVar->hasFlag(DF_SELFNAME)) {
      // don't check anything, assume it's a reference to the
      // class I'm in (testcase: t0168.cc)
    }

    // check template argument compatibility
    else if (qualifier->targs.isNotEmpty()) {
      if (!ct->isTemplate()) {
        error(stringc
          << "class `" << qual << "' isn't a template");
        // recovery: use the scope anyway
      }   

      else {
        // TODO: maybe put this back in
        //ct = checkTemplateArguments(ct, qualifier->targs);
        // UPDATE: dsw: I think the code below will fail if this is
        // has not been done, so I typecheck the PQName now in
        // Declarator::mid_tcheck() before this is called

        // obtain a list of semantic arguments
        SObjList<STemplateArgument> sargs;
        if (!templArgsASTtoSTA(qualifier->targs, sargs)) {
          return ct;      // error already reported; recovery: use primary
        }

        if ((lflags & LF_DECLARATOR) &&
            containsTypeVariables(qualifier->targs)) {    // fix t0185.cc?  seems to...
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
            // 8/07/04: I think the test above, 'containsTypeVariables',
            // has resolved this issue.
            error(stringc << "cannot find template primary or specialization `"
                          << qualifier->toString() << "'",
                  EF_DISAMBIGUATES);
            return ct;     // recovery: use the primary
          }

          return primaryOrSpec->type->asCompoundType();
        }

        // if the template arguments are not concrete, then this is
        // a dependent name, e.g. "C<T>::foo"
        if (containsTypeVariables(qualifier->targs)) {
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

        // NOTE: don't delete this yet.  If you replace the above code
        // with this code, then in/t0216.cc should fail, but it
        // doesn't.  There is an assertion in Variable
        // *Env::lookupPQVariable(PQName const *name, LookupFlags
        // flags, Scope *&scope) right after the (only) call to
        // lookupPQVariable_internal() that should fail for
        // in/t0216.cc but 1) it doesn't anyway, something I don't
        // understand, since the lookup to foo() at the bottom of
        // t0216.cc doesn't find the instantiated one, and 2) other
        // tests fail that assertion also.
//          SObjList<STemplateArgument> qsargs;
//          templArgsASTtoSTA(qualifier->targs, qsargs);
//          // this reinterpret_cast trick seems to be what Scott wants
//          // for this lack of owner/serf polymorphism problem
//          xassert(ct->templateInfo()->isPrimary());
//          Variable *inst = getInstThatMatchesArgs
//            (ct->templateInfo(), reinterpret_cast<ObjList<STemplateArgument>&>(qsargs));
//          if (inst) {
//            ct = inst->type->asCompoundType();
//          }
//          // otherwise, we just stay with the primary.
//          // FIX: in that case, we should assert that the args all match
//          // what they would if they were for a primary; such as a
//          // typevar arg for a type template parameter
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
    if (qualifier->targs.isNotEmpty()) {
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

// NOTE: It is *not* the job of this function to do overload
// resolution!  If the client wants that done, it must do it itself,
// *after* doing the lookup.
Variable *Env::lookupPQVariable_internal(PQName const *name, LookupFlags flags,
                                         Scope *&scope)
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
        if (flags & LF_TYPENAME) {
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

    var = scope->lookupVariable(name->getName(), *this, flags | LF_QUALIFIED);
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
    if (!final->isPQ_template()) {
      flags |= LF_SELFNAME;
    }

    var = lookupVariable(name->getName(), flags, scope);
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

  // compare the name's template status to whether template
  // arguments were supplied; note that you're only *required*
  // to pass arguments for template classes, since template
  // functions can infer their arguments
  if (var->isTemplate(false /*considerInherited*/)) {
    // dsw: I need a way to get the template without instantiating
    // it.
    if (flags & LF_TEMPL_PRIMARY) {
      return var;
    }

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
      return NULL;
    }

    if (var->isTemplateClass()) {
      // apply the template arguments to yield a new type based
      // on the template

      // obtain a list of semantic arguments
      PQ_template const *tqual = final->asPQ_templateC();
      SObjList<STemplateArgument> sargs;
      if (!templArgsASTtoSTA(tqual->args, sargs)) {
        return NULL;       // error already reported
      }

      // if any of the arguments are dependent, then the whole
      // instantiation is dependent
      if (hasDependentArgs(sargs)) {
        return dependentTypeVar;
      }

      // if the template arguments are not concrete, then create
      // a PsuedoInstantiation
      if (containsTypeVariables(sargs)) {
        PseudoInstantiation *pi =
          createPseudoInstantiation(var->type->asCompoundType(), sargs);
        xassert(pi->typedefVar);
        return pi->typedefVar;
      }

      return instantiateClassTemplate(loc(), var, sargs);
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

        // make a list of semantic template args
        SObjList<STemplateArgument> sargs;
        if (!templArgsASTtoSTA(final->asPQ_templateC()->args, sargs)) {
          return NULL;      // error already reporeted
        }

        return instantiateFunctionTemplate(loc(), var, sargs);
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
  Variable *var = lookupPQVariable_internal(name, flags, scope);

  // in normal typechecking, nothing should look up to a variable from
  // template-definition never-never land
  if ( !(flags & LF_TEMPL_PRIMARY) &&
       getTemplTcheckMode() == TTM_1NORMAL &&
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
  if (flags & LF_INNER_ONLY) {
    // here as in every other place 'innerOnly' is true, I have
    // to skip non-accepting scopes since that's not where the
    // client is planning to put the name
    foundScope = acceptingScope();
    return foundScope->lookupVariable(name, *this, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();
    if ((flags & LF_SKIP_CLASSES) && s->isClassScope()) {
      continue;
    }

    Variable *v = s->lookupVariable(name, *this, flags);
    if (v) {
      foundScope = s;
      return v;
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

    ret = scope->lookupCompound(name->getName(), flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no class/struct/union called `"
        << name->getName() << "'",
        EF_DISAMBIGUATES);
      return NULL;
    }

    return ret;
  }
  else {
    ret = lookupCompound(name->getName(), flags);
  }

  // apply template arguments if any
  if (ret) {
    Variable *var = applyPQNameTemplateArguments(ret->typedefVar, name, flags);
    if (var && var->type->isCompoundType()) {
      ret = var->type->asCompoundType();
    }
  }

  return ret;
}

CompoundType *Env::lookupCompound(StringRef name, LookupFlags flags)
{
  if (flags & LF_INNER_ONLY) {
    return acceptingScope()->lookupCompound(name, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    CompoundType *ct = iter.data()->lookupCompound(name, flags);
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

    EnumType *ret = scope->lookupEnum(name->getName(), flags);
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
    return acceptingScope()->lookupEnum(name, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    EnumType *et = iter.data()->lookupEnum(name, flags);
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
           s->scopeKind == SK_TEMPLATE_PARAMS )) {
      break;
    }

    if (s->isTemplateParamScope()) {
      // found some parameters that we need to record in a TemplateInfo
      if (!ret) {
        ret = new TemplateInfo(loc());
      }

      // are these as-yet unassociated params?
      if (!s->parameterizedPrimary) {
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
          = new InheritedTemplateParams(s->parameterizedPrimary->type->asCompoundType());
        itp->params.appendAll(s->templateParams);

        if (s->templateParams.isNotEmpty()) {
          couldBeCompleteSpecMember = false;
        }

        // stash them in 'ret', prepending so as we work from innermost
        // to outermost, so the last one prepended will be the outermost,
        // and hence first in the list when we're done
        ret->inheritedParams.prepend(itp);

        TRACE("templateParams", "inherited " << itp->paramsToCString() <<
                                " from " << s->parameterizedPrimary->name);
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


// This function was originally created to support the elaboration
// phase, as it happened interleaved with type checking.  Now that
// elaboration is its own pass, we could get rid of the notion of
// shadow typedefs entirely.
void Env::makeShadowTypedef(Scope *scope, Variable *tv)   // "tv" = typedef var
{
  xassert(tv->hasFlag(DF_TYPEDEF));

  if (disambErrorsSuppressChanges()) {
    // we're in a situation where environment changes are bad.. so let
    // the type be silently masked
    TRACE("env", "not actually making shadow typedef variable for `"
          << tv->name << "' because there are disambiguating errors");
    return;
  }

  // make a name that can't collide with a user identifier
  StringRef shadowName = str(stringc << tv->name << "-shadow");
  TRACE("env", "making shadow typedef variable `" << shadowName <<
        "' for `" << tv->name <<
        "' because a non-typedef var is masking it");

  // shadow shouldn't exist
  xassert(!scope->lookupVariable(shadowName, *this, LF_INNER_ONLY));
  
  // modify the variable to get the new name, and add it to the scope
  tv->name = shadowName;
  scope->addVariable(tv);
  
  // at least for now, don't call registerVariable or addedNewVariable,
  // since it's a weird situation
}


bool Env::isShadowTypedef(Variable *tv)
{
  // this isn't a common query, so it's ok if it's a little slow
  return suffixEquals(tv->name, "-shadow");
}


Type *Env::makeNewCompound(CompoundType *&ct, Scope * /*nullable*/ scope,
                           StringRef name, SourceLoc loc,
                           TypeIntr keyword, bool forward)
{
  ct = tfac.makeCompoundType((CompoundType::Keyword)keyword, name);
  if (scope) {
    setParentScope(ct, scope);
  }

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

  // make the implicit typedef
  Type *ret = makeType(loc, ct);
  Variable *tv = makeVariable(loc, name, ret, DF_TYPEDEF | DF_IMPLICIT);
  ct->typedefVar = tv;

  // transfer template parameters
  { 
    TemplateInfo *ti = takeCTemplateInfo();
    if (ti) {
      ct->setTemplateInfo(ti);
      
      // it turns out any time I pass in a non-NULL scope, I'm in
      // the process of making a primary
      if (scope) {
        if (this->scope()->isTemplateParamScope()) {
          this->scope()->setParameterizedPrimary(ct->typedefVar);
        }

        // set its defnScope, which is used during instantiation
        ti->defnScope = scope;
      }
    }
  }

  if (name && scope) {
    scope->registerVariable(tv);
    if (!scope->addVariable(tv)) {
      // this isn't really an error, because in C it would have
      // been allowed, so C++ does too [ref?]
      //return env.error(stringc
      //  << "implicit typedef associated with " << ct->keywordAndName()
      //  << " conflicts with an existing typedef or variable",
      //  true /*disambiguating*/);
      makeShadowTypedef(scope, tv);
    }
    else {
      addedNewVariable(scope, tv);
    }
  }

  // also add the typedef to the class' scope
  if (name && lang.compoundSelfName) {
    Type *selfType;
    if (tv->templateInfo() && tv->templateInfo()->hasParameters()) {
      // the self type should be a PseudoInstantiation, not the raw template
      selfType = pseudoSelfInstantiation(ct, CV_NONE);
    }
    else {
      // just the class' type
      selfType = tfac.cloneType(ret);
    }
    Variable *selfVar = makeVariable(loc, name, selfType, DF_TYPEDEF | DF_SELFNAME);
    ct->addUniqueVariable(selfVar);
    addedNewVariable(ct, selfVar);
  }

  return ret;
}


// -------- diagnostics --------
Type *Env::error(SourceLoc L, char const *msg, ErrorFlags eflags)
{
  string instLoc = instLocStackString();
  TRACE("error", ((eflags & EF_DISAMBIGUATES)? "[d] " : "")
              << toString(L) << ": " << msg << instLoc);

  bool report = (eflags & EF_DISAMBIGUATES) || (eflags & EF_STRONG) || (!disambiguateOnly);
  if (report) {
    errors.addError(new ErrorMsg(L, msg, eflags, instLoc));
  }

  return errorType();
}

Type *Env::errorType()
{
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}

Type *Env::error(char const *msg, ErrorFlags eflags)
{
  return error(loc(), msg, eflags);
}


Type *Env::warning(char const *msg)
{
  string instLoc = instLocStackString();
  TRACE("error", "warning: " << msg << instLoc);
  if (!disambiguateOnly) {
    errors.addError(new ErrorMsg(loc(), msg, EF_WARNING, instLoc));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}


Type *Env::unimp(char const *msg)
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


Type *Env::error(Type *t, char const *msg)
{
  if (t->isSimple(ST_DEPENDENT)) {
    // no report, propagate dependentness
    return t;
  }

  // TODO: remove containsTypeVariables() from this..
  if (t->containsErrors() ||
      t->containsTypeVariables()) {   // hack until template stuff fully working
    // no report
    return getSimpleType(SL_UNKNOWN, ST_ERROR);
  }
  else {
    // report; clashes never disambiguate
    return error(msg, EF_NONE);
  }
}


bool Env::setDisambiguateOnly(bool newVal)
{
  bool ret = disambiguateOnly;
  disambiguateOnly = newVal;
  return ret;
}


Type *Env::implicitReceiverType()
{
  // TODO: this appears to be wrong in several respects:
  //   - we should be checking scope()->curFunction, not
  //     an enclosing class scope
  //   - static methods (both caller and callee) might need
  //     special treatment
  //   - when the caller is a non-static method, the cv-flags
  //     for the receiver object must match those of 'this'

  CompoundType *encScope = enclosingClassScope();
  if (!encScope) {
    // there's no 'this' object to use as an implicit receiver
    return NULL;
  }
  else {
    // we're in the scope of some class, so the call could be a
    // method call, passing 'this' as the receiver; compute the type
    // of that receiver argument
    return tfac.makeTypeOf_receiver(loc() /*?*/, encScope, CV_NONE, NULL);
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
    // TODO: save this somewhere instead of making it over and over
    Type *selfType = pseudoSelfInstantiation(nat->asCompoundType(), cv);
    recType = makeReferenceType(loc, selfType);
  }
  else {
    // this doesn't use 'implicitReceiverType' because of the warnings above
    recType = tfac.makeTypeOf_receiver(loc, nat, cv, syntax);
  }

  return makeVariable(loc, receiverName, recType, DF_PARAMETER);
}


void Env::addBuiltinUnaryOp(OverloadableOp op, Type *x)
{
  builtinUnaryOperator[op].push(createBuiltinUnaryOp(op, x));
}

Variable *Env::createBuiltinUnaryOp(OverloadableOp op, Type *x)
{
  Type *t_void = getSimpleType(SL_INIT, ST_VOID);

  Variable *v = declareFunction1arg(
    t_void /*irrelevant*/, operatorName[op],
    x, "x",
    FF_BUILTINOP);
  v->setFlag(DF_BUILTIN);

  return v;
}


void Env::addBuiltinBinaryOp(OverloadableOp op, Type *x, Type *y)
{
  addBuiltinBinaryOp(op, new PolymorphicCandidateSet(
    createBuiltinBinaryOp(op, x, y)));
}

void Env::addBuiltinBinaryOp(OverloadableOp op, PredicateCandidateSet::PreFilter pre,
                                                PredicateCandidateSet::PostFilter post,
                                                bool isAssignment)
{
  if (isAssignment) {
    addBuiltinBinaryOp(op, new AssignmentCandidateSet(pre, post));
  }
  else {
    addBuiltinBinaryOp(op, new PredicateCandidateSet(pre, post));
  }
}

void Env::addBuiltinBinaryOp(OverloadableOp op, CandidateSet * /*owner*/ cset)
{
  builtinBinaryOperator[op].push(cset);
}


Variable *Env::createBuiltinBinaryOp(OverloadableOp op, Type *x, Type *y)
{
  // PLAN:  Right now, I just leak a bunch of things.  To fix
  // this, I want to have the Env maintain a pool of Variables
  // that represent built-in operators during overload
  // resolution.  I ask the Env for operator-(T,T) with a
  // specific T, and it rewrites an existing one from the pool
  // for me.  Later I give all the pool elements back.

  Type *t_void = getSimpleType(SL_INIT, ST_VOID);

  Variable *v = declareFunction2arg(
    t_void /*irrelevant*/, operatorName[op],
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
bool Env::almostEqualTypes(Type /*const*/ *t1, Type /*const*/ *t2)
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

  // no exception: strict equality (well, toplevel param cv can differ)
  Type::EqFlags eqFlags = Type::EF_IGNORE_PARAM_CV;

  if (lang.allow_KR_ParamOmit) {
    eqFlags |= Type::EF_ALLOW_KR_PARAM_OMIT;
  }

  return equalOrIsomorphic(t1, t2, eqFlags);
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
      newFt->doneParams();

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

  // check for overloading
  OverloadSet *overloadSet = getOverloadForDeclaration(prior, type);

  // are we heading for a conflict with an alias?
  if (prior &&
      prior->usingAlias &&
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
  if (origVar->templateInfo()) {
    newVar->setTemplateInfo(new TemplateInfo(*origVar->templateInfo()));
  }

  if (newVar == prior) {
    // found that the name/type named an equivalent entity
    return;
  }

  // hook 'newVar' up as an alias for 'origVar'
  newVar->usingAlias = origVar->skipAlias();    // don't make long chains

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
  Variable *prior = scope->lookupVariable(name, *this, LF_INNER_ONLY);
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
    if (!equalOrIsomorphic(iterft, ft, 
           Type::EF_STAT_EQ_NONSTAT | Type::EF_IGNORE_IMPLICIT)) {
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
  return equalOrIsomorphic(ft1, ft2, Type::EF_SIGNATURE);
}


// if the types are concrete, compare for equality; if at least one
// is not, compare for isomorphism
//
// The parameters here should be const, but MatchTypes doesn't
// make claims about constness so I just dropped const from here ...
bool Env::equalOrIsomorphic(Type *a, Type *b, Type::EqFlags eflags)
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
    // potential overloading situation; get the two function types
    FunctionType *priorFt = prior->type->asFunctionType();
    FunctionType *specFt = type->asFunctionType();

    // can only be an overloading if their signatures differ,
    // or it's a conversion operator and they differ at all
    if (prior->name == conversionOperatorName?
          !equalOrIsomorphic(priorFt, specFt) :         // conversion: equality check
          !equivalentSignatures(priorFt, specFt)) {     // non-conversion: signature check
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
  // is the parameter list an ellipsis?
  SObjList<Variable> &params = ft->params;
  if (params.count() != 1) return false;
  if (!params.first()->type->isEllipsis()) return false;
  return true;
  
  // sm: A true FunctionType should never have an ST_ELLIPSIS parameter.
  // Instead, mark the FunctionType as FF_VARARGS.
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
        error(prior->type, stringc
          << "duplicate definition for `" << name
          << "' of type `" << prior->type->toString()
          << "'; previous at " << toString(prior->loc));

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
      if (enclosingClass &&
          !(dflags & DF_INLINE_DEFN) &&
          !prior->isImplicitTypedef()) {    // allow implicit typedef to be hidden
        if (!prior->usingAlias) {
          error(stringc
            << "duplicate member declaration of `" << name
            << "' in " << enclosingClass->keywordAndName()
            << "; previous at " << toString(prior->loc));
          goto makeDummyVar;
        }
        else {
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
          prior->usingAlias = NULL;
          scope->registerVariable(prior);
          return prior;
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
    else {
      // if the previous guy was an implicit typedef, then as a
      // special case allow it, and arrange for the environment
      // to replace the implicit typedef with the variable being
      // declared here
      if (prior->isImplicitTypedef()) {
        TRACE("env",    "replacing implicit typedef of " << prior->name
                     << " at " << prior->loc << " with new decl at "
                     << loc);
        forceReplace = true;

        // for support of the elaboration module, we don't want to lose
        // the previous name altogether; make a shadow
        makeShadowTypedef(scope, prior);

        goto noPriorDeclaration;
      }
      else {
        // this message reports two declarations which declare the same
        // name, but their types are different; we only jump here *after*
        // ruling out the possibility of function overloading

        // dsw: in K&R C sometimes what happens is that a function is
        // called and then later declared; at the function call site a
        // declaration is invented with type 'int (...)' but the real
        // declaration is likely to collide with that here.  We don't
        // try to back-patch and do anything clever or sound, we just
        // turn the error into a warning so that the file can go
        // through; this is an unsoundness
        if (lang.allowCallToUndeclFunc &&
            prior->hasFlag(DF_FORWARD) &&
            prior->type->isFunctionType() &&
            isImplicitKandRFuncType(prior->type->asFunctionType())) {
          // we allow the prior declaration to remain
          warning(stringc
                  << "prior declaration of function `" << name
                  << "' at " << prior->loc
                  << " had type `" << prior->type->toString()
                  << "', but this one uses `" << type->toString() << "'"
                  << " This is most likely due to the prior declaration being implied "
                  << "by a call to a function before it was declared.  "
                  << "Keeping the implied, weaker declaration; THIS IS UNSOUND.");
        } else {
          error(type, stringc
                << "prior declaration of `" << name
                << "' at " << prior->loc
                << " had type `" << prior->type->toString()
                << "', but this one uses `" << type->toString() << "'");
          goto makeDummyVar;
        }
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

    // TODO: if 'type' refers to a function type, and it has
    // some default arguments supplied, then:
    //   - it should only be adding new defaults, not overriding
    //     any from a previous declaration
    //   - the new defaults should be merged into the type retained
    //     in 'prior->type', so that further uses in this translation
    //     unit will have the benefit of the default arguments
    //   - the resulting type should have all the default arguments
    //     contiguous, and at the end of the parameter list
    // reference: cppstd, 8.3.6

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
      !var->usingAlias) {
    return var;
  }

  if (!var->overload && !var->usingAlias->overload) {
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
  (CompoundType *ct, SObjList<STemplateArgument> const &args)
{                                                            
  TRACE("pseudo", "creating " << ct->name << sargsToString(args));

  // make the object itself
  PseudoInstantiation *pi = new PseudoInstantiation(ct);

  // attach the template arguments
  SFOREACH_OBJLIST(STemplateArgument, args, iter) {
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
      if (!s->parameterizedPrimary) {
        lastUnassoc = s;
      }
      else if (s->parameterizedPrimary == bareQualifierVar) {
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
  else {
    // well, may as well cement the association now
    lastUnassoc->setParameterizedPrimary(bareQualifierVar);

    TRACE("templateParams",
      "associated " << paramsToCString(lastUnassoc->templateParams) <<
      " with " << bareQualifierVar->name);
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
    TRACE("env", "temporarily removing " << s->desc());
    dest.prepend(s);
  }
}

// Undo the effects of 'removeScopesInside'.
void Env::restoreScopesInside(ObjList<Scope> &src, Scope *bound)
{
  xassert(scopes.first() == bound);
  while (src.isNotEmpty()) {
    Scope *s = src.removeFirst();
    TRACE("env", "restoring " << s->desc());
    scopes.prepend(s);
  }
}


bool Env::ensureCompleteType(char const *action, Type *type)
{
  if (type->isCompoundType()) {
    CompoundType *ct = type->asCompoundType();

    // maybe it's a template we can instantiate?
    if (!ct->isComplete() && ct->isInstantiation()) {
      instantiateClassBody(ct->typedefVar);
    }

    if (!ct->isComplete()) {
      error(stringc << "attempt to " << action << 
                       " incomplete type `" << type->toString() << "'");
      return false;
    }
  }

  return true;
}


// EOF
