// cc_env.cc            see license.txt for copyright and terms of use
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys
#include "ckheap.h"      // heapCheck
#include "strtable.h"    // StringTable
#include "cc_lang.h"     // CCLang
#include "strutil.h"     // suffixEquals
#include "overload.h"    // selectBestCandidate_templCompoundType
#include "matchtype.h"   // MatchType

TD_class Env::mode1Dummy(NULL, NULL); // arguments are meaningless


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

/* static */
TemplCandidates::STemplateArgsCmp TemplCandidates::compareSTemplateArgs
  (TypeFactory &tfac, STemplateArgument const *larg, STemplateArgument const *rarg)
{
  xassert(larg->kind == rarg->kind);

  switch(larg->kind) {
  default:
    xfailure("illegal TemplateArgument kind");
    break;

  case STemplateArgument::STA_NONE: // not yet resolved into a valid template argument
    xfailure("STA_NONE TemplateArgument kind");
    break;

  case STemplateArgument::STA_TYPE: // type argument
    {
    // check if left is at least as specific as right
    bool leftAtLeastAsSpec;
    {
      MatchTypes match(tfac, MatchTypes::MM_WILD);
      if (match.match_Type(larg->value.t, rarg->value.t)) {
        leftAtLeastAsSpec = true;
      } else {
        leftAtLeastAsSpec = false;
      }
    }
    // check if right is at least as specific as left
    bool rightAtLeastAsSpec;
    {
      MatchTypes match(tfac, MatchTypes::MM_WILD);
      if (match.match_Type(rarg->value.t, larg->value.t)) {
        rightAtLeastAsSpec = true;
      } else {
        rightAtLeastAsSpec = false;
      }
    }

    // change of basis matrix
    if (leftAtLeastAsSpec) {
      if (rightAtLeastAsSpec) {
        return STAC_EQUAL;
      } else {
        return STAC_LEFT_MORE_SPEC;
      }
    } else {
      if (rightAtLeastAsSpec) {
        return STAC_RIGHT_MORE_SPEC;
      } else {
        return STAC_INCOMPARABLE;
      }
    }

    }
    break;

  case STemplateArgument::STA_INT: // int or enum argument
    if (larg->value.i == rarg->value.i) {
      return STAC_EQUAL;
    }
    return STAC_INCOMPARABLE;
    break;

  case STemplateArgument::STA_REFERENCE: // reference to global object
  case STemplateArgument::STA_POINTER: // pointer to global object
  case STemplateArgument::STA_MEMBER: // pointer to class member
    if (larg->value.v == rarg->value.v) {
      return STAC_EQUAL;
    }
    return STAC_INCOMPARABLE;
    break;

  case STemplateArgument::STA_TEMPLATE: // template argument (not implemented)
    xfailure("STA_TEMPLATE TemplateArgument kind; not implemented");
    break;
  }
}


/* static */
int TemplCandidates::compareCandidatesStatic
  (TypeFactory &tfac, TemplateInfo const *lti, TemplateInfo const *rti)
{
  // I do not even put the primary into the set so it should never
  // show up.
  xassert(lti->isNotPrimary());
  xassert(rti->isNotPrimary());

  // they should have the same primary
  xassert(lti->getMyPrimaryIdem() == rti->getMyPrimaryIdem());

  // they should always have the same number of arguments; the number
  // of parameters is irrelevant
  xassert(lti->arguments.count() == rti->arguments.count());

  STemplateArgsCmp leaning = STAC_EQUAL;// which direction are we leaning?
  // for each argument pairwise
  ObjListIter<STemplateArgument> lIter(lti->arguments);
  ObjListIter<STemplateArgument> rIter(rti->arguments);
  for(;
      !lIter.isDone();
      lIter.adv(), rIter.adv()) {
    STemplateArgument const *larg = lIter.data();
    STemplateArgument const *rarg = rIter.data();
    STemplateArgsCmp cmp = compareSTemplateArgs(tfac, larg, rarg);
    switch(cmp) {
    default: xfailure("illegal STemplateArgsCmp"); break;
    case STAC_LEFT_MORE_SPEC:
      if (leaning == STAC_EQUAL) {
        leaning = STAC_LEFT_MORE_SPEC;
      } else if (leaning == STAC_RIGHT_MORE_SPEC) {
        leaning = STAC_INCOMPARABLE;
      }
      // left stays left and incomparable stays incomparable
      break;
    case STAC_RIGHT_MORE_SPEC:
      if (leaning == STAC_EQUAL) {
        leaning = STAC_RIGHT_MORE_SPEC;
      } else if (leaning == STAC_LEFT_MORE_SPEC) {
        leaning = STAC_INCOMPARABLE;
      }
      // right stays right and incomparable stays incomparable
      break;
    case STAC_EQUAL:
      // all stay same
      break;
    case STAC_INCOMPARABLE:
      leaning = STAC_INCOMPARABLE; // incomparable is an absorbing state
    }
  }
  xassert(rIter.isDone());      // we checked they had the same number of arguments earlier

  switch(leaning) {
  default: xfailure("illegal STemplateArgsCmp"); break;
  case STAC_LEFT_MORE_SPEC: return -1; break;
  case STAC_RIGHT_MORE_SPEC: return 1; break;
  case STAC_EQUAL:
    // FIX: perhaps this should be a user error?
    xfailure("Two template argument tuples are identical");
    break;
  case STAC_INCOMPARABLE: return 0; break;
  }
}


int TemplCandidates::compareCandidates(Variable const *left, Variable const *right)
{
  TemplateInfo *lti = const_cast<Variable*>(left)->templateInfo();
  xassert(lti);
  TemplateInfo *rti = const_cast<Variable*>(right)->templateInfo();
  xassert(rti);

  return compareCandidatesStatic(tfac, lti, rti);
}


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


// --------------------- InstContext -----------------

InstContext::InstContext
  (Variable *baseV0,
   Variable *instV0,
   Scope *foundScope0,
   SObjList<STemplateArgument> &sargs0)
    : baseV(baseV0)
    , instV(instV0)
    , foundScope(foundScope0)
    , sargs(cloneSArgs(sargs0))
{}


// --------------------- PartialScopeStack -----------------

void PartialScopeStack::stackIntoEnv(Env &env)
{
  for (int i=scopes.count()-1; i>=0; --i) {
    Scope *s = this->scopes.nth(i);
    env.scopes.prepend(s);
  }
}


void PartialScopeStack::unStackOutOfEnv(Env &env)
{
  for (int i=0; i<scopes.count(); ++i) {
    Scope *s = this->scopes.nth(i);
    Scope *removed = env.scopes.removeFirst();
    xassert(s == removed);
  }
}


// --------------------- FuncTCheckContext -----------------

FuncTCheckContext::FuncTCheckContext
  (Function *func0,
   Scope *foundScope0,
   PartialScopeStack *pss0)
    : func(func0)
    , foundScope(foundScope0)
    , pss(pss0)
{}


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
    receiverName(str("__receiver")),
    otherName(str("__other")),
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

    doOverload(tracingSys("doOverload") && lang.allowOverloading),
    doOperatorOverload(tracingSys("doOperatorOverload") && lang.allowOverloading),
    collectLookupResults(NULL)
{
  // dsw: for esoteric reasions I need the tfac to be publicly
  // available at times
  global_tfac = &tfac;

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
    tfac.makeReferenceType(HERE, makeCVAtomicType(HERE, ct, CV_CONST));

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

  // 5/04/04: sm: I moved this out of the GNU_EXTENSION section because
  // the mozilla tests use it, and I won't want to make them only
  // active in gnu mode ..
  //
  // for GNU compatibility
  // void *__builtin_next_arg(void *p);
  declareFunction1arg(t_voidptr, "__builtin_next_arg",
                      t_voidptr, "p");

  #ifdef GNU_EXTENSION
    Type *t_int = getSimpleType(HERE, ST_INT);
    Type *t_unsigned_int = getSimpleType(HERE, ST_UNSIGNED_INT);
    Type *t_char = getSimpleType(HERE, ST_CHAR);
    Type *t_charconst = getSimpleType(HERE, ST_CHAR, CV_CONST);
    Type *t_charptr = makePtrType(HERE, t_char);
    Type *t_charconstptr = makePtrType(HERE, t_charconst);

    // dsw: This is a form, not a function, since it takes an expression
    // AST node as an argument; however, I need a function that takes no
    // args as a placeholder for it sometimes.
    var__builtin_constant_p = declareSpecialFunction("__builtin_constant_p");

    // typedef void *__builtin_va_list;
    addVariable(makeVariable(SL_INIT, str("__builtin_va_list"),
                             t_voidptr, DF_TYPEDEF | DF_BUILTIN | DF_GLOBAL));

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

    // dsw: I made up the signature to this one; FIX: should probably
    // also be marked NORETURN.
    // void __assert_fail(char const *__assertion, char const *__file,
    // unsigned int __line, char const *__function);
    declareFunction4arg(t_void, "__assert_fail",
                        t_charconstptr, "__assertion",
                        t_charconstptr, "__file",
                        t_unsigned_int, "__line",
                        t_charconstptr, "__function",
                        FF_NONE, NULL);
  #endif // GNU_EXTENSION

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
     NULL /*prior*/, NULL /*overloadSet*/,
     true /*reallyAddVariable*/);
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


void Env::debugPrintScopes()
{
  cout << "Scopes and variables" << endl;
  for (int i=0; i<scopes.count(); ++i) {
    Scope *s = scopes.nth(i);
    cout << "scope " << i << ", scopeKind " << toString(s->scopeKind) << endl;
    for (StringSObjDict<Variable>::IterC iter = s->getVariableIter();
         !iter.isDone();
         iter.next()) {
      Variable *value = iter.value();
      cout << "\tkey '" << iter.key()
           << "' value value->name '" << value->name
           << "' ";
      printf("%p ", value);
//        cout << "value->serialNumber " << value->serialNumber;
      cout << endl;
    }
  }
}


