// cc_env.cc            see license.txt for copyright and terms of use
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys
#include "ckheap.h"      // heapCheck
#include "strtable.h"    // StringTable
#include "cc_lang.h"     // CCLang
#include "strutil.h"     // suffixEquals


inline ostream& operator<< (ostream &os, SourceLoc sl)
  { return os << toString(sl); }


// --------------------- Env -----------------
                                 
int throwClauseSerialNumber = 0; // don't make this a member of Env

Env::Env(StringTable &s, CCLang &L, TypeFactory &tf, TranslationUnit *tunit0)
  : scopes(),
    disambiguateOnly(false),
    anonTypeCounter(1),
    ctorFinished(false),

    disambiguationNestingLevel(0),
    errors(),
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
    thisName(str("this")),
    quote_C_quote(str("\"C\"")),
    quote_C_plus_plus_quote(str("\"C++\"")),

    // these are done below because they have to be declared as functions too
    special_getStandardConversion(NULL),
    special_getImplicitConversion(NULL),
    special_testOverload(NULL),
    special_computeLUB(NULL),

    dependentTypeVar(NULL),
    dependentVar(NULL),
    errorTypeVar(NULL),
    errorVar(NULL),

    var__builtin_constant_p(NULL),

    // operatorName[] initialized below

    // builtin{Un,Bin}aryOperator[] start as arrays of empty
    // arrays, and then have things added to them below

    tunit(tunit0),

    currentLinkage(quote_C_plus_plus_quote),

    doOverload(tracingSys("doOverload")),
    doOperatorOverload(tracingSys("doOperatorOverload")),
    collectLookupResults(NULL),
    doElaboration(true),

    tempSerialNumber(0),
    e_newSerialNumber(0)

    // made this global