PartialScopeStack *Env::shallowClonePartialScopeStack(Scope *foundScope)
{
  xassert(foundScope);
  PartialScopeStack *pss = new PartialScopeStack();
  int scopesCount = scopes.count();
  xassert(scopesCount>=2);
  for (int i=0; i<scopesCount; ++i) {
    Scope *s0 = scopes.nth(i);
    Scope *s1 = scopes.nth(i+1);
    // NOTE: omit foundScope itself AND the template scope below it
    if (s1 == foundScope) {
      xassert(s0->scopeKind == SK_TEMPLATE);
      return pss;
    }
    xassert(s0->scopeKind != SK_TEMPLATE);
    pss->scopes.append(s0);
  }
  xfailure("failed to find foundScope in the scope stack");
  return NULL;
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


Env::TemplTcheckMode Env::getTemplTcheckMode() const {
  if (templateDeclarationStack.isEmpty()
      || templateDeclarationStack.topC() == &mode1Dummy) {
    return TTM_1NORMAL;
  }
  // we are in a template declaration
  if (funcDeclStack.isEmpty()) return TTM_3TEMPL_DEF;
  // we are in a function declaration somewhere
  if (funcDeclStack.topC()->isInitializer()) return TTM_3TEMPL_DEF;
  // we are in a function declaration and not in a default argument
  xassert(funcDeclStack.topC()->isD_func());
  return TTM_2TEMPL_FUNC_DECL;
}


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


bool Env::currentScopeAboveTemplEncloses(Scope const *s)
{
  if (scope()->scopeKind == SK_TEMPLATE) {
    // when we are doing instantiation of the definition of a
    // function member of a templatized class, we end up in this
    // situation
    return scopes.nth(1)->encloses(s);
  } else {
    return currentScopeEncloses(s);
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
  ScopeSeq scopes;
  if (!getQualifierScopes(scopes, name, dependent, anyTemplates)) {
    return NULL;
  }
  else {
    return scopes.pop();     // last one added
  }
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
  bool &anyTemplates)            // set to true if we look in uninstantiated templates
{
  // get the first qualifier
  StringRef qual = qualifier->qualifier;
  if (!qual) {      // i.e. "::" qualifier
    // should be syntactically impossible to construct bare "::"
    // with template arguments
    xassert(qualifier->targs.isEmpty());

    // this is a reference to the global scope, i.e. the scope
    // at the bottom of the stack
    return scopes.last();
  }

  // look for a class called 'qual' in 'startingScope'; look in the
  // *variables*, not the *compounds*, because it is legal to make
  // a typedef which names a scope, and use that typedef'd name as
  // a qualifier
  //
  // however, this still is not quite right, see cppstd 3.4.3 para 1
  //
  // update: LF_TYPES_NAMESPACES now gets it right, I think
  Variable *qualVar = startingScope==NULL?
    lookupVariable(qual, LF_TYPES_NAMESPACES) :
    startingScope->lookupVariable(qual, *this, LF_TYPES_NAMESPACES);
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

        Variable *inst = instantiateTemplate_astArgs
          (loc(),               // FIX: ???
           (ct->typedefVar->scope ? ct->typedefVar->scope : globalScope()), // FIX: ???
           ct->typedefVar,
           NULL /*instV*/,
           qualifier->targs);
        xassert(inst);
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

  // begin searching from the current environment
  Scope *scope = NULL;

  while (name->hasQualifiers()) {
    PQ_qualifier const *qualifier = name->asPQ_qualifierC();

    // use the new inner-loop function
    scope = lookupOneQualifier(scope, qualifier,
                               dependent, anyTemplates);
    if (!scope) {
      // stop searching and return what we have
      return false;
    }

    // save this scope
    scopes.push(scope);

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


// FIX: this lookup doesn't do very well for overloaded function
// templates where one primary is more specific than the other; the
// more specific one matches both itself and the less specific one,
// and this gets called ambiguous
Variable *Env::lookupPQVariable_primary_resolve(
  PQName const *name, LookupFlags flags, FunctionType *signature,
  MatchTypes::MatchMode matchMode)
{
  // only makes sense in one context; will push this spec around later
  xassert(flags & LF_TEMPL_PRIMARY);

  // first, call the version that does *not* do overload selection
  Variable *var = lookupPQVariable(name, flags);

  if (!var) return NULL;
  OverloadSet *oloadSet = var->getOverloadSet();
  if (oloadSet->count() > 1) {
    xassert(var->type->isFunctionType()); // only makes sense for function types
    // FIX: Somehow I think this isn't right
    var = findTemplPrimaryForSignature(oloadSet, signature, matchMode);
    if (!var) {
      // FIX: sometimes I just want to know if there is one; not sure
      // this is always an error.
//        error("function template specialization does not match "
//              "any primary in the overload set");
      return NULL;
    }
  }
  return var;
}


Variable *Env::findTemplPrimaryForSignature
  (OverloadSet *oloadSet,
   FunctionType *signature,
   MatchTypes::MatchMode matchMode)
{
  if (!signature) {
    xfailure("This is one more place where you need to add a signature "
             "to the call to Env::lookupPQVariable() to deal with function "
             "template overload resolution");
    return NULL;
  }

  Variable *candidatePrim = NULL;
  SFOREACH_OBJLIST_NC(Variable, oloadSet->set, iter) {
    Variable *var0 = iter.data();
    // skip non-template members of the overload set
    if (!var0->isTemplate()) continue;

    TemplateInfo *tinfo = var0->templateInfo();
    xassert(tinfo);
    xassert(tinfo->isPrimary()); // can only have primaries at the top level

    // check that the function type could be a special case of the
    // template primary
    //
    // FIX: I don't know if this is really as precise a lookup as is
    // possible.
    MatchTypes match(tfac, matchMode);
    if (match.match_Type
        (signature,
         var0->type
         // FIX: I don't know if this should be top or not or if it
         // matters.
         )) {
      if (candidatePrim) {
        xfailure("ambiguous attempt to lookup "
                 "overloaded function template primary from specialization");
        return NULL;            // ambiguous
      } else {
        candidatePrim = var0;
      }
    }
  }
  return candidatePrim;
}


void Env::initArgumentsFromASTTemplArgs
  (TemplateInfo *tinfo,
   ASTList<TemplateArgument> const &templateArgs)
{
  xassert(tinfo);
  xassert(tinfo->arguments.count() == 0); // don't use this if there are already arguments
  FOREACH_ASTLIST(TemplateArgument, templateArgs, iter) {
    TemplateArgument const *targ = iter.data();
    xassert(targ->sarg.hasValue());
    tinfo->arguments.append(new STemplateArgument(targ->sarg));
  }
}


bool Env::checkIsoToASTTemplArgs
  (ObjList<STemplateArgument> &templateArgs0,
   ASTList<TemplateArgument> const &templateArgs1)
{
  MatchTypes match(tfac, MatchTypes::MM_ISO);
  ObjListIterNC<STemplateArgument> iter0(templateArgs0);
  FOREACH_ASTLIST(TemplateArgument, templateArgs1, iter1) {
    if (iter0.isDone()) return false;
    TemplateArgument const *targ = iter1.data();
    xassert(targ->sarg.hasValue());
    STemplateArgument sta(targ->sarg);
    if (!match.match_STA(iter0.data(), &sta, 2 /*matchDepth*/)) return false;
    iter0.adv();
  }
  return iter0.isDone();
}


Variable *Env::getInstThatMatchesArgs
  (TemplateInfo *tinfo, ObjList<STemplateArgument> &arguments, Type *type0)
{
  Variable *prevInst = NULL;
  SFOREACH_OBJLIST_NC(Variable, tinfo->getInstantiations(), iter) {
    Variable *instantiation = iter.data();
    if (type0 && !instantiation->getType()->equals(type0)) continue;
    MatchTypes match(tfac, MatchTypes::MM_ISO);
    bool unifies = match.match_Lists2
      (instantiation->templateInfo()->arguments, arguments, 2 /*matchDepth*/);
    if (unifies) {
      if (instantiation->templateInfo()->isMutant() &&
          prevInst->templateInfo()->isMutant()) {
        // if one mutant matches, more may, so just use the first one
        continue;
      }
      // any other combination of double matching is an error; that
      // is, I don't think you can get a non-mutant instance in more
      // than once
      xassert(!prevInst);
      prevInst = instantiation;
    }
  }
  return prevInst;
}


bool Env::loadBindingsWithExplTemplArgs(Variable *var, ASTList<TemplateArgument> const &args,
                                        MatchTypes &match)
{
  xassert(var->templateInfo());
  xassert(var->templateInfo()->isPrimary());

  SObjListIterNC<Variable> paramIter(var->templateInfo()->params);
  ASTListIter<TemplateArgument> argIter(args);
  while (!paramIter.isDone() && !argIter.isDone()) {
    Variable *param = paramIter.data();

    // no bindings yet and param names unique
    //
    // FIX: maybe I shouldn't assume that param names are
    // unique; then this would be a user error
    if (param->type->isTypeVariable()) {
      xassert(!match.bindings.getTypeVar(param->type->asTypeVariable()));
    } else {
      // for, say, int template parameters
      xassert(!match.bindings.getObjVar(param));
    }

    TemplateArgument const *arg = argIter.data();

    // FIX: when it is possible to make a TA_template, add
    // check for it here.
    //              xassert("Template template parameters are not implemented");

    if (param->hasFlag(DF_TYPEDEF) && arg->isTA_type()) {
      STemplateArgument *bound = new STemplateArgument;
      bound->setType(arg->asTA_typeC()->type->getType());
      match.bindings.putTypeVar(param->type->asTypeVariable(), bound);
    }
    else if (!param->hasFlag(DF_TYPEDEF) && arg->isTA_nontype()) {
      STemplateArgument *bound = new STemplateArgument;
      Expression *expr = arg->asTA_nontypeC()->expr;
      if (expr->getType()->isIntegerType()) {
        int staticTimeValue;
        bool constEvalable = expr->constEval(*this, staticTimeValue);
        if (!constEvalable) {
          // error already added to the environment
          return false;
        }
        bound->setInt(staticTimeValue);
      } else if (expr->getType()->isReference()) {
        // Subject: Re: why does STemplateArgument hold Variables?
        // From: Scott McPeak <smcpeak@eecs.berkeley.edu>
        // To: "Daniel S. Wilkerson" <dsw@eecs.berkeley.edu>
        // The Variable is there because STA_REFERENCE (etc.)
        // refer to (linker-visible) symbols.  You shouldn't
        // be making an STemplateArgument out of an expression
        // directly; if the template parameter is
        // STA_REFERENCE (etc.) then dig down to find the
        // Variable the programmer wants to use.
        //
        // haven't tried to exhaustively enumerate what kinds
        // of expressions this could be; handle other types as
        // they come up
        xassert(expr->isE_variable());
        bound->setReference(expr->asE_variable()->var);
      } else {
        unimp("unhandled kind of template argument");
        return false;
      }
      match.bindings.putObjVar(param, bound);
    }
    else {
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      char const *argKind = arg->isTA_type()? "type" : "non-type";
      // FIX: make a provision for template template parameters here

      // NOTE: condition this upon a boolean flag reportErrors if it
      // starts to fail while filtering functions for overload
      // resolution; see Env::inferTemplArgsFromFuncArgs() for an
      // example
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            << " parameter, but `" << arg->argString() << "' is a "
            << argKind << " argument",
            EF_STRONG);
      return false;
    }
    paramIter.adv();
    argIter.adv();
  }
  return true;
}


bool Env::inferTemplArgsFromFuncArgs
  (Variable *var,
   TypeListIter &argListIter,
   MatchTypes &match,
   bool reportErrors)
{
  xassert(var->templateInfo());
  xassert(var->templateInfo()->isPrimary());

  if (tracingSys("template")) {
    cout << "Deducing template arguments from function arguments" << endl;
  }
  // FIX: make this work for vararg functions
  int i = 1;            // for error messages
  SFOREACH_OBJLIST_NC(Variable, var->type->asFunctionType()->params, paramIter) {
    Variable *param = paramIter.data();
    xassert(param);
    // we could run out of args and it would be ok as long as we
    // have default arguments for the rest of the parameters
    if (!argListIter.isDone()) {
      Type *curArgType = argListIter.data();
      bool argUnifies = match.match_Type(curArgType, param->type);
      if (!argUnifies) {
        if (reportErrors) {
          error(stringc << "during function template instantiation: "
                << " argument " << i << " `" << curArgType->toString() << "'"
                << " is incompatable with parameter, `"
                << param->type->toString() << "'");
        }
        return false;             // FIX: return a variable of type error?
      }
    } else {
      // we don't use the default arugments for template arugment
      // inference, but there should be one anyway.
      if (!param->value) {
        if (reportErrors) {
          error(stringc << "during function template instantiation: too few arguments to " <<
                var->name);
        }
        return false;
      }
    }
    ++i;
    // advance the argIterCur if there is one
    if (!argListIter.isDone()) argListIter.adv();
  }
  if (!argListIter.isDone()) {
    if (reportErrors) {
      error(stringc << "during function template instantiation: too many arguments to " <<
            var->name);
    }
    return false;
  }
  return true;
}


bool Env::getFuncTemplArgs
  (MatchTypes &match,
   SObjList<STemplateArgument> &sargs,
   PQName const *final,
   Variable *var,
   TypeListIter &argListIter,
   bool reportErrors)
{
  xassert(var->templateInfo()->isPrimary());

  // 'final' might be NULL in the case of doing overload resolution
  // for templatized ctors (that is, the ctor is templatized, but not
  // necessarily the class)
  if (final && final->isPQ_template()) {
    if (!loadBindingsWithExplTemplArgs(var, final->asPQ_templateC()->args, match)) {
      return false;
    }
  }

  if (!inferTemplArgsFromFuncArgs(var, argListIter, match, reportErrors)) {
    return false;
  }

  // put the bindings in a list in the right order; FIX: I am rather
  // suspicious that the set of STemplateArgument-s should be
  // everywhere represented as a map instead of as a list
  //
  // try to report more than just one at a time if we can
  bool haveAllArgs = true;
  SFOREACH_OBJLIST_NC(Variable, var->templateInfo()->params, templPIter) {
    Variable *param = templPIter.data();
    STemplateArgument *sta = NULL;
    if (param->type->isTypeVariable()) {
      sta = match.bindings.getTypeVar(param->type->asTypeVariable());
    } else {
      sta = match.bindings.getObjVar(param);
    }
    if (!sta) {
      if (reportErrors) {
        error(stringc << "No argument for parameter `" << templPIter.data()->name << "'");
      }
      haveAllArgs = false;
    }
    sargs.append(sta);
  }
  return haveAllArgs;
}


Variable *Env::lookupPQVariable_function_with_args(
  PQName const *name, LookupFlags flags,
  FakeList<ArgExpression> *funcArgs)
{
  // first, lookup the name but return just the template primary
  Scope *scope;      // scope where found
  Variable *var = lookupPQVariable(name, flags | LF_TEMPL_PRIMARY, scope);
  if (!var || !var->isTemplate()) {
    // nothing unusual to do
    return var;
  }
                                     
  if (!var->isTemplateFunction()) {
    // most of the time this can't happen, b/c the func/ctor ambiguity
    // resolution has already happened and inner2 is only called when
    // the 'var' looks ok; but if there are unusual ambiguities, e.g.
    // in/t0171.cc, then we get here even after 'var' has yielded an
    // error.. so just bail, knowing that an error has already been
    // reported (hence the ambiguity will be resolved as something else)
    return NULL;
  }

  // if we are inside a function definition, just return the primary
  TemplTcheckMode ttm = getTemplTcheckMode();
  // FIX: this can happen when in the parameter list of a function
  // template definition a class template is instantiated (as a
  // Mutant)
  // UPDATE: other changes should prevent this
  xassert(ttm != TTM_2TEMPL_FUNC_DECL);
  if (ttm == TTM_2TEMPL_FUNC_DECL || ttm == TTM_3TEMPL_DEF) {
    xassert(var->templateInfo()->isPrimary());
    return var;
  }

  PQName const *final = name->getUnqualifiedNameC();

  // duck overloading
  OverloadSet *oloadSet = var->getOverloadSet();
  if (oloadSet->count() > 1) {
    xassert(var->type->isFunctionType()); // only makes sense for function types
    // FIX: the correctness of this depends on someone doing
    // overload resolution later, which I don't think is being
    // done.
    return var;
    // FIX: this isn't right; do overload resolution later;
    // otherwise you get a null signature being passed down
    // here during E_variable::itcheck_x()
    //              var = oloadSet->findTemplPrimaryForSignature(signature);
    // // FIX: make this a user error, or delete it
    // xassert(var);
  }
  xassert(var->templateInfo()->isPrimary());

  // get the semantic template arguments
  SObjList<STemplateArgument> sargs;
  {
    TypeListIter_FakeList argListIter(funcArgs);
    MatchTypes match(tfac, MatchTypes::MM_BIND);
    if (!getFuncTemplArgs(match, sargs, final, var, argListIter, true /*reportErrors*/)) {
      return NULL;
    }
  }

  // apply the template arguments to yield a new type based on
  // the template; note that even if we had some explicit
  // template arguments, we have put them into the bindings now,
  // so we omit them here
  return instantiateTemplate(loc(), scope, var, NULL /*inst*/, NULL /*bestV*/, sargs);
}


Variable *Env::lookupPQVariable(PQName const *name, LookupFlags flags)
{
  Scope *dummy = NULL;
  return lookupPQVariable(name, flags, dummy);
}

// NOTE: It is *not* the job of this function to do overload
// resolution!  If the client wants that done, it must do it itself,
// *after* doing the lookup.
Variable *Env::lookupPQVariable_internal(PQName const *name, LookupFlags flags, 
                                         Scope *&scope)
{
  Variable *var;

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
    var = lookupVariable(name->getName(), flags, scope);
  }

  if (var) {
    // get the final component of the name, so we can inspect
    // whether it has template arguments
    PQName const *final = name->getUnqualifiedNameC();

    // reference to a class in whose scope I am?
    if (var->hasFlag(DF_SELFNAME)) {
      if (!final->isPQ_template()) {
        // cppstd 14.6.1 para 1: if the name refers to a template
        // in whose scope we are, then it need not have arguments
        trace("template") << "found bare reference to enclosing template: "
                          << var->name << "\n";
        return var;
      }
      else {
        // TODO: if the template arguments match the current
        // (DF_SELFNAME) declaration, then we're talking about 'var',
        // otherwise we go through instantiateTemplate(); for now I'm
        // just going to assume we're talking about 'var'
        return var;
      }
    }

    // compare the name's template status to whether template
    // arguments were supplied; note that you're only *required*
    // to pass arguments for template classes, since template
    // functions can infer their arguments
    if (var->isTemplate()) {
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

        // sm: I think this assertion should be removed.  It asserts
        // a fact that is only tangentially related to the code at hand.
        //xassert(var->type->asCompoundType()->getTypedefVar() == var);
        //
        // in fact, as I suspected, it is wrong; using-directives can
        // create aliases (e.g. t0194.cc)
        return instantiateTemplate_astArgs(loc(), scope, var, NULL /*inst*/,
                                           final->asPQ_templateC()->args);
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
          return instantiateTemplate_astArgs(loc(), scope, var, NULL /*inst*/,
                                             final->asPQ_templateC()->args);
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
  }

  return var;
}

// NOTE: It is *not* the job of this function to do overload
// resolution!  If the client wants that done, it must do it itself,
// *after* doing the lookup.
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
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    CompoundType *ret = scope->lookupCompound(name->getName(), flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no class/struct/union called `"
        << name->getName() << "'",
        EF_DISAMBIGUATES);
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
  return takeCTemplateInfo(NULL /*baseName*/);
}

TemplateInfo * /*owner*/ Env::takeCTemplateInfo(StringRef baseName)
{
  TemplateInfo *ret = NULL;

  Scope *s = scope();
  if (s->curTemplateParams) {
    ret = new TemplateInfo(baseName, loc());
    ret->params.concat(s->curTemplateParams->params);
    delete s->curTemplateParams;
    s->curTemplateParams = NULL;
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
  ct->setTemplateInfo(takeCTemplateInfo(name));

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
    Variable *tv2 = makeVariable(loc, name, tfac.cloneType(ret), DF_TYPEDEF | DF_SELFNAME);
    ct->addUniqueVariable(tv2);
    addedNewVariable(ct, tv2);
  }

  return ret;
}


// ----------------- template instantiation -------------
static bool doesUnificationRequireBindings
  (TypeFactory &tfac,
   SObjList<STemplateArgument> &sargs,
   ObjList<STemplateArgument> &arguments)
{
  // re-unify and check that no bindings get added
  MatchTypes match(tfac, MatchTypes::MM_BIND);
  bool unifies = match.match_Lists(sargs, arguments, 2 /*matchDepth*/);
  xassert(unifies);             // should of course still unify
  // bindings should be trivial for a complete specialization
  // or instantiation
  return !match.bindings.isEmpty();
}


void Env::insertBindingsForPrimary
  (Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  xassert(baseV);
  SObjListIter<Variable> paramIter(baseV->templateInfo()->params);
  SObjListIterNC<STemplateArgument> argIter(sargs);
  while (!paramIter.isDone()) {
    Variable const *param = paramIter.data();
    
    // if we have exhaused the explicit arguments, use a NULL 'sarg'
    // to indicate that we need to use the default arguments from
    // 'param' (if available)
    STemplateArgument *sarg = argIter.isDone()? NULL : argIter.data();

    if (sarg && sarg->isTemplate()) {
      xassert("Template template parameters are not implemented");
    }

    if (param->hasFlag(DF_TYPEDEF) &&
        (!sarg || sarg->isType())) {
      if (!sarg && !param->defaultParamType) {
        error(stringc
          << "too few template arguments to `" << baseV->name << "'");
        return;
      }

      // bind the type parameter to the type argument
      Type *t = sarg? sarg->getType() : param->defaultParamType;
      Variable *binding = makeVariable(param->loc, param->name,
                                       t, DF_TYPEDEF);
      addVariable(binding);
    }
    else if (!param->hasFlag(DF_TYPEDEF) &&
             (!sarg || sarg->isObject())) {
      if (!sarg && !param->value) {
        error(stringc
          << "too few template arguments to `" << baseV->name << "'");
        return;
      }

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

      // set 'bindings->value', in some cases creating AST
      if (!sarg) {
        binding->value = param->value;

        // sm: the statement above seems reasonable, but I'm not at
        // all convinced it's really right... has it been tcheck'd?
        // has it been normalized?  are these things necessary?  so
        // I'll wait for a testcase to remove this assertion... before
        // this assertion *is* removed, someone should read over the
        // applicable parts of cppstd
        xfailure("unimplemented: default non-type argument");
      }
      else if (sarg->kind == STemplateArgument::STA_INT) {
        E_intLit *value0 = buildIntegerLiteralExp(sarg->getInt());
        // FIX: I'm sure we can do better than SL_UNKNOWN here
        value0->type = tfac.getSimpleType(SL_UNKNOWN, ST_INT, CV_CONST);
        binding->value = value0;
      } 
      else if (sarg->kind == STemplateArgument::STA_REFERENCE) {
        E_variable *value0 = new E_variable(NULL /*PQName*/);
        value0->var = sarg->getReference();
        binding->value = value0;
      }
      else {
        unimp(stringc << "STemplateArgument objects that are of kind other than "
              "STA_INT are not implemented: " << sarg->toString());
        return;
      }
      xassert(binding->value);
      addVariable(binding);
    }
    else {                 
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      // FIX: make a provision for template template parameters here
      char const *argKind = sarg->isType()? "type" : "non-type";
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            << " parameter, but `" << sarg->toString() << "' is a "
            << argKind << " argument", EF_STRONG);
    }

    paramIter.adv();
    if (!argIter.isDone()) {
      argIter.adv();
    }
  }

  xassert(paramIter.isDone());
  if (!argIter.isDone()) {
    error(stringc
          << "too many template arguments to `" << baseV->name << "'", EF_STRONG);
  }
}


void Env::insertBindingsForPartialSpec
  (Variable *baseV, MatchBindings &bindings)
{
  for(SObjListIterNC<Variable> paramIter(baseV->templateInfo()->params);
      !paramIter.isDone();
      paramIter.adv()) {
    Variable *param = paramIter.data();
    STemplateArgument *arg = NULL;
    if (param->type->isTypeVariable()) {
      arg = bindings.getTypeVar(param->type->asTypeVariable());
    } else {
      arg = bindings.getObjVar(param);
    }
    if (!arg) {
      error(stringc
            << "during partial specialization parameter `" << param->name
            << "' not bound in inferred bindings", EF_STRONG);
      return;
    }

    if (param->hasFlag(DF_TYPEDEF) &&
        arg->isType()) {
      // bind the type parameter to the type argument
      Variable *binding =
        makeVariable(param->loc, param->name,
                     // FIX: perhaps this clone type is gratuitous
                     tfac.cloneType(arg->value.t),
                     DF_TYPEDEF);
      addVariable(binding);
    }
    else if (!param->hasFlag(DF_TYPEDEF) &&
             arg->isObject()) {
      if (param->type->isIntegerType() && arg->kind == STemplateArgument::STA_INT) {
        // bind the int parameter directly to the int expression; this
        // will suffice for checking the template body, because I can
        // const-eval the expression whenever it participates in type
        // determination; the type must be made 'const' so that
        // E_variable::constEval will believe it can evaluate it
        Type *bindType =
          // FIX: perhaps this clone type is gratuitous
          tfac.cloneType(tfac.applyCVToType
                         (param->loc, CV_CONST,
                          param->type, NULL /*syntax*/));
        Variable *binding = makeVariable(param->loc, param->name,
                                         bindType, DF_NONE);
        // we omit the text as irrelevant and expensive to reconstruct
        binding->value = new E_intLit(NULL);
        binding->value->asE_intLit()->i = arg->value.i;
        addVariable(binding);
      } else {
        unimp(stringc
              << "non-integer non-type non-template template arguments not implemented");
      }
    }
    else {                 
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      char const *argKind = arg->isType()? "type" : "non-type";
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            // FIX: might do better than what toString() gives
            << " parameter, but `" << arg->toString() << "' is a "
            << argKind << " argument", EF_STRONG);
    }
  }

  // Note that it is not necessary here to attempt to detect too few
  // or too many arguments for the given parameters.  If the number
  // doesn't match then the unification will fail and we will attempt
  // to instantiate the primary, which will then report this error.
}