//      throwClauseSerialNumber(0),
{
  // slightly less verbose
  //#define HERE HERE_SOURCELOC     // old
  #define HERE SL_INIT              // decided I prefer this

  // create first scope
  SourceLoc emptyLoc = SL_UNKNOWN;
  {                               
    // among other things, SK_GLOBAL causes Variables inserted into
    // this scope to acquire DF_GLOBAL
    Scope *s = new Scope(SK_GLOBAL, 0 /*changeCount*/, emptyLoc);
    scopes.prepend(s);
  }

  // create the typeid type
  CompoundType *ct = tfac.makeCompoundType(CompoundType::K_CLASS, str("type_info"));
  // TODO: put this into the 'std' namespace
  // TODO: fill in the proper fields and methods
  type_info_const_ref = 
    tfac.makeRefType(HERE, makeCVAtomicType(HERE, ct, CV_CONST));

  dependentTypeVar = makeVariable(HERE, str("<dependentTypeVar>"),
                                  getSimpleType(HERE, ST_DEPENDENT), DF_TYPEDEF);

  dependentVar = makeVariable(HERE, str("<dependentVar>"),
                              getSimpleType(HERE, ST_DEPENDENT), DF_NONE);

  errorTypeVar = makeVariable(HERE, str("<errorTypeVar>"),
                              getSimpleType(HERE, ST_ERROR), DF_TYPEDEF);

  // this is *not* a typedef, because I use it in places that I
  // want something to be treated as a variable, not a type
  errorVar = makeVariable(HERE, str("<errorVar>"),
                          getSimpleType(HERE, ST_ERROR), DF_NONE);

  // create declarations for some built-in operators
  // [cppstd 3.7.3 para 2]
  Type *t_void = getSimpleType(HERE, ST_VOID);
  Type *t_voidptr = makePtrType(HERE, t_void);

  // note: my stddef.h typedef's size_t to be 'int', so I just use
  // 'int' directly here instead of size_t
  Type *t_size_t = getSimpleType(HERE, ST_INT);

  // but I do need a class called 'bad_alloc'..
  //   class bad_alloc;
  CompoundType *dummyCt;
  Type *t_bad_alloc =
    makeNewCompound(dummyCt, scope(), str("bad_alloc"), HERE,
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

  // for GNU compatibility
  // void *__builtin_next_arg(void *p);
  declareFunction1arg(t_voidptr, "__builtin_next_arg",
                      t_voidptr, "p");

  // dsw: This is a form, not a function, since it takes an expression
  // AST node as an argument; however, I need a function that takes no
  // args as a placeholder for it sometimes.
  var__builtin_constant_p = declareSpecialFunction("__builtin_constant_p");

  // for testing various modules
  special_getStandardConversion = declareSpecialFunction("__getStandardConversion")->name;
  special_getImplicitConversion = declareSpecialFunction("__getImplicitConversion")->name;
  special_testOverload = declareSpecialFunction("__testOverload")->name;
  special_computeLUB = declareSpecialFunction("__computeLUB")->name;

  setupOperatorOverloading();

  #undef HERE

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

    Type *Tr = tfac.makeRefType(SL_INIT, T);
    Type *Tvr = tfac.makeRefType(SL_INIT, Tv);

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

    Type *Tr = tfac.makeRefType(SL_INIT, T);
    Type *Tvr = tfac.makeRefType(SL_INIT, Tv);

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

    Type *Tp = tfac.makePointerType(SL_INIT, PO_POINTER, CV_NONE, T);
    Type *Tpv = tfac.makePointerType(SL_INIT, PO_POINTER, CV_VOLATILE, T);

    Type *Tpr = tfac.makeRefType(SL_INIT, Tp);
    Type *Tpvr = tfac.makeRefType(SL_INIT, Tpv);

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

    Type *Lr = tfac.makeRefType(SL_INIT, L);
    Type *Lvr = tfac.makeRefType(SL_INIT, Lv);

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

    Type *Tp = tfac.makePointerType(SL_INIT, PO_POINTER, CV_NONE, T);
    Type *Tpv = tfac.makePointerType(SL_INIT, PO_POINTER, CV_VOLATILE, T);

    Type *Tpr = tfac.makeRefType(SL_INIT, Tp);
    Type *Tpvr = tfac.makeRefType(SL_INIT, Tpv);

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

    Type *Lr = tfac.makeRefType(SL_INIT, L);
    Type *Lvr = tfac.makeRefType(SL_INIT, Lv);
    
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
  // sm: Generally, I don't like to do things that might throw
  // exceptions in destructors, b/c if we're unwinding the stack then
  // it escalates the failure (thereby complicating diagnosis of the
  // original problem, and making recovery impossible). 
  //xassert(fullExpressionAnnotStack.isEmpty());
  if (!fullExpressionAnnotStack.isEmpty()) {
    cout << "BUG: fullExpressionAnnotStack isn't empty!\n";
  }

  // delete the scopes one by one, so we can skip any
  // which are in fact not owned
  while (scopes.isNotEmpty()) {
    Scope *s = scopes.removeFirst();
    if (s->curCompound || s->namespaceVar) {
      // this isn't one we own
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


FunctionType *Env::beginConstructorFunctionType(SourceLoc loc)
{
  FunctionType *ft = makeFunctionType(loc, getSimpleType(loc, ST_CDTOR));
  ft->flags |= FF_CTOR;
  // asymmetry with makeDestructorFunctionType(): this must be done by the client
//    ft->doneParams();
  return ft;
}


FunctionType *Env::makeDestructorFunctionType(SourceLoc loc)
{
  FunctionType *ft = makeFunctionType(loc, getSimpleType(loc, ST_CDTOR));
  ft->flags |= FF_DTOR;
  ft->doneParams();
  return ft;
}


Scope *Env::enterScope(ScopeKind sk, char const *forWhat)
{
  trace("env") << locStr() << ": entered scope for " << forWhat << "\n";

  // propagate the 'curFunction' field
  Function *f = scopes.first()->curFunction;
  Scope *newScope = new Scope(sk, getChangeCount(), loc());
  scopes.prepend(newScope);
  newScope->curFunction = f;

  // this is actually a no-op since there can't be any edges for
  // a new scope, but I do it for uniformity
  newScope->openedScope(*this);

  return newScope;
}

void Env::exitScope(Scope *s)
{
  s->closedScope();

  trace("env") << locStr() << ": exited " << s->desc() << "\n";
  Scope *f = scopes.removeFirst();
  xassert(s == f);
  delete f;
}


void Env::extendScope(Scope *s)
{
  trace("env") << locStr() << ": extending " << s->desc() << "\n";

  Scope *prevScope = scope();
  scopes.prepend(s);
  s->curLoc = prevScope->curLoc;

  s->openedScope(*this);
}

void Env::retractScope(Scope *s)
{
  trace("env") << locStr() << ": retracting " << s->desc() << "\n";

  s->closedScope();

  Scope *first = scopes.removeFirst();
  xassert(first == s);
  // we don't own 's', so don't delete it
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


// -------- insertion --------
Scope *Env::acceptingScope(DeclFlags df)
{
  if (lang.noInnerClasses && 
      (df & (DF_TYPEDEF | DF_ENUMERATOR))) {
    // C mode: typedefs and enumerators go into the outer scope
    return outerScope();
  }

  Scope *s = scopes.first();    // first in list
  if (s->canAcceptNames) {
    return s;    // common case
  }

  s = scopes.nth(1);            // second in list
  if (s->canAcceptNames) {
    return s;
  }

  // since non-accepting scopes should always be just above
  // an accepting scope
  xfailure("had to go more than two deep to find accepting scope");
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
    trace("env") << "not adding variable `" << v->name
                 << "' because there are disambiguating errors\n";
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
    prevLookup->getOverloadSet()->addMember(v);   // might make an overload set
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
    trace("env") << "not adding compound `" << ct->name
                 << "' because there are disambiguating errors\n";
    return true;
  }

  return typeAcceptingScope()->addCompound(ct);
}


bool Env::addEnum(EnumType *et)
{
  // like above
  if (disambErrorsSuppressChanges()) {
    trace("env") << "not adding enum `" << et->name
                 << "' because there are disambiguating errors\n";
    return true;
  }

  return typeAcceptingScope()->addEnum(et);
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
  // this scope keeps track of which scope we've identified
  // so far, given how many qualifiers we've processed;
  // initially it is NULL meaning we're still at the default,
  // lexically-enclosed scope
  Scope *scope = NULL;

  do {
    PQ_qualifier const *qualifier = name->asPQ_qualifierC();

    // get the first qualifier
    StringRef qual = qualifier->qualifier;
    if (!qual) {
      // this is a reference to the global scope, i.e. the scope
      // at the bottom of the stack
      scope = scopes.last();

      // should be syntactically impossible to construct bare "::"
      // with template arguments
      xassert(!qualifier->targs);
    }

    else {
      // look for a class called 'qual' in scope-so-far; look in the
      // *variables*, not the *compounds*, because it is legal to make
      // a typedef which names a scope, and use that typedef'd name as
      // a qualifier
      //
      // however, this still is not quite right, see cppstd 3.4.3 para 1
      //
      // update: LF_TYPES_NAMESPACES now gets it right, I think
      Variable *qualVar =
        scope==NULL? lookupVariable(qual, LF_TYPES_NAMESPACES) :
                     scope->lookupVariable(qual, *this, LF_TYPES_NAMESPACES);
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
          true /*disambiguating*/);
        return NULL;
      }
                                                                              
      // this is what LF_TYPES_NAMESPACES means
      xassert(qualVar->hasFlag(DF_TYPEDEF) || qualVar->hasFlag(DF_NAMESPACE));
                                         
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

        // the const_cast here is unfortunate, but I don't see an
        // easy way around it
        CompoundType *ct =
          const_cast<CompoundType*>(qualVar->type->ifCompoundType());
        if (!ct) {
          error(stringc
            << "typedef'd name `" << qual << "' doesn't refer to a class, "
            << "so it can't be used as a scope qualifier");
          return NULL;
        }

        if (ct->isTemplate()) {
          anyTemplates = true;
        }

        // check template argument compatibility
        if (qualifier->targs) {
          if (!ct->isTemplate()) {
            error(stringc
              << "class `" << qual << "' isn't a template");
            // recovery: use the scope anyway
          }   

          else {
            // TODO: maybe put this back in
            //ct = checkTemplateArguments(ct, qualifier->targs);

            // for now, restricted form of specialization selection to
            // get in/t0154.cc through
            if (qualifier->targs->count() == 1) {
              SFOREACH_OBJLIST_NC(CompoundType, ct->templateInfo->instantiations, iter) {
                CompoundType *special = iter.data();
                if (!special->templateInfo->arguments.count() == 1) continue;

                if (qualifier->targs->first()->sarg.equals(
                      special->templateInfo->arguments.first())) {
                  // found the specialization or existing instantiation!
                  ct = special;
                  break;
                }
              }
            }

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
        scope = ct;
      }
      
      // case 2: qualifier refers to a namespace
      else /*DF_NAMESPACE*/ {
        if (qualifier->targs) {
          error(stringc << "namespace `" << qual << "' can't accept template args");
        }
        
        // the namespace becomes the active scope
        scope = qualVar->scope;
        xassert(scope);
      }
    }

    // advance to the next name in the sequence
    name = qualifier->rest;
  } while (name->hasQualifiers());

  return scope;
}


#if 0     // aborted implementation for now
// verify correspondence between template arguments and
// template parameters; return selected specialization of 'ct'
CompoundType *Env::
  checkTemplateArguments(CompoundType *ct, FakeList<TempalteArgument> const *args)
{
  ClassTemplateInfo *tinfo = ct->templateInfo;
  xassert(tinfo);

  // check argument list against parameter list
  TemplateArgument const *argIter = args->firstC();
  SFOREACH_OBJLIST(Variable, tinfo->params, paramIter) {
    if (!argIter) {
      error("not enough arguments to template");
      return NULL;
    }

    // for now, since I only have type-valued parameters
    // and type-valued arguments, correspondence is automatic

    argIter = argIter->next;
  }
  if (argIter) {
    error("too many arguments to template");
    return NULL;
  }

  // check for specializations
  CompoundType *special = selectSpecialization(tinfo, args);
  if (!special) {
    return ct;         // no specialization
  }
  else {
    return special;    // specialization found
  }
}

// check the list of specializations in 'tinfo' for one that
// matches the given arguments in 'args'; return the matching
// specialization, or NULL if none matches
CompoundType *Env::
  selectSpecialization(ClassTemplateInfo *tinfo, FakeList<TempalteArgument> const *args)
{
  SFOREACH_OBJLIST_NC(CompoundType, tinfo->specializations, iter) {
    CompoundType *special = iter.data();

    // create an empty unification environment
    TypeUnificationEnv tuEnv;
    bool matchFailure = false;

    // check supplied argument list against specialized argument list
    TemplateArgument const *userArg = args->firstC();
    FAKELIST_FOREACH(TemplateArgument,
                     special->templateInfo->specialArguments,
                     specialArg) {
      // the specialization cannot have more arguments than
      // the client code supplies; TODO: make sure that the
      // code which accepts specializations enforces this
      xassert(argIter);

      // get the types for each
      Type *userType = userArg->asTA_typeC()->getType();
      Type *specialType = specialArg->asTA_typeC()->getType();

      // unify them
      if (!tuEnv.unify(userType, specialType)) {
        matchFailure = true;
        break;
      }

      userArg = userArg->next;
    }

    if (userArg) {
      // we didn't use all of the user's arguments.. I guess this
      // is a partial specialization of some sort, and it matches
    }

    if (!matchFailure) {
      // success
      return special;

      // TODO: try all the specializations, and complain if more
      // than once succeeds
    }
  }

  // no matching specialization
  return NULL;
}
#endif // 0, aborted implementation


Variable *Env::lookupPQVariable(PQName const *name, LookupFlags flags)
{
  Variable *var;

  Scope *scope;      // scope in which name is found, if it is
  if (name->hasQualifiers()) {
    // look up the scope named by the qualifiers
    bool dependent = false, anyTemplates = false;
    scope = lookupQualifiedScope(name, dependent, anyTemplates);
    if (!scope) {
      if (dependent) {
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

    // look inside the final scope for the final name
    var = scope->lookupVariable(name->getName(), *this, flags);
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
        false /*disambiguating*/);
      return NULL;
    }
  }

  else {
    var = lookupVariable(name->getName(), flags, scope);
  }

  if (var) {
    // get the final component of the name, so we can inspect
    // whether it has template arguments
    PQName const *final = name->getUnqualifiedNameC();

    // compare the name's template status to whether template
    // arguments were supplied; note that you're only *required*
    // to pass arguments for template classes, since template
    // functions can infer their arguments
    if (var->isTemplateClass()) {
      if (!final->isPQ_template()) {
        // cppstd 14.6.1 para 1: if the name refers to a template
        // in whose scope we are, then it need not have arguments
        CompoundType *enclosing = findEnclosingTemplateCalled(final->getName());
        if (enclosing) {
          trace("template") << "found bare reference to enclosing template: "
                            << enclosing->name << "\n";
          return enclosing->typedefVar;
        }

        // this disambiguates
        //   new Foo< 3 > +4 > +5;
        // which could absorb the '3' as a template argument or not,
        // depending on whether Foo is a template
        error(stringc
          << "`" << var->name << "' is a template, but template "
          << "arguments were not supplied",
          true /*disambiguating*/);
        return NULL;
      }
      else {
        // apply the template arguments to yield a new type based
        // on the template
        return instantiateClassTemplate(scope, var->type->asCompoundType(),
                                        final->asPQ_templateC()->args);
      }
    }
    else if (!var->isTemplate() &&
             final->isPQ_template()) {
      // disambiguates the same example as above, but selects
      // the opposite interpretation
      error(stringc
        << "`" << var->name << "' is not a template, but template "
        << "arguments were supplied",
        true /*disambiguating*/);
      return NULL;
    }
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
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    CompoundType *ret = scope->lookupCompound(name->getName(), flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no class/struct/union called `"
        << name->getName() << "'",
        true /*disambiguating*/);
      return NULL;
    }

    return ret;
  }

  return lookupCompound(name->getName(), flags);
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
        true /*disambiguating*/);
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


TemplateParams * /*owner*/ Env::takeTemplateParams()
{
  Scope *s = scope();
  TemplateParams *ret = s->curTemplateParams;
  s->curTemplateParams = NULL;
  return ret;
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


ClassTemplateInfo *Env::takeTemplateClassInfo(StringRef baseName)
{
  ClassTemplateInfo *ret = NULL;

  Scope *s = scope();
  if (s->curTemplateParams) {
    ret = new ClassTemplateInfo(baseName);
    ret->params.concat(s->curTemplateParams->params);
    delete takeTemplateParams();
  }

  return ret;
}


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

  // transfer template parameters
  ct->templateInfo = takeTemplateClassInfo(name);

  ct->forward = forward;
  if (name && scope) {
    bool ok = scope->addCompound(ct);
    xassert(ok);     // already checked that it was ok
  }

  // make the implicit typedef
  Type *ret = makeType(loc, ct);
  Variable *tv = makeVariable(loc, name, ret, DF_TYPEDEF | DF_IMPLICIT);
  ct->typedefVar = tv;
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

  return ret;
}


// ----------------- template instantiation -------------
Variable *Env::instantiateClassTemplate
  (Scope *foundScope, CompoundType *base, 
   FakeList<TemplateArgument> *arguments, CompoundType *inst)
{
  // go over the list of arguments, and make a list of
  // semantic arguments
  SObjList<STemplateArgument> sargs;
  {
    FAKELIST_FOREACH_NC(TemplateArgument, arguments, iter) {
      if (iter->sarg.hasValue()) {
        sargs.append(&iter->sarg);
      }
      else {
        // this happens frequently when processing the uninstantiated
        // template, but should then be suppressed in that case
        error("attempt to use unresolved arguments to instantiate a class");
        return dependentTypeVar;
      }
    }
  }

  // has this class already been instantiated?
  if (!inst) {
    SFOREACH_OBJLIST(CompoundType, base->templateInfo->instantiations, iter) {
      if (iter.data()->templateInfo->equalArguments(sargs)) {
        // found it
        Variable *ret = iter.data()->typedefVar;
        xassert(ret);    // I've had a few instances of failing to make this,
                         // and if it looks like lookup failed if it's NULL
        return ret;
      }
    }
  }

  // render the template arguments into a string that we can use
  // as the name of the instantiated class; my plan is *not* that
  // this string serve as the unique ID, but rather that it be
  // a debugging aid only
  //
  // dsw: UPDATE: now it's used during type construction for
  // elaboration, so has to be just the base name
  StringRef instName = base->name;
  trace("template") << (base->forward? "(forward) " : "")
                    << "instantiating class: "
                    << base->name << sargsToString(sargs) << endl;

  // remove scopes from the environment until the innermost
  // scope on the scope stack is the same one that the template
  // definition appeared in; template definitions can see names
  // visible from their defining scope only [cppstd 14.6 para 1]
  ObjList<Scope> innerScopes;
  Scope *argScope = NULL;
  if (!base->forward) {      // don't mess with it if not going to tcheck anything
    while (scopes.first() != foundScope) {
      innerScopes.prepend(scopes.removeFirst());
    }

    // make a new scope for the template arguments
    argScope = enterScope(SK_TEMPLATE, "template argument bindings");

    // simultaneously iterate over the parameters and arguments,
    // creating bindings from params to args
    SObjListIter<Variable> paramIter(base->templateInfo->params);
    FakeList<TemplateArgument> *argIter(arguments);
    while (!paramIter.isDone() && argIter) {
      Variable const *param = paramIter.data();
      TemplateArgument *arg = argIter->first();

      if (param->hasFlag(DF_TYPEDEF) &&
          arg->isTA_type()) {
        // bind the type parameter to the type argument; I considered
        // simply modifying 'param' but decided this would be cleaner
        // for now..
        Variable *binding = makeVariable(param->loc, param->name,
                                         arg->asTA_type()->type->getType(),
                                         DF_TYPEDEF);
        addVariable(binding);
      }
      else if (!param->hasFlag(DF_TYPEDEF) &&
               arg->isTA_nontype()) {
        // TODO: verify that the argument in fact matches the parameter type

        // bind the nontype parameter directly to the nontype expression;
        // this will suffice for checking the template body, because I
        // can const-eval the expression whenever it participates in
        // type determination; the type must be made 'const' so that
        // E_variable::constEval will believe it can evaluate it
        Type *bindType = tfac.applyCVToType(param->loc, CV_CONST,
                                            param->type, NULL /*syntax*/);
        Variable *binding = makeVariable(param->loc, param->name,
                                         bindType, DF_NONE);
        binding->value = arg->asTA_nontype()->expr;
        addVariable(binding);
      }
      else {                 
        // mismatch between argument kind and parameter kind
        char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
        char const *argKind = arg->isTA_type()? "type" : "non-type";
        error(stringc
          << "`" << param->name << "' is a " << paramKind
          << " parameter, but `" << arg->argString() << "' is a "
          << argKind << " argument");
      }

      paramIter.adv();
      argIter = argIter->butFirst();
    }

    if (!paramIter.isDone()) {
      error(stringc
        << "too few template arguments to `" << base->name
        << "' (and partial specialization is not implemented)");
    }
    else if (argIter) {
      error(stringc
        << "too many template arguments to `" << base->name << "'");
    }
  }

  // make a copy of the template definition body AST
  TS_classSpec *copy = base->forward? NULL : base->syntax->clone();
  if (copy && tracingSys("cloneAST")) {
    cout << "--------- clone of " << base->name << " ------------\n";
    copy->debugPrint(cout, 0);
  }

  // make the CompoundType that will represent the instantiated class,
  // if we don't already have one
  if (!inst) {
    // 1/21/03: I had been using 'instName' as the class name, but that
    // causes problems when trying to recognize constructors
    inst = tfac.makeCompoundType(base->keyword, base->name);
    inst->instName = instName;     // stash it here instead
    inst->forward = base->forward;

    // copy over the template arguments so we can recognize this
    // instantiation later
    inst->templateInfo = new ClassTemplateInfo(base->name);
    {
      SFOREACH_OBJLIST(STemplateArgument, sargs, iter) {
        inst->templateInfo->arguments.append(new STemplateArgument(*iter.data()));
      }

      // remember the argument syntax too, in case this is a
      // forward declaration
      inst->templateInfo->argumentSyntax = arguments;
    }

    // tell the base template about this instantiation; this has to be
    // done before invoking the type checker, to handle cases where the
    // template refers to itself recursively (which is very common)
    base->templateInfo->instantiations.append(inst);

    // wrap the compound in a regular type
    SourceLoc copyLoc = copy? copy->loc : SL_UNKNOWN;
    Type *type = makeType(copyLoc, inst);

    // make a fake implicit typedef; this class and its typedef won't
    // actually appear in the environment directly, but making the
    // implicit typedef helps ensure uniformity elsewhere; also must be
    // done before type checking since it's the typedefVar field which
    // is returned once the instantiation is found via 'instantiations'
    Variable *var = makeVariable(copyLoc, instName, type,
                                 DF_TYPEDEF | DF_IMPLICIT);
    inst->typedefVar = var;
    foundScope->registerVariable(var);
  }

  if (copy) {
    // invoke the TS_classSpec typechecker, giving to it the
    // CompoundType we've built to contain its declarations; for
    // now I don't support nested template instantiation
    copy->ctype = inst;
    copy->tcheckIntoCompound(*this, DF_NONE, inst, false /*inTemplate*/,
                             NULL /*containingClass*/);

    if (tracingSys("cloneTypedAST")) {
      cout << "--------- typed clone of " << base->name << " ------------\n";
      copy->debugPrint(cout, 0);
    }

    // remove the argument scope
    exitScope(argScope);

    // re-add the inner scopes removed above
    while (innerScopes.isNotEmpty()) {
      scopes.prepend(innerScopes.removeFirst());
    }
  }

  return inst->typedefVar;
}


// given a name that was found without qualifiers or template arguments,
// see if we're currently inside the scope of a template definition
// with that name
CompoundType *Env::findEnclosingTemplateCalled(StringRef name)
{
  FOREACH_OBJLIST(Scope, scopes, iter) {
    Scope const *s = iter.data();

    if (s->curCompound &&
        s->curCompound->templateInfo &&
        s->curCompound->templateInfo->baseName == name) {
      return s->curCompound;
    }
  }
  return NULL;     // not found
}


void Env::instantiateForwardClasses(Scope *scope, CompoundType *base)
{
  SFOREACH_OBJLIST_NC(CompoundType, base->templateInfo->instantiations, iter) {
    CompoundType *inst = iter.data();
    xassert(inst->templateInfo);

    if (inst->forward) {
      trace("template") << "instantiating previously forward " << inst->name << "\n";
      inst->forward = false;
      instantiateClassTemplate(scope, base, inst->templateInfo->argumentSyntax, 
                               inst /*use this one*/);
    }
    else {
      // this happens in e.g. t0079.cc, when the template becomes
      // an instantiation of itself because the template body
      // refers to itself with template arguments supplied
      
      // update: maybe not true anymore?
    }
  }
}


// -------- diagnostics --------
Type *Env::error(SourceLoc L, char const *msg, bool disambiguates)
{
  trace("error") << (disambiguates? "[d] " : "") << "error: " << msg << endl;
  if (!disambiguateOnly || disambiguates) {
    errors.addError(new ErrorMsg(L, msg, 
      disambiguates ? EF_DISAMBIGUATES : EF_NONE));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}

Type *Env::error(char const *msg, bool disambiguates)
{
  return error(loc(), msg, disambiguates);
}


Type *Env::warning(char const *msg)
{
  trace("error") << "warning: " << msg << endl;
  if (!disambiguateOnly) {
    errors.addError(new ErrorMsg(loc(), msg, EF_WARNING));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}


Type *Env::unimp(char const *msg)
{
  // always print this immediately, because in some cases I will
  // segfault (deref'ing NULL) right after printing this
  cout << "unimplemented: " << msg << endl;

  errors.addError(new ErrorMsg(
    loc(), stringc << "unimplemented: " << msg, EF_NONE));
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
    return error(msg, false /*disambiguates*/);
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
    return tfac.makeTypeOf_this(loc() /*?*/, encScope, CV_NONE, NULL);
  }
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


// true if two function types have equivalent signatures, meaning
// if their names are the same then they refer to the same function,
// not two overloaded instances
bool equivalentSignatures(FunctionType const *ft1, FunctionType const *ft2)
{
  return ft1->innerEquals(ft2, Type::EF_SIGNATURE);
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
bool almostEqualTypes(Type const *t1, Type const *t2)
{
  if (t1->isArrayType() &&
      t2->isArrayType()) {
    ArrayType const *at1 = t1->asArrayTypeC();
    ArrayType const *at2 = t2->asArrayTypeC();

    if ((at1->hasSize() && !at2->hasSize()) ||
        (at2->hasSize() && !at1->hasSize())) {
      // the exception kicks in
      return at1->eltType->equals(at2->eltType);
    }
  }

  // no exception: strict equality (well, toplevel param cv can differ)
  return t1->equals(t2, Type::EF_IGNORE_PARAM_CV);
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
bool sameScopes(Scope *s1, Scope *s2)
{
  if (s1 && s1->scopeKind == SK_GLOBAL) { s1 = NULL; }
  if (s2 && s2->scopeKind == SK_GLOBAL) { s2 = NULL; }

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
      Type *thisType = tfac.makeTypeOf_this(SL_UNKNOWN, ct, oldFt->getThisCV(), NULL /*syntax*/);
      Variable *thisVar = makeVariable(loc, thisName, thisType, DF_PARAMETER);
      newFt->addThisParam(thisVar);

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
    type->isFunctionType()? type->asFunctionType()->getThisCV() : CV_NONE);

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
      overloadSet = prior->getOverloadSet();
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
    Variable *match = prior->overload->findByType(ft, this_cv);
    if (match) {
      prior = match;
    }
  }

  return prior;
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
    // or it's a conversion operator
    if (!equivalentSignatures(priorFt, specFt) ||
        (prior->name == conversionOperatorName &&
         !priorFt->equals(specFt))) {
      // ok, allow the overload
      TRACE("ovl",    "overloaded `" << prior->name
                   << "': `" << prior->type->toString()
                   << "' and `" << type->toString() << "'");
      overloadSet = prior->getOverloadSet();
      prior = NULL;    // so we don't consider this to be the same
    }
  }

  return overloadSet;
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
          !prior->hasFlag(DF_IMPLICIT)) {    // allow implicit typedef to be hidden
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
      if (prior->hasFlag(DF_IMPLICIT)) {
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
        error(type, stringc
          << "prior declaration of `" << name
          << "' at " << prior->loc
          << " had type `" << prior->type->toString()
          << "', but this one uses `" << type->toString() << "'");
        goto makeDummyVar;
      }
    }

    // if the prior declaration refers to a different entity than the
    // one we're trying to declare here, we have a conflict (I'm trying
    // to implement 7.3.3 para 11); I try to determine whether they
    // are the same or different based on the scopes in which they appear
    if (!sameScopes(prior->skipAlias()->scope, scope)) {
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


// ------------------------ elaboration -------------------------
PQ_name *Env::makeTempName()
{
  // can't collide with user identifier
  StringRef name0 = str(stringc << "temp-name-" << tempSerialNumber++);
  return new PQ_name(loc(), name0);
}

StringRef Env::makeE_newVarName()
{
  return str(stringc << "e_new-name-" << e_newSerialNumber++);
}

StringRef Env::makeThrowClauseVarName()
{
  return str(stringc << "throwClause-name-" << throwClauseSerialNumber++);
}


PQName *Env::make_PQ_fullyQualifiedName(Scope *s, PQName *name0)
{
  // should only be called for named scopes
  Variable *typedefVar = s->getTypedefName();
  xassert(typedefVar);

  // prepend to 'name0' information about 's'
  {
    // construct the list of template arguments; we must rebuild them
    // instead of using templateInfo->argumentSyntax directly, because the
    // TS_names that are used in the latter might not be in scope here
    FakeList<TemplateArgument> *targs = FakeList<TemplateArgument>::emptyList();
    if (s->curCompound && s->curCompound->templateInfo) {
      FAKELIST_FOREACH_NC(TemplateArgument, 
                          s->curCompound->templateInfo->argumentSyntax, iter) {
        ASTSWITCH(TemplateArgument, iter) {
          ASTCASE(TA_type, t) {
            // pull out the Type, then build a new ASTTypeId for it
            targs = targs->prepend(new TA_type(buildASTTypeId(t->type->getType())));
          }
          ASTNEXT(TA_nontype, n) {
            // just use it directly.. someone could be evil and include
            // sizeof(TS_name(...)) in here to fool us, but oh well
            targs = targs->prepend(n);
          }
          ASTENDCASED
        }
      }

      // built them in reverse order for O(1) insertion, now fix that
      targs->reverse();
    }

    // now build a PQName
    if (name0) {
      // cons a qualifier on to the front
      name0 = new PQ_qualifier(loc(), typedefVar->name, targs, name0);
    }
    else {
      // dsw: dang it Scott, why the templatization asymmetry between
      // PQ_name/template on one hand and PQ_qualifier on the other?
      //
      // sm: to save space in the common PQ_name case
      if (targs) {
        name0 = new PQ_template(loc(), typedefVar->name, targs);
      }
      else {
        name0 = new PQ_name(loc(), typedefVar->name);
      }
    }
  }

  // now find additional qualifiers needed to name 's' itself
  if (s->parentScope) {
    return make_PQ_fullyQualifiedName(s->parentScope, name0);
  }
  else if (s->scopeKind == SK_GLOBAL) {
    // prepend what syntactically would be a leading "::"
    return new PQ_qualifier(loc(), NULL /*qualifier*/,
                            FakeList<TemplateArgument>::emptyList(),
                            name0);
  }
  else {
    // no further qualification is possible, so we cross our fingers
    // and hope it isn't necessary either
    return name0;
  }
}


PQName *Env::make_PQ_fullyQualifiedDtorName(CompoundType *ct)
{
  return make_PQ_fullyQualifiedName(ct,
    new PQ_name(loc(), str(stringc << "~" << ct->name)));
}


ASTTypeId *Env::buildASTTypeId(Type *type)
{
  // kick off the recursive decomposition
  IDeclarator *surrounding = new D_name(loc(), NULL /*name*/);
  return inner_buildASTTypeId(type, surrounding);
}


// 'surrounding' is the syntax that denotes the type constructors already
// stripped off of 'type' since the outer call to 'buildASTTypeId'.  The
// manipulations here are a little strange because we're turning the
// Type world inside-out to get the TypeSpecifier/Declarator syntax.
//
// Here's an example scenario:
//
//         ASTTypeId         <---- this is what we're ultimately trying to make
//          /      \                                                         .
//         /     D_pointer         <-- return value for this invocation
//        /        /   \                                                     .
//       /        /  D_pointer     <-- current value of 'surrounding'
//      |        /     /   \                                                 .
//  TS_simple   /     |   D_name   <-- first value of 'surrounding'
//     |       |      |     |
//    int      *      *   (null)
//     .       .      .
//     .       .      .
//     .       .    PointerType    <-- original argument to buildASTTypeId
//     .       .     /
//     .     PointerType           <-- current 'type'
//     .      /
//    CVAtomic                     <-- type->asPointerType()->atType
//      |
//    Simple(int)
//
// The dotted lines show the correspondence between structures in the
// Type world and those in the TypeSpecifier/Declarator world.  The
// pictured scenario is midway through the recursive decomposition of
// 'int**'.
//
ASTTypeId *Env::inner_buildASTTypeId(Type *type, IDeclarator *surrounding)
{
  // The strategy here is to use typedefs when we can, and build
  // declarators when we have to.  Consider:
  //
  //   // from t0027.cc
  //   { typedef int* x; }     // 'int*' has a defunct typedef
  //   Foo<int*> f;            // cannot typedef 'x'!  must use 'int*'
  //
  // but
  //
  //   // related to t0156.cc
  //   template <class T>
  //   class A {
  //     T t;                  // must use typedef 'T'!  'C' isn't visible
  //   };
  //   void foo() {
  //     class C {};           // local class; cannot name directly
  //     A<C*> a;              // template argument not a simple TS_name
  //   }
  //
  // So:
  //   - for simple types, we use TS_simple
  //   - for compound types, typedefs are only option
  //   - for constructed types, try the typedefs, then resort to D_pointer, etc.

  // first part tries to build a type specifier
  TypeSpecifier *spec = NULL;

  if (type->isCVAtomicType()) {        // expected common case
    CVAtomicType *at = type->asCVAtomicType();

    if (at->isSimpleType()) {
      // easy case
      spec = new TS_simple(loc(), at->atomic->asSimpleType()->type);
    }
    else {
      // typedef is only option, since we can't sit down and spell
      // out the definition again
      spec = buildTypedefSpecifier(type);
      if (!spec) {
        xfailure(stringc << locStr()
          << ": failed to find a usable typedef alias for `"
          << type->toString() << "'");
      }
    }
  }
  else {
    // try the typedef option, but it's not necessarily a problem if
    // we don't find one now
    spec = buildTypedefSpecifier(type);
  }

  // did the above efforts find a spec we can use?
  if (spec) {
    return new ASTTypeId(
      spec,
      new Declarator(surrounding, NULL /*init*/)
    );
  }

  // we must deconstruct the type and build a declarator that will
  // denote it; somewhere down here we've got to find a name that has
  // meaning in this scope, or else reach a simple type
  switch (type->getTag()) {
    default: xfailure("bag tag");

    case Type::T_POINTER: {
      PointerType *pt = type->asPointerType();
      return inner_buildASTTypeId(pt->atType,
        new D_pointer(loc(), pt->op==PO_POINTER, pt->cv, surrounding));
    }

    case Type::T_FUNCTION: {
      FunctionType *ft = type->asFunctionType();

      // I don't think you can use a member function in any of the
      // contexts that provoke type syntax synthesis
      xassert(!ft->isMethod());

      // parameter list (isn't this fun?)
      FakeList<ASTTypeId> *paramSyntax = FakeList<ASTTypeId>::emptyList();
      SFOREACH_OBJLIST_NC(Variable, ft->params, iter) {
        paramSyntax = paramSyntax->prepend(buildASTTypeId(iter.data()->type));
      }
      paramSyntax->reverse();    // pray at altar of O(n)

      if (ft->exnSpec) {
        // straightforward to implement, but I'm lazy right now
        xfailure("unimplemented: synthesized type name with an exception spec");
      }

      return inner_buildASTTypeId(ft->retType,
        new D_func(loc(), surrounding, paramSyntax, CV_NONE, NULL /*exnSpec*/));
    }

    case Type::T_ARRAY: {
      ArrayType *at = type->asArrayType();
      return inner_buildASTTypeId(at->eltType,
        new D_array(loc(), surrounding,
          at->hasSize()? buildIntegerLiteralExp(at->size) : NULL));
    }

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType *ptm = type->asPointerToMemberType();
      return inner_buildASTTypeId(ptm->atType,
        new D_ptrToMember(loc(), make_PQ_fullyQualifiedName(ptm->inClass),
                          ptm->cv, surrounding));
    }
  }
}


TS_name *Env::buildTypedefSpecifier(Type *type)
{
  if (type->typedefAliases.isEmpty()) {
    return NULL;    // no aliases to try
  }

  // since I'm going to be doing speculative lookups that shouldn't
  // become user-visible error messages, arrange to throw away any
  // that get generated
  SuppressErrors suppress(*this);

  // to make this work in some pathological cases (d0026.cc,
  // t0156.cc), we need to iterate over the list of typedef aliases
  // until we find the one that will tcheck to 'type'
  SFOREACH_OBJLIST_NC(Variable, type->typedefAliases, iter) {
    Variable *alias = iter.data();

    TRACE("buildTypedefSpecifier",
      "`" << type->toString() << "': trying " << alias->name);

    // first, the final component of the name
    PQName *name = new PQ_name(loc(), alias->name);

    // then the qualifier prefix, if any
    if (alias->scope) {
      name = make_PQ_fullyQualifiedName(alias->scope->curCompound, name);
    }

    // did that work?
    Variable *found = lookupPQVariable(name);
    if (found && found->type == type) {
      // good to go; wrap it in a type specifier
      return new TS_name(loc(), name, false /*typenameUsed*/);
    }

    // didn't work, try the next alias

    // it's tempting to deallocate 'name', but since
    // make_PQ_fullyQualifiedName might return something that shares
    // subtrees with the original AST, we can't
  }

  return NULL;      // none of the aliases work
}


Expression *Env::buildIntegerLiteralExp(int i)
{
  StringRef text = str(stringc << i);
  return new E_intLit(text);
}


// EOF