void Env::insertBindings(Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  if (baseV->templateInfo()->isPartialSpec()) {
    // unify again to compute the bindings again since we forgot
    // them already
    MatchTypes match(tfac, MatchTypes::MM_BIND);
    bool matches = match.match_Lists(sargs, baseV->templateInfo()->arguments, 2 /*matchDepth*/);
    xassert(matches);           // this is bad if it fails
    // FIX: this
    //        if (tracingSys("template")) {
    //          cout << "bindings generated by unification with partial-specialization "
    //            "specialization-pattern" << endl;
    //          printBindings(match.bindings);
    //        }
    insertBindingsForPartialSpec(baseV, match.bindings);
  } else {
    xassert(baseV->templateInfo()->isPrimary());
    insertBindingsForPrimary(baseV, sargs);
  }
}


// go over the list of arguments, and make a list of semantic
// arguments
void Env::templArgsASTtoSTA
  (ASTList<TemplateArgument> const &arguments,
   SObjList<STemplateArgument> &sargs)
{
  FOREACH_ASTLIST(TemplateArgument, arguments, iter) {
    // the caller wants me not to modify the list, and I am not going
    // to, but I need to put non-const pointers into the 'sargs' list
    // since that's what the interface to 'equalArguments' expects..
    // this is a problem with const-polymorphism again
    TemplateArgument *ta = const_cast<TemplateArgument*>(iter.data());
    if (!ta->sarg.hasValue()) {
      error(stringc << "TemplateArgument has no value " << ta->argString(), EF_STRONG);
      return;
    }
    sargs.append(&(ta->sarg));
  }
}


Variable *Env::instantiateTemplate_astArgs
  (SourceLoc loc, Scope *foundScope, 
   Variable *baseV, Variable *instV,
   ASTList<TemplateArgument> const &astArgs)
{
  SObjList<STemplateArgument> sargs;
  templArgsASTtoSTA(astArgs, sargs);
  return instantiateTemplate(loc, foundScope, baseV, instV, NULL /*bestV*/, sargs);
}


MatchTypes::MatchMode Env::mapTcheckModeToTypeMatchMode(TemplTcheckMode tcheckMode)
{
  // map the typechecking mode to the type matching mode
  MatchTypes::MatchMode matchMode = MatchTypes::MM_NONE;
  switch(tcheckMode) {
  default: xfailure("bad mode"); break;
  case TTM_1NORMAL:
    matchMode = MatchTypes::MM_BIND;
    break;
  case TTM_2TEMPL_FUNC_DECL:
    matchMode = MatchTypes::MM_ISO;
    break;
  case TTM_3TEMPL_DEF:
    xfailure("mapTcheckModeToTypeMatchMode: shouldn't be here TTM_3TEMPL_DEF mode");
    break;
  }
  return matchMode;
}


Variable *Env::findMostSpecific(Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  // baseV should be a template primary
  xassert(baseV->templateInfo()->isPrimary());
  if (tracingSys("template")) {
    cout << "Env::instantiateTemplate: "
         << "template instantiation, searching instantiations of primary "
         << "for best match to the template arguments"
         << endl;
    baseV->templateInfo()->debugPrint();
  }

  MatchTypes::MatchMode matchMode = mapTcheckModeToTypeMatchMode(getTemplTcheckMode());

  // iterate through all of the instantiations and build up an
  // ObjArrayStack<Candidate> of candidates
  TemplCandidates templCandidates(tfac);
  SFOREACH_OBJLIST_NC(Variable, baseV->templateInfo()->getInstantiations(), iter) {
    Variable *var0 = iter.data();
    TemplateInfo *templInfo0 = var0->templateInfo();
    xassert(templInfo0);        // should have templateness
    // Sledgehammer time.
    if (templInfo0->isMutant()) continue;
    // see if this candidate matches
    MatchTypes match(tfac, matchMode);
    if (match.match_Lists(sargs, templInfo0->arguments, 2 /*matchDepth*/)) {
      templCandidates.candidates.push(var0);
    }
  }

  // there are no candidates so we just use the primary
  if (templCandidates.candidates.isEmpty()) {
    return baseV;
  }

  // if there are any candidates to try, select the best
  Variable *bestV = selectBestCandidate_templCompoundType(templCandidates);

  // if there is not best candidiate, then the call is ambiguous and
  // we should deal with that error
  if (!bestV) {
    // FIX: what is the right thing to do here?
    error(stringc << "ambiguous attempt to instantiate template", EF_STRONG);
    return NULL;
  }
  // otherwise, use the best one

  // if the best is an instantiation/complete specialization check
  // that no bindings get generated during unification
  if (bestV->templateInfo()->isCompleteSpecOrInstantiation()) {
    xassert(!doesUnificationRequireBindings
            (tfac, sargs, bestV->templateInfo()->arguments));
  }
  // otherwise it is a partial specialization

  return bestV;
}


// remove scopes from the environment until the innermost
// scope on the scope stack is the same one that the template
// definition appeared in; template definitions can see names
// visible from their defining scope only [cppstd 14.6 para 1]
//
// update: (e.g. t0188.cc) pop scopes until we reach one that
// *contains* (or equals) the defining scope
//
// 4/20/04: Even more (e.g. t0204.cc), we need to push scopes
// to dig back down to the defining scope.  So the picture now
// looks like this:
//
//       global                   /---- this is "foundScope"
//         |                     /
//         namespace1           /   }
//         | |                 /    }- 2. push these: "pushedScopes"
//         | namespace11  <---/     }
//         |   |
//         |   template definition
//         |
//         namespace2               }
//           |                      }- 1. pop these: "poppedScopes"
//           namespace21            }
//             |
//             point of instantiation
//
// actually, it's *still* not right, because
//   - this allows the instantiation to see names declared in
//     'namespace11' that are below the template definition, and
//   - it's entirely wrong for dependent names, a concept I
//     largely ignore at this time
// however, I await more examples before continuing to refine
// my approximation to the extremely complex lookup rules
Scope *Env::prepArgScopeForTemlCloneTcheck
  (ObjList<Scope> &poppedScopes, SObjList<Scope> &pushedScopes, Scope *foundScope)
{
  xassert(foundScope);
  // pop scope scopes
  while (!scopes.first()->enclosesOrEq(foundScope)) {
    poppedScopes.prepend(scopes.removeFirst());
    if (scopes.isEmpty()) {
      xfailure("emptied scope stack searching for defining scope");
    }
  }

  // make a list of the scopes to push; these form a path from our
  // current scope to the 'foundScope'
  Scope *s = foundScope;
  while (s != scopes.first()) {
    pushedScopes.prepend(s);
    s = s->parentScope;
    if (!s) {
      if (scopes.first()->isGlobalScope()) {
        // ok, hit the global scope in the traversal
        break;
      }
      else {
        xfailure("missed the current scope while searching up "
                 "from the defining scope");
      }
    }
  }

  // now push them in list order, which means that 'foundScope'
  // will be the last one to be pushed, and hence the innermost
  // (I waited until now b/c this is the opposite order from the
  // loop above that fills in 'pushedScopes')
  SFOREACH_OBJLIST_NC(Scope, pushedScopes, iter) {
    // Scope 'iter.data()' is now on both lists, but neither owns
    // it; 'scopes' does not own Scopes that are named, as explained
    // in the comments near its declaration (cc_env.h)
    scopes.prepend(iter.data());
  }

  // make a new scope for the template arguments
  Scope *argScope = enterScope(SK_TEMPLATE, "template argument bindings");

  return argScope;
}


void Env::unPrepArgScopeForTemlCloneTcheck
  (Scope *argScope, ObjList<Scope> &poppedScopes, SObjList<Scope> &pushedScopes)
{
  // remove the argument scope
  exitScope(argScope);

  // restore the original scope structure
  pushedScopes.reverse();
  while (pushedScopes.isNotEmpty()) {
    // make sure the ones we're removing are the ones we added
    xassert(scopes.first() == pushedScopes.first());
    scopes.removeFirst();
    pushedScopes.removeFirst();
  }

  // re-add the inner scopes removed above
  while (poppedScopes.isNotEmpty()) {
    scopes.prepend(poppedScopes.removeFirst());
  }
}


static bool doSTemplArgsContainVars(SObjList<STemplateArgument> &sargs)
{
  SFOREACH_OBJLIST_NC(STemplateArgument, sargs, iter) {
    if (iter.data()->containsVariables()) return true;
  }
  return false;
}


// see comments in cc_env.h
//
// sm: TODO: I think this function should be split into two, one for
// classes and one for functions.  To the extent they share mechanism
// it would be better to encapsulate the mechanism than to share it
// by threading two distinct flow paths through the same code.
//
//
// dsw: There are three dimensions to the space of paths through this
// code.
//
// 1 - class or function template
// 
// 2 - baseForward: true exactly when we are typechecking a forward
// declaration and not a definition.
//
// 3 - instV: non-NULL when
//   a) a template primary was forward declared
//   b) it has now been defined
//   c) it has forwarded instantiations (& specializations?)  which
//   when they were instantiated, did not find a specialization and so
//   they are now being instantiated
// Note the test 'if (!instV)' below.  In this case, we are always
// going to instantiate the primary and not the specialization,
// because if the specialization comes after the instantiation point,
// it is not relevant anyway.
//
//
// dsw: We have three typechecking modes:
//
//   1) normal: template instantiations are done fully;
//
//   2) template function declaration: declarations are instantiated
//   but not definitions;
//
//   3) template definition: template instantiation request just result
//   in the primary being returned.
//
// Further, inside template definitions, default arguments are not
// typechecked and function template calls do not attempt to infer
// their template arguments from their function arguments.
//
// Two stacks in Env allow us to distinguish these modes record when
// the type-checker is typechecking a node below a TemplateDeclaration
// and a D_func/Initializer respectively.  See the implementation of
// Env::getTemplTcheckMode() for the details.
Variable *Env::instantiateTemplate
  (SourceLoc loc,
   Scope *foundScope,
   Variable *baseV,
   Variable *instV,
   Variable *bestV,
   SObjList<STemplateArgument> &sargs,
   Variable *funcFwdInstV)
{
  // NOTE: There is an ordering bug here, e.g. it fails t0209.cc.  The
  // problem here is that we choose an instantiation (or create one)
  // based only on the arguments explicitly present.  Since default
  // arguments are delayed until later, we don't realize that C<> and
  // C<int> are the same class if it has a default argument of 'int'.
  //
  // The fix I think is to insert argument bindings first, *then*
  // search for or create an instantiation.  In some cases this means
  // we bind arguments and immediately throw them away (i.e. when an
  // instantiation already exists), but I don't see any good
  // alternatives.

  xassert(baseV->templateInfo()->isPrimary());
  // if we are in a template definition typechecking mode just return
  // the primary
  TemplTcheckMode tcheckMode = getTemplTcheckMode();
  if (tcheckMode == TTM_3TEMPL_DEF) {
    return baseV;
  }

  // maintain the inst loc stack (ArrayStackPopper is in smbase/array.h)
  ArrayStackPopper<SourceLoc> pusher_popper(instantiationLocStack, loc /*pushVal*/);

  // 1 **** what template are we going to instanitate?  We now find
  // the instantiation/specialization/primary most specific to the
  // template arguments we have been given.  If it is a complete
  // specialization, we use it as-is.  If it is a partial
  // specialization, we reassign baseV.  Otherwise we fall back to the
  // primary.  At the end of this section if we have not returned then
  // baseV is the template to instantiate.

  Variable *oldBaseV = baseV;   // save baseV for other uses later
  // has this class already been instantiated?
  if (!instV) {
    // Search through the instantiations of this primary and do an
    // argument/specialization pattern match.
    if (!bestV) {
      // FIX: why did I do this?  It doesn't work.
//        if (tcheckMode == TTM_2TEMPL_FUNC_DECL) {
//          // in mode 2, just instantiate the primary
//          bestV = baseV;
//        }
      bestV = findMostSpecific(baseV, sargs);
      // if no bestV then the lookup was ambiguous, error has been reported
      if (!bestV) return NULL;
    }
    if (bestV->templateInfo()->isCompleteSpecOrInstantiation()) {
      return bestV;
    }
    baseV = bestV;              // baseV is saved in oldBaseV above
  }
  // there should be something non-trivial to instantiate
  xassert(baseV->templateInfo()->argumentsContainVariables()
          // or the special case of a primary, which doesn't have
          // arguments but acts as if it did
          || baseV->templateInfo()->isPrimary()
          );

  // render the template arguments into a string that we can use
  // as the name of the instantiated class; my plan is *not* that
  // this string serve as the unique ID, but rather that it be
  // a debugging aid only
  //
  // dsw: UPDATE: now it's used during type construction for
  // elaboration, so has to be just the base name
  StringRef instName;
  bool baseForward = false;
  bool sTemplArgsContainVars = doSTemplArgsContainVars(sargs);
  if (baseV->type->isCompoundType()) {
    if (tcheckMode == TTM_2TEMPL_FUNC_DECL) {
      // This ad-hoc condition is to prevent a class template
      // instantiation in mode 2 that happens to not have any
      // quantified template variables from being treated as
      // forwareded.  Otherwise, what will happen is that if the
      // same template arguments are seen in mode 3, the class will
      // not be re-instantiated and a normal mode 3 class will have
      // no body.  To see this happen turn of the next line and run
      // in/d0060.cc and watch it fail.  Now comment out the 'void
      // g(E<A> &a)' in d0060.cc and run again and watch it pass.
      if (sTemplArgsContainVars) {
        // don't instantiate the template in a function template
        // definition parameter list
        baseForward = true;
      }
    } else {
      baseForward = baseV->type->asCompoundType()->forward;
    }
    instName = baseV->type->asCompoundType()->name;
  } else {
    xassert(baseV->type->isFunctionType());
    // we should never get here in TTM_2TEMPL_FUNC_DECL mode
    xassert(tcheckMode != TTM_2TEMPL_FUNC_DECL);
    Declaration *declton = baseV->templateInfo()->declSyntax;
    if (declton && !baseV->funcDefn) {
      Declarator *decltor = declton->decllist->first();
      xassert(decltor);
      // FIX: is this still necessary?
      baseForward = (decltor->context == DC_TD_PROTO);
      // lets find out
      xassert(baseForward);
    } else {
      baseForward = false;
    }
    instName = baseV->name;     // FIX: just something for now
  }

  // A non-NULL instV is marked as no longer forwarded before being
  // passed in.
  if (instV) {
    xassert(!baseForward);
  }
  trace("template") << (baseForward ? "(forward) " : "")
                    << "instantiating class: "
                    << instName << sargsToString(sargs) << endl;

  // 2 **** Prepare the argument scope for template instantiation

  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = NULL;
  // don't mess with it if not going to tcheck anything; we need it in
  // either case for function types
  if (!(baseForward && baseV->type->isCompoundType())) {
    argScope = prepArgScopeForTemlCloneTcheck(poppedScopes, pushedScopes, foundScope);
    insertBindings(baseV, sargs);
  }

  // 3 **** make a copy of the template definition body AST (unless
  // this is a forward declaration)

  TS_classSpec *copyCpd = NULL;
  TS_classSpec *cpdBaseSyntax = NULL; // need this below
  Function *copyFun = NULL;
  if (baseV->type->isCompoundType()) {
    cpdBaseSyntax = baseV->type->asCompoundType()->syntax;
    if (baseForward) {
      copyCpd = NULL;
    } else {
      xassert(cpdBaseSyntax);
      copyCpd = cpdBaseSyntax->clone();
    }
  } else {
    xassert(baseV->type->isFunctionType());
    Function *baseSyntax = baseV->funcDefn;
    if (baseForward) {
      copyFun = NULL;
    } else {
      xassert(baseSyntax);
      copyFun = baseSyntax->clone();
    }
  }

  // FIX: merge these
  if (copyCpd && tracingSys("cloneAST")) {
    cout << "--------- clone of " << instName << " ------------\n";
    copyCpd->debugPrint(cout, 0);
  }
  if (copyFun && tracingSys("cloneAST")) {
    cout << "--------- clone of " << instName << " ------------\n";
    copyFun->debugPrint(cout, 0);
  }

  // 4 **** make the Variable that will represent the instantiated
  // class during typechecking of the definition; this allows template
  // classes that refer to themselves and recursive function templates
  // to work.  The variable is assigned to instV; if instV already
  // exists, this work does not need to be done: the template was
  // previously forwarded and that forwarded definition serves the
  // purpose.

  if (!instV) {
    // copy over the template arguments so we can recognize this
    // instantiation later
    StringRef name = baseV->type->isCompoundType()
      ? baseV->type->asCompoundType()->name    // no need to call 'str', 'name' is already a StringRef
      : NULL;
    TemplateInfo *instTInfo = new TemplateInfo(name, loc);
    instTInfo->instantiatedFrom = baseV;
    SFOREACH_OBJLIST(STemplateArgument, sargs, iter) {
      instTInfo->arguments.append(new STemplateArgument(*iter.data()));
    }

    // record the location which provoked this instantiation; this
    // information will be useful if it turns out we can only to a
    // forward-declaration here, since later when we do the delayed
    // instantiation, 'loc' will be only be available because we
    // stashed it here
    //instTInfo->instLoc = loc;
    // actually, this is now part of the constructor argument above, 
    // but I'll leave the comment

    // FIX: Scott, its almost as if you just want to clone the type
    // here.
    //
    // sm: No, I want to type-check the instantiated cloned AST.  The
    // resulting type will be quite different, since it will refer to
    // concrete types instead of TypeVariables.
    if (baseV->type->isCompoundType()) {
      // 1/21/03: I had been using 'instName' as the class name, but
      // that causes problems when trying to recognize constructors
      CompoundType *instVCpdType = tfac.makeCompoundType(baseV->type->asCompoundType()->keyword,
                                                         baseV->type->asCompoundType()->name);
      instVCpdType->instName = instName; // stash it here instead
      instVCpdType->forward = baseForward;

      // wrap the compound in a regular type
      SourceLoc copyLoc = copyCpd ? copyCpd->loc : SL_UNKNOWN;
      Type *type = makeType(copyLoc, instVCpdType);

      // make a fake implicit typedef; this class and its typedef
      // won't actually appear in the environment directly, but making
      // the implicit typedef helps ensure uniformity elsewhere; also
      // must be done before type checking since it's the typedefVar
      // field which is returned once the instantiation is found via
      // 'instantiations'
      instV = makeVariable(copyLoc, instName, type,
                           DF_TYPEDEF | DF_IMPLICIT);
      instV->type->asCompoundType()->typedefVar = instV;

      if (lang.compoundSelfName) {
        // also make the self-name, which *does* go into the scope
        // (testcase: t0167.cc)
        Variable *var2 = makeVariable(copyLoc, instName, type,
                                      DF_TYPEDEF | DF_SELFNAME);
        instV->type->asCompoundType()->addUniqueVariable(var2);
        addedNewVariable(instV->type->asCompoundType(), var2);
      }
    } else {
      xassert(baseV->type->isFunctionType());
      // sm: It seems to me the sequence should be something like this:
      //   1. bind template parameters to concrete types
      //        dsw: this has been done already above
      //   2. tcheck the declarator portion, thus yielding a FunctionType
      //      that refers to concrete types (not TypeVariables), and
      //      also yielding a Variable that can be used to name it
      //        dsw: this is done here
      if (copyFun) {
        xassert(!baseForward);
        // NOTE: 1) the whole point is that we don't check the body,
        // and 2) it is very important that we do not add the variable
        // to the namespace, otherwise the primary is masked if the
        // template body refers to itself
        copyFun->tcheck(*this, false /*checkBody*/,
                        false /*reallyAddVariable*/, funcFwdInstV /*prior*/);
        instV = copyFun->nameAndParams->var;
        //   3. add said Variable to the list of instantiations, so if the
        //      function recursively calls itself we'll be ready
        //        dsw: this is done below
        //   4. tcheck the function body
        //        dsw: this is done further below.
      } else {
        xassert(baseForward);
        // We do have to clone the forward declaration before
        // typechecking.
        Declaration *fwdDecl = baseV->templateInfo()->declSyntax;
        xassert(fwdDecl);
        // use the same context again but make sure it is well defined
        xassert(fwdDecl->decllist->count() == 1);
        DeclaratorContext ctxt = fwdDecl->decllist->first()->context;
        xassert(ctxt != DC_UNKNOWN);
        Declaration *copyDecl = fwdDecl->clone();
        xassert(argScope);
        xassert(!funcFwdInstV);
        copyDecl->tcheck(*this, ctxt,
                         false /*reallyAddVariable*/, NULL /*prior*/);
        xassert(copyDecl->decllist->count() == 1);
        Declarator *copyDecltor = copyDecl->decllist->first();
        instV = copyDecltor->var;
      }
    }

    xassert(instV);
    xassert(foundScope);
    foundScope->registerVariable(instV);

    // tell the base template about this instantiation; this has to be
    // done before invoking the type checker, to handle cases where the
    // template refers to itself recursively (which is very common)
    //
    // dsw: this is the one place where I use oldBase instead of base;
    // looking at the other code above, I think it is the only one
    // where I should, but I'm not sure.
    xassert(oldBaseV->templateInfo() && oldBaseV->templateInfo()->isPrimary());
    // dsw: this had to be moved down here as you can't get the
    // typedefVar until it exists
    instV->setTemplateInfo(instTInfo);
    if (funcFwdInstV) {
      // addInstantiation would have done this
      instV->templateInfo()->setMyPrimary(oldBaseV->templateInfo());
    } else {
      // NOTE: this can mutate instV if instV is a duplicated mutant
      // in the instantiation list; FIX: perhaps find another way to
      // prevent creating duplicate mutants in the first place; this
      // is quite wasteful if we have cloned an entire class of AST
      // only to throw it away again
      Variable *newInstV = oldBaseV->templateInfo()->
        addInstantiation(instV, true /*suppressDup*/);
      if (newInstV != instV) {
        // don't do stage 5 below; just use the newInstV and be done
        // with it
        copyCpd = NULL;
        copyFun = NULL;
        instV = newInstV;
      }
      xassert(instV->templateInfo()->getMyPrimaryIdem() == oldBaseV->templateInfo());
    }
    xassert(instTInfo->isNotPrimary());
    xassert(instTInfo->getMyPrimaryIdem() == oldBaseV->templateInfo());
  }

  // 5 **** typecheck the cloned AST

  if (copyCpd || copyFun) {
    xassert(!baseForward);
    xassert(argScope);

    // we are about to tcheck the cloned AST.. it might be that we
    // were in the context of an ambiguous expression when the need
    // for instantiated arose, but we want the tchecking below to
    // proceed as if it were not ambiguous (it isn't ambiguous, it
    // just might be referenced from a context that is)
    //
    // so, the following object will temporarily set the level to 0,
    // and restore its original value when the block ends
    {
      DisambiguateNestingTemp nestTemp(*this, 0);

      if (instV->type->isCompoundType()) {
        // FIX: unlike with function templates, we can't avoid
        // typechecking the compound type clone when not in 'normal'
        // mode because otherwise implicit members such as ctors don't
        // get elaborated into existence
//          if (getTemplTcheckMode() == TTM_1NORMAL) { . . .
        xassert(copyCpd);
        // invoke the TS_classSpec typechecker, giving to it the
        // CompoundType we've built to contain its declarations; for
        // now I don't support nested template instantiation
        copyCpd->ctype = instV->type->asCompoundType();
        // preserve the baseV and sargs so that when the member
        // function bodies are typechecked later we have them
        InstContext *instCtxt = new InstContext(baseV, instV, foundScope, sargs);
        copyCpd->tcheckIntoCompound
          (*this,
           DF_NONE,
           copyCpd->ctype,
           false /*inTemplate*/,
           // that's right, really check the member function bodies
           // exactly when we are instantiating something that is not
           // a complete specialization; if it *is* a complete
           // specialization, then the tcheck will be done later, say,
           // when the function is used
           sTemplArgsContainVars /*reallyTcheckFunctionBodies*/,
           instCtxt,
           foundScope,
           NULL /*containingClass*/);
        // this is turned off because it doesn't work: somewhere the
        // mutants are actually needed; Instead we just avoid them
        // above.
        //      deMutantify(baseV);

        // find the funcDefn's for all of the function declarations
        // and then instantiate them; FIX: do the superclass members
        // also?
        FOREACH_ASTLIST_NC(Member, cpdBaseSyntax->members->list, memIter) {
          Member *mem = memIter.data();
          if (!mem->isMR_decl()) continue;
          Declaration *decltn = mem->asMR_decl()->d;
          Declarator *decltor = decltn->decllist->first();
          // this seems to happen with anonymous enums
          if (!decltor) continue;
          if (!decltor->var->type->isFunctionType()) continue;
          xassert(decltn->decllist->count() == 1);
          Function *funcDefn = decltor->var->funcDefn;
          // skip functions that haven't been defined yet; hopefully
          // they'll be defined later and if they are, their
          // definitions will be patched in then
          if (!funcDefn) continue;
          // I need to instantiate this funcDefn.  This means: 1)
          // clone it; [NOTE: this is not being done: 2) all the
          // arguments are already in the scope, but if the definition
          // named them differently, it won't work, so throw out the
          // current template scope and make another]; then 3) just
          // typecheck it.
          Function *copyFuncDefn = funcDefn->clone();
          // find the instantiation of the cloned declaration that
          // goes with it; FIX: this seems brittle to me; I'd rather
          // have something like a pointer from the cloned declarator
          // to the one it was cloned from.
          Variable *funcDefnInstV = instV->type->asCompoundType()->lookupVariable
            (decltor->var->name, *this,
             LF_INNER_ONLY |
             // this shouldn't be necessary, but if it turns out to be
             // a function template, we don't want it instantiated
             LF_TEMPL_PRIMARY);
          xassert(funcDefnInstV);
          xassert(strcmp(funcDefnInstV->name, decltor->var->name)==0);
          if (funcDefnInstV->templateInfo()) {
            // FIX: I don't know what to do with function template
            // members of class templates just now, but what we do
            // below is probably wrong, so skip them
            continue;
          }
          xassert(!funcDefnInstV->funcDefn);

          // FIX: I wonder very much if the right thing is happening
          // to the scopes at this point.  I think all the current
          // scopes up to the global scope need to be removed, and
          // then the template scope re-created, and then typecheck
          // this.  The extra scopes are probably harmless, but
          // shouldn't be there.

          // save all the scopes from the foundScope down to the
          // current scope; Scott: I think you are right that I didn't
          // need to save them all, but I can't see how to avoid
          // saving these.  How else would I re-create them?  Note
          // that this is idependent of the note above about fixing
          // the scope stack: we save whatever is there before
          // entering Function::tcheck()
          xassert(instCtxt);
          xassert(foundScope);
          FuncTCheckContext *tcheckCtxt = new FuncTCheckContext
            (copyFuncDefn, foundScope, shallowClonePartialScopeStack(foundScope));

          // urk, there's nothing to do here.  The declaration has
          // already been tchecked, and we don't want to tcheck the
          // definition yet, so we can just point the var at the
          // cloned definition; I'll run the tcheck with
          // checkBody=false anyway just for uniformity.
          copyFuncDefn->tcheck(*this,
                               false /*checkBody*/,
                               false /*reallyAddVariable*/,
                               funcDefnInstV /*prior*/);
          // pase in the definition for later use
          xassert(!funcDefnInstV->funcDefn);
          funcDefnInstV->funcDefn = copyFuncDefn;

          // preserve the instantiation context
          xassert(funcDefnInstV = copyFuncDefn->nameAndParams->var);
          copyFuncDefn->nameAndParams->var->setInstCtxts(instCtxt, tcheckCtxt);
        }
      } else {
        xassert(instV->type->isFunctionType());
        // if we are in a template definition, don't typecheck the
        // cloned function body
        if (tcheckMode == TTM_1NORMAL) {
          xassert(copyFun);
          copyFun->funcType = instV->type->asFunctionType();
          xassert(scope()->isGlobalTemplateScope());
          // NOTE: again we do not add the variable to the namespace
          copyFun->tcheck(*this, true /*checkBody*/,
                          false /*reallyAddVariable*/, instV /*prior*/);
          xassert(instV->funcDefn == copyFun);
        }
      }
    }

    if (tracingSys("cloneTypedAST")) {
      cout << "--------- typed clone of " 
           << instName << sargsToString(sargs) << " ------------\n";
      if (copyCpd) copyCpd->debugPrint(cout, 0);
      if (copyFun) copyFun->debugPrint(cout, 0);
    }
  }
  // this else case can now happen if we find that there was a
  // duplicated instV and therefore jump out during stage 4
//    } else {
//      xassert(baseForward);
//    }

  // 6 **** Undo the scope, reversing step 2
  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck(argScope, poppedScopes, pushedScopes);
    argScope = NULL;
  }
  // make sure we haven't forgotten these
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());

  return instV;
}


// given a name that was found without qualifiers or template arguments,
// see if we're currently inside the scope of a template definition
// with that name
CompoundType *Env::findEnclosingTemplateCalled(StringRef name)
{
  FOREACH_OBJLIST(Scope, scopes, iter) {
    Scope const *s = iter.data();

    if (s->curCompound &&
        s->curCompound->templateInfo() &&
        s->curCompound->templateInfo()->baseName == name) {
      return s->curCompound;
    }
  }
  return NULL;     // not found
}


void Env::provideDefForFuncTemplDecl
  (Variable *forward, TemplateInfo *primaryTI, Function *f)
{
  xassert(forward);
  xassert(primaryTI);
  xassert(forward->templateInfo()->getMyPrimaryIdem() == primaryTI);
  xassert(primaryTI->isPrimary());
  Variable *fVar = f->nameAndParams->var;
  // update things in the declaration; I copied this from
  // Env::createDeclaration()
  TRACE("odr",    "def'n of " << forward->name
        << " at " << toString(f->getLoc())
        << " overrides decl at " << toString(forward->loc));
  forward->loc = f->getLoc();
  forward->setFlag(DF_DEFINITION);
  forward->clearFlag(DF_EXTERN);
  forward->clearFlag(DF_FORWARD); // dsw: I added this
  // make this idempotent
  if (forward->funcDefn) {
    xassert(forward->funcDefn == f);
  } else {
    forward->funcDefn = f;
    if (tracingSys("template")) {
      cout << "definition of function template " << fVar->toString()
           << " attached to previous forwarded declaration" << endl;
      primaryTI->debugPrint();
    }
  }
  // make this idempotent
  if (fVar->templateInfo()->getMyPrimaryIdem()) {
    xassert(fVar->templateInfo()->getMyPrimaryIdem() == primaryTI);
  } else {
    fVar->templateInfo()->setMyPrimary(primaryTI);
  }
}


void Env::ensureFuncMemBodyTChecked(Variable *v)
{
  if (getTemplTcheckMode() != TTM_1NORMAL) return;
  xassert(v);
  xassert(v->getType()->isFunctionType());

  // FIX: perhaps these should be assertion failures instead
  TemplateInfo *vTI = v->templateInfo();
  if (vTI) {
    // NOTE: this is pretty weak, because it only applies to function
    // templates not to function members of class templates, and the
    // instantiation of function templates is never delayed.
    // Therefore, we might as well say "return" here.
    if (vTI->isMutant()) return;
    if (!vTI->isCompleteSpecOrInstantiation()) return;
  }

  Function *funcDefn0 = v->funcDefn;

  InstContext *instCtxt = v->instCtxt;
  // anything that wasn't part of a template instantiation
  if (!instCtxt) {
    // if this function is defined, it should have been typechecked by now;
    // UPDATE: unless it is defined later in the same class
//      if (funcDefn0) {
//        xassert(funcDefn0->hasBodyBeenTChecked);
//      }
    return;
  }

  FuncTCheckContext *tcheckCtxt = v->tcheckCtxt;
  xassert(tcheckCtxt);

  // if we are a member of a templatized class and we have not seen
  // the funcDefn, then it must come later, but this is not
  // implemented
  if (!funcDefn0) {
    unimp("class template function memeber invocation "
          "before function definition is unimplemented");
    return;
  }

  // if has been tchecked, we are done
  if (funcDefn0->hasBodyBeenTChecked) {
    return;
  }

  // OK, seems it can be a partial specialization as well
//    xassert(instCtxt->baseV->templateInfo()->isPrimary());
  xassert(instCtxt->baseV->templateInfo()->isPrimary() ||
          instCtxt->baseV->templateInfo()->isPartialSpec());
  // this should only happen for complete specializations
  xassert(instCtxt->instV->templateInfo()->isCompleteSpecOrInstantiation());

  // set up the scopes the way instantiateTemplate() would
  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = prepArgScopeForTemlCloneTcheck
    (poppedScopes, pushedScopes, instCtxt->foundScope);
  insertBindings(instCtxt->baseV, *(instCtxt->sargs));

  // mess with the scopes the way typechecking would between when it
  // leaves instantiateTemplate() and when it arrives at the function
  // tcheck
  xassert(scopes.count() >= 2);
  xassert(scopes.nth(1) == tcheckCtxt->foundScope);
  xassert(scopes.nth(0)->scopeKind == SK_TEMPLATE);
  tcheckCtxt->pss->stackIntoEnv(*this);

  xassert(funcDefn0 == tcheckCtxt->func);
  funcDefn0->tcheck(*this,
                    true /*checkBody*/,
                    false /*reallyAddVariable*/,
                    v /*prior*/);
  // should still be true
  xassert(v->funcDefn == funcDefn0);

  tcheckCtxt->pss->unStackOutOfEnv(*this);

  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck(argScope, poppedScopes, pushedScopes);
    argScope = NULL;
  }
  // make sure we haven't forgotten these
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());
}


void Env::instantiateForwardFunctions(Variable *forward, Variable *primary)
{
  TemplateInfo *primaryTI = primary->templateInfo();
  xassert(primaryTI);
  xassert(primaryTI->isPrimary());

  // temporarily supress TTM_3TEMPL_DEF and return to TTM_1NORMAL for
  // purposes of instantiating the forward function templates
  StackMaintainer<TemplateDeclaration> sm1(templateDeclarationStack, &mode1Dummy);

  // Find all the places where this declaration was instantiated,
  // where this function template specialization was
  // called/instantiated after it was declared but before it was
  // defined.  In each, now instantiate with the same template
  // arguments and fill in the funcDefn; UPDATE: see below, but now we
  // check that the definition was already provided rather than
  // providing one.
  SFOREACH_OBJLIST_NC(Variable, primaryTI->getInstantiations(), iter) {
    Variable *instV = iter.data();
    TemplateInfo *instTIinfo = instV->templateInfo();
    xassert(instTIinfo);
    xassert(instTIinfo->isNotPrimary());
    if (instTIinfo->instantiatedFrom != forward) continue;
    // should not have a funcDefn as it is instantiated from a forward
    // declaration that does not yet have a definition and we checked
    // that above already
    xassert(!instV->funcDefn);

    // instantiate this definition
    SourceLoc instLoc = instTIinfo->instLoc;

    Variable *instWithDefn = instantiateTemplate
      (instLoc,
       // FIX: I don't know why it is possible for the primary here to
       // not have a scope, but it is.  Since 1) we are in the scope
       // of the definition that we want to instantiate, and 2) from
       // experiments with gdb, I have to put the definition of the
       // template in the same scope as the declaration, I conclude
       // that I can use the same scope as we are in now
//         primary->scope,    // FIX: ??
       // UPDATE: if the primary is in the global scope, then I
       // suppose its scope might be NULL; I don't remember the rule
       // for that.  So, maybe it's this:
//         primary->scope ? primary->scope : env.globalScope()
       scope(),
       primary,
       NULL /*instV; only used by instantiateForwardClasses; this
              seems to be a fundamental difference*/,
       forward /*bestV*/,
       reinterpret_cast< SObjList<STemplateArgument>& >  // hack..
         (instV->templateInfo()->arguments),
       // don't actually add the instantiation to the primary's
       // instantiation list; we will do that below
       instV
       );
    // previously the instantiation of the forward declaration
    // 'forward' produced an instantiated declaration; we now provide
    // a definition for it; UPDATE: we now check that when the
    // definition of the function was typechecked that a definition
    // was provided for it, since we now re-use the declaration's var
    // in the defintion declartion (the first pass when
    // checkBody=false).
    //
    // FIX: I think this works for functions the definition of which
    // is going to come after the instantiation request of the
    // containing class template since I think both of these will be
    // NULL; we will find out
    xassert(instV->funcDefn == instWithDefn->funcDefn);
  }
}


void Env::instantiateForwardClasses(Scope *scope, Variable *baseV)
{
  // temporarily supress TTM_3TEMPL_DEF and return to TTM_1NORMAL for
  // purposes of instantiating the forward classes
  StackMaintainer<TemplateDeclaration> sm(templateDeclarationStack, &mode1Dummy);

  SFOREACH_OBJLIST_NC(Variable, baseV->templateInfo()->getInstantiations(), iter) {
    Variable *instV = iter.data();
    xassert(instV->templateInfo());
    CompoundType *inst = instV->type->asCompoundType();
    // this assumption is made below
    xassert(inst->templateInfo() == instV->templateInfo());

    if (inst->forward) {
      trace("template") << "instantiating previously forward " << inst->name << "\n";
      inst->forward = false;
      
      // this location was stored back when the template was
      // forward-instantiated
      SourceLoc instLoc = inst->templateInfo()->instLoc;

      instantiateTemplate(instLoc,
                          scope,
                          baseV,
                          instV /*use this one*/,
                          NULL /*bestV*/,
                          reinterpret_cast< SObjList<STemplateArgument>& >  // hack..
                            (inst->templateInfo()->arguments)
                          );
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
Type *Env::error(SourceLoc L, char const *msg, ErrorFlags eflags)
{
  bool disambiguates = !!(eflags & EF_DISAMBIGUATES);
  string instLoc = instLocStackString();
  trace("error") << (disambiguates? "[d] " : "")
                 << toString(L) << ": " << msg << instLoc << endl;
  bool report = (eflags & EF_DISAMBIGUATES) || (eflags & EF_STRONG) || (!disambiguateOnly);
  if (report) {
    errors.addError
      (new ErrorMsg
       (L, msg, disambiguates ? EF_DISAMBIGUATES : EF_NONE, instLoc));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}

Type *Env::error(char const *msg, ErrorFlags eflags)
{
  return error(loc(), msg, eflags);
}


Type *Env::warning(char const *msg)
{
  string instLoc = instLocStackString();
  trace("error") << "warning: " << msg << instLoc << endl;
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
  // this doesn't use 'implicitReceiverType' because of the warnings above
  Type *recType = tfac.makeTypeOf_receiver(loc, nat, cv, syntax);
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
bool Env::almostEqualTypes(Type const *t1, Type const *t2)
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
  Type::EqFlags eqFlags = Type::EF_IGNORE_PARAM_CV;

  if (lang.allow_KR_ParamOmit) {
    eqFlags |= Type::EF_ALLOW_KR_PARAM_OMIT;
  }

  return t1->equals(t2, eqFlags);
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
                                       scope, enclosingClass, prior, overloadSet,
                                       true /*reallyAddVariable*/);
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
  OverloadSet *overloadSet, // set into which to insert it, if that's what to do
  // should we really add the variable to the scope?  if false, don't
  // register the variable anywhere, which is a situation that occurs
  // in template instantiation
  bool reallyAddVariable
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
    //
    // this is only a problem if the prior is supposed to exist in a
    // scope somewhere which was passed down to us, which is not the
    // case if !reallyAddVariable, where it might exist only in the
    // instantiation list of some template primary; FIX: this isn't
    // quite so elegant
    if (reallyAddVariable) {
      if (!sameScopes(prior->skipAlias()->scope, scope)) {
        error(type, stringc
              << "prior declaration of `" << name
              << "' at " << prior->loc
              << " refers to a different entity, so it conflicts with "
              << "the one being declared here");
        goto makeDummyVar;
      }
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

  if (reallyAddVariable) {
    if (overloadSet) {
      // don't add it to the environment (another overloaded version
      // is already in the environment), instead add it to the overload set
      //
      // dsw: don't add it if it is a template specialization
      if (!(dflags & DF_TEMPL_SPEC)) {
        overloadSet->addMember(newVar);
        newVar->overload = overloadSet;
      }
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


// ------------------------ elaboration -------------------------
PQName *Env::make_PQ_fullyQualifiedName(Scope *s, PQName *name0)
{
  name0 = make_PQ_qualifiedName(s, name0);

  // now find additional qualifiers needed to name 's' itself
  if (s->parentScope) {
    return make_PQ_fullyQualifiedName(s->parentScope, name0);
  }
  else if (s->scopeKind == SK_GLOBAL) {
    // sm: 12/19/03: The intent of this code was to notice when the
    // scope 's' appeared in the global scope, meaning it should be
    // qualified with "::".  Therefore the test that makes sense to me
    // is
    //
    //    s->getTypedefName()->scopeKind == SK_GLOBAL
    //
    // However, Daniel reports that this breaks things.  I suspect
    // that is due to the failure to handle "::" correctly in general.
    // Since, for the moment, things appear to work correctly with the
    // test as it is above, I'm going to leave it.
    //
    // But as further evidence that my understanding is at least
    // partially correct, notice that indeed this is never reached
    // with the code the way it is.
    xfailure("this is not reached");

    // prepend what syntactically would be a leading "::"
    return new PQ_qualifier(loc(), NULL /*qualifier*/,
                            new ASTList<TemplateArgument>,
                            name0);
  }
  else {
    // no further qualification is possible, so we cross our fingers
    // and hope it isn't necessary either
    return name0;
  }
}


// prepend to 'name0' information about 's'
PQName *Env::make_PQ_qualifiedName(Scope *s, PQName *name0)
{
  // should only be called for named scopes
  Variable *typedefVar = s->getTypedefName();
  xassert(typedefVar);

  // construct the list of template arguments; we must rebuild them
  // instead of using templateInfo->arguments directly, because the
  // TS_names that are used in the latter might not be in scope here
  ASTList<TemplateArgument> *targs = make_PQ_templateArgs(s);

  // now build a PQName
  if (name0) {
    // cons a qualifier on to the front
    name0 = new PQ_qualifier(loc(), typedefVar->name, targs, name0);
  }
  else {
    name0 = make_PQ_possiblyTemplatizedName(loc(), typedefVar->name, targs);
  }

  return name0;
}


PQName *Env::make_PQ_possiblyTemplatizedName
  (SourceLoc loc, StringRef name, ASTList<TemplateArgument> *targs)
{
  // dsw: dang it Scott, why the templatization asymmetry between
  // PQ_name/template on one hand and PQ_qualifier on the other?
  //
  // sm: to save space in the common PQ_name case
  if (targs) {
    return new PQ_template(loc, name, targs);
  }
  else {
    return new PQ_name(loc, name);
  }
}

// construct the list of template arguments
ASTList<TemplateArgument> *Env::make_PQ_templateArgs(Scope *s)
{
  ASTList<TemplateArgument> *targs = new ASTList<TemplateArgument>;
  if (s->curCompound && s->curCompound->templateInfo()) {
    FOREACH_OBJLIST(STemplateArgument, 
                    s->curCompound->templateInfo()->arguments, iter) {
      STemplateArgument const *sarg = iter.data();
      if (sarg->kind == STemplateArgument::STA_TYPE) {
        // pull out the Type, then build a new ASTTypeId for it
        targs->append(new TA_type(buildASTTypeId(sarg->getType())));
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
        targs->append(new TA_nontype(e));
      }
    }
  }
  return targs;
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
//
//    int      *      *   (null)
//
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
    default: xfailure("bad tag");

    case Type::T_POINTER:
    case Type::T_REFERENCE: {
      return inner_buildASTTypeId(type->getAtType(),
        new D_pointer(loc(), type->isPointerType() /*isPtr*/,
                      type->getCVFlags(), surrounding));
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
        new D_ptrToMember(loc(), make_PQ_fullyQualifiedName(ptm->inClass()),
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
  FOREACH_SASTLIST_NC(Variable, type->typedefAliases, iter) {
    Variable *alias = iter.data();

    TRACE("buildTypedefSpecifier",
      "`" << type->toString() << "': trying " << alias->name);

    // first, the final component of the name
    PQName *name = new PQ_name(loc(), alias->name);

    // then the qualifier prefix, if any
    if (alias->scope) {
      // the code used pass 'alias->scope->curCompound', which I think is
      // wrong, since all it does is cause a segfault in the case that
      // 'alias->scope' is a namespace and not a class
      name = make_PQ_fullyQualifiedName(alias->scope, name);
    }

    // did that work?  (this check uses 'equals' because of the possibility
    // of non-trivial 'clone' operations in the type factory; in the default
    // Elsa configuration, '==' would suffice)
    Variable *found = lookupPQVariable(name);
    if (found && found->type->equals(type)) {
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


E_intLit *Env::buildIntegerLiteralExp(int i)
{
  StringRef text = str(stringc << i);
  E_intLit *ret = new E_intLit(text);
  ret->i = i;
  return ret;
}


// EOF
