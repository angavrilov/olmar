// cc_env.h            see license.txt for copyright and terms of use
// Env class, which is the compile-time C++ environment

#ifndef CC_ENV_H
#define CC_ENV_H

#include "cc_type.h"      // Type, AtomicType, etc. (r)
#include "strobjdict.h"   // StrObjDict
#include "strsobjdict.h"  // StrSObjDict
#include "owner.h"        // Owner
#include "exc.h"          // xBase
#include "sobjlist.h"     // SObjList
#include "objstack.h"     // ObjStack
#include "sobjstack.h"    // SObjStack
#include "cc_ast.h"       // C++ ast components
#include "variable.h"     // Variable (r)
#include "cc_scope.h"     // Scope
#include "cc_err.h"       // ErrorList
#include "array.h"        // ArrayStack
#include "builtinops.h"   // CandidateSet

class StringTable;        // strtable.h
class CCLang;             // cc_lang.h

// counter for generating unique throw clause names; NOTE: FIX: they
// must not be duplicated across translation units since they are
// global!  Thus, this must survive even multiple Env objects.
// (this is for elaboration)
extern int throwClauseSerialNumber;

// the entire semantic analysis state
class Env {
protected:   // data
  // stack of lexical scopes; first is innermost
  //
  // NOTE: If a scope has a non-NULL curCompound or namespaceVar,
  // then this list does *not* own it.  Otherwise it does own it.
  ObjList<Scope> scopes;

  // when true, all errors are ignored (dropped on floor) except:
  //   - errors with the 'disambiguates' flag set
  //   - unimplemented functionality
  // this is used when processing bodies of template classes and
  // functions, where we don't know anything about the type
  // parameters
  bool disambiguateOnly;

  // counter for constructing names for anonymous types
  int anonTypeCounter;

  // initially false, this becomes true once Env::Env has finished;
  // this is used to distinguish entities introduced automatically
  // at the start from those that appeared in the input file
  bool ctorFinished;

public:      // data
  // nesting level of disambiguation passes; 0 means not disambiguating;
  // this is used for certain kinds of error reporting and suppression
  int disambiguationNestingLevel;

  // list of error messages; during disambiguation, the existing
  // list is set aside, so 'errors' only has errors from the
  // disambiguation we're doing now (if any)
  ErrorList errors;

  // string table for making new strings
  StringTable &str;

  // language options in effect
  CCLang &lang;

  // interface for making types
  TypeFactory &tfac;

  // client analyses may need to get ahold of all the Variables that I
  // made up, so this is a list of them; these don't include Variables
  // built for parameters of function types, but the functions
  // themselves appear here so the parameters are reachable (NOTE:
  // at the moment, I don't think anyone is using this information)
  ArrayStack<Variable*> madeUpVariables;

  // type for typeid expression
  Type *type_info_const_ref;      // (serf)

  // special names
  StringRef conversionOperatorName;
  StringRef constructorSpecialName;
  StringRef functionOperatorName;
  StringRef thisName;
  StringRef quote_C_quote;
  StringRef quote_C_plus_plus_quote;

  StringRef special_getStandardConversion;
  StringRef special_getImplicitConversion;
  StringRef special_testOverload;
  StringRef special_computeLUB;

  // special variables associated with particular types
  Variable *dependentTypeVar;           // (serf)
  Variable *dependentVar;               // (serf)
  Variable *errorTypeVar;               // (serf)
  Variable *errorVar;                   // (serf)

  // dsw: Can't think of a better way to do this, sorry.
  Variable *var__builtin_constant_p;

  // operator function names, indexed by the operators they overload
  StringRef operatorName[NUM_OVERLOADABLE_OPS];

  // built-in operator function sets, indexed by operator
  ArrayStack<Variable*> builtinUnaryOperator[NUM_OVERLOADABLE_OPS];
  ObjArrayStack<CandidateSet> builtinBinaryOperator[NUM_OVERLOADABLE_OPS];

  // TODO: elminate this!
  TranslationUnit *tunit;
                
  // current linkage type, as a string literal like "C" or "C++"
  StringRef currentLinkage;

  // when true, the type checker does overload resolution; this isn't
  // enabled by default because it's not fully implemented, and
  // consequently, turning it on leads to spurious errors on some test
  // cases (TODO: eventually, enable this permanently)
  bool doOverload;
  
  // when true, operator expressions are checked to see if they
  // are to be treated as calls to overloaded operator functions
  bool doOperatorOverload;

  // when non-NULL, the variable lookup results are collected and
  // compared to the text stored in this pointer; it is supplied via
  // an an 'asm' directive (see TF_asm::itcheck)
  StringRef collectLookupResults;

  // ------------------- for elaboration ------------------
  // when true, do elaboration
  bool doElaboration;

  // so that we can find the closest nesting S_compound for when we
  // need to insert temporary variables; its scope should always be
  // the current scope.
  SObjStack<FullExpressionAnnot> fullExpressionAnnotStack;

  // counters for generating unique temporary names; not unique
  // across translation units
  int tempSerialNumber;
  int e_newSerialNumber;

private:     // funcs
  // old
  //CompoundType *instantiateClass(
  //  CompoundType *tclass, FakeList<TemplateArgument> *args);

  // these are intended to help build the initialization-time stuff,
  // not build functions that result from the user's input syntax
  Variable *declareFunctionNargs(
    Type *retType, char const *funcName,
    Type **argTypes, char const **argNames, int numArgs,
    FunctionFlags flags, 
    Type * /*nullable*/ exnType);

  Variable *declareFunction0arg(Type *retType, char const *funcName,
                                FunctionFlags flags = FF_NONE,
                                Type * /*nullable*/ exnType = NULL);

  Variable *declareFunction1arg(Type *retType, char const *funcName,
                                Type *arg1Type, char const *arg1Name,
                                FunctionFlags flags = FF_NONE,
                                Type * /*nullable*/ exnType = NULL);

  Variable *declareFunction2arg(Type *retType, char const *funcName,
                                Type *arg1Type, char const *arg1Name,
                                Type *arg2Type, char const *arg2Name,
                                FunctionFlags flags = FF_NONE,
                                Type * /*nullable*/ exnType = NULL);

  Variable *declareSpecialFunction(char const *name);

  CompoundType *findEnclosingTemplateCalled(StringRef name);

  void setupOperatorOverloading();

  void addBuiltinUnaryOp(OverloadableOp op, Type *x);

  void addBuiltinBinaryOp(OverloadableOp op, Type *x, Type *y);
  void addBuiltinBinaryOp(OverloadableOp op, PredicateCandidateSet::PreFilter pre,
                                             PredicateCandidateSet::PostFilter post,
                                             bool isAssignment = false);
  void addBuiltinBinaryOp(OverloadableOp op, CandidateSet * /*owner*/ cset);

public:      // funcs
  Env(StringTable &str, CCLang &lang, TypeFactory &tfac, TranslationUnit *tunit0);
  virtual ~Env();      // 'virtual' only to silence stupid warning; destruction is not part of polymorphic contract

  int getChangeCount() const { return scopeC()->getChangeCount(); }

  // scopes
  Scope *enterScope(ScopeKind sk, char const *forWhat);   // returns new Scope
  void exitScope(Scope *s);       // paired with enterScope()
  void extendScope(Scope *s);     // push onto stack, but don't own
  void retractScope(Scope *s);    // paired with extendScope()

  // the current, innermost scope
  Scope *scope() { return scopes.first(); }
  Scope const *scopeC() const { return scopes.firstC(); }

  // innermost scope that can accept names; the decl flags might
  // be used to choose exactly which scope to use
  Scope *acceptingScope(DeclFlags df = DF_NONE);
  Scope *typeAcceptingScope() { return acceptingScope(DF_TYPEDEF); }

  // innermost non-class, non-template, non-function-prototype scope
  Scope *outerScope();

  // innermost scope that can accept names, *other* than
  // the one we're in now
  Scope *enclosingScope();

  // return the nearest enclosing scope of kind 'k', or NULL if there
  // is none with that kind
  Scope *enclosingKindScope(ScopeKind k);
  Scope *globalScope() { return enclosingKindScope(SK_GLOBAL); }

  // essentially: enclosingKindScope(SK_CLASS)->curCompound;
  CompoundType *enclosingClassScope();

  bool inTemplate()
    { return !!enclosingKindScope(SK_TEMPLATE); }

  // true if the current scope contains 's' as a nested scope
  bool currentScopeEncloses(Scope const *s);
  
  // return the innermost scope that contains both the current
  // scope and 'target'
  Scope *findEnclosingScope(Scope *target);

  // source location tracking
  void setLoc(SourceLoc loc);                // sets scope()->curLoc
  SourceLoc loc() const;                     // gets scope()->curLoc
  string locStr() const { return toString(loc()); }

  // insertion into the current scope; return false if the
  // name collides with one that is already there (but if
  // 'forceReplace' true, silently replace instead); if you
  // want to insert into a specific scope, use Scope::addVariable
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

  // like 'addVariable' in that the 'scope' field gets set, but
  // nothing is added to the maps
  void registerVariable(Variable *v);

  // like 'addVariable', but if 'prevLookup' is not NULL then 'v' gets
  // added to prevLookup's overload set instead of the current scope;
  // it is *not* the case that all overloaded variables are added
  // using this interface
  void addVariableWithOload(Variable *prevLookup, Variable *v);

  // lookup in the environment (all scopes); return NULL if can't find
  Variable *lookupPQVariable(PQName const *name, LookupFlags f=LF_NONE);
  Variable *lookupVariable  (StringRef name,     LookupFlags f=LF_NONE);
  
  // this variant returns the Scope in which the name was found
  Variable *lookupVariable(StringRef name, LookupFlags f, Scope *&scope);

  CompoundType *lookupPQCompound(PQName const *name, LookupFlags f=LF_NONE);
  CompoundType *lookupCompound  (StringRef name,     LookupFlags f=LF_NONE);

  EnumType *lookupPQEnum(PQName const *name, LookupFlags f=LF_NONE);
  EnumType *lookupEnum  (StringRef name,     LookupFlags f=LF_NONE);

  // look up a particular scope; the 'name' part of the PQName
  // will be ignored; if we can't find this scope, return NULL
  // *and* report it as an error; there must be at least one
  // qualifier in 'name'; 'dependent' is set to true if the lookup
  // failed because we tried to look into a template parameter;
  // 'anyTemplates' is set to true if any of the scopes named a
  // template type
  Scope *lookupQualifiedScope(PQName const *name,
    bool &dependent, bool &anyTemplates);
  Scope *lookupQualifiedScope(PQName const *name);

  // if the innermost scope has some template parameters, take
  // them out and return them; otherwise return NULL
  TemplateParams * /*owner*/ takeTemplateParams();
  
  // like the above, but wrap it in a ClassTemplateInfo
  ClassTemplateInfo * /*owner*/ takeTemplateClassInfo(StringRef baseName);

  // return a new name for an anonymous type; 'keyword' says
  // which kind of type we're naming
  StringRef getAnonName(TypeIntr keyword);

  // introduce a new compound type name; return the constructed
  // CompoundType's pointer in 'ct', after inserting it into 'scope'
  // (if that is not NULL)
  Type *makeNewCompound(CompoundType *&ct, Scope * /*nullable*/ scope,
                        StringRef name, SourceLoc loc,
                        TypeIntr keyword, bool forward);

  // instantate 'base' with 'arguments', and return the implicit
  // typedef Variable associated with the resulting type; 'scope'
  // is the scope in which 'base' was found; if 'inst' is not
  // NULL then we already have a compound for this instantiation
  // (from a forward declaration), so use that one
  Variable *instantiateClassTemplate(
    Scope *scope, CompoundType *base,
    FakeList<TemplateArgument> *arguments, CompoundType *inst = NULL);

  // given a template class that was just made non-forward,
  // instantiate all of its forward-declared instances
  void instantiateForwardClasses(Scope *scope, CompoundType *base);

  // diagnostic reports; all return ST_ERROR type
  Type *error(SourceLoc L, char const *msg, bool disambiguates=false);
  Type *error(char const *msg, bool disambiguates=false);
  Type *warning(char const *msg);
  Type *unimp(char const *msg);

  // diagnostics involving type clashes; will be suppressed
  // if the type is ST_ERROR
  Type *error(Type *t, char const *msg);

  // set 'disambiguateOnly' to 'val', returning prior value
  bool setDisambiguateOnly(bool val);
  bool onlyDisambiguating() const { return disambiguateOnly; }

  // return true if the given list of errors contain any which
  // are disambiguating errors
  bool hasDisambErrors() const { return errors.hasDisambErrors(); }
  
  // return true if environment modifications should be suppressed
  // because of disambiguating errors
  bool disambErrorsSuppressChanges() const
    { return disambiguationNestingLevel>0 && hasDisambErrors(); }

  // number of errors; intended to be called after type checking,
  // to see how many errors (if any) resulted
  int numErrors() const { return errors.count(); }

  // makes a function type that returns ST_CDTOR and accepts no params
  FunctionType *makeDestructorFunctionType(SourceLoc loc);

  // similar to above, except its flagged with FF_CTOR, and the caller
  // must call 'doneParams' when it has finished adding parameters
  FunctionType *beginConstructorFunctionType(SourceLoc loc);

  // TypeFactory funcs; all of these simply delegate to 'tfac'
  CVAtomicType *makeCVAtomicType(SourceLoc loc, AtomicType *atomic, CVFlags cv)
    { return tfac.makeCVAtomicType(loc, atomic, cv); }
  PointerType *makePointerType(SourceLoc loc, PtrOper op, CVFlags cv, Type *atType)
    { return tfac.makePointerType(loc, op, cv, atType); }
  FunctionType *makeFunctionType(SourceLoc loc, Type *retType)
    { return tfac.makeFunctionType(loc, retType); }
  ArrayType *makeArrayType(SourceLoc loc, Type *eltType, int size = -1)
    { return tfac.makeArrayType(loc, eltType, size); }

  // slight oddball since no 'loc' passed..
  Type *makeRefType(Type *underlying)
    { return tfac.makeRefType(loc(), underlying); }

  // (this does the work of the old 'makeMadeUpVariable')
  Variable *makeVariable(SourceLoc L, StringRef n, Type *t, DeclFlags f);

  CVAtomicType *getSimpleType(SourceLoc loc, SimpleTypeId st, CVFlags cv = CV_NONE)
    { return tfac.getSimpleType(loc, st, cv); }
  CVAtomicType *makeType(SourceLoc loc, AtomicType *atomic)
    { return tfac.makeType(loc, atomic); }
  Type *makePtrType(SourceLoc loc, Type *type)
    { return tfac.makePtrType(loc, type); }

  // if in a context where an implicit receiver object is available,
  // return its type; otherwise return NULL
  Type *implicitReceiverType();

  // others are more obscure, so I'll just call into 'tfac' directly
  // in the places I call them
                                                              
  // create a built-in candidate for operator overload resolution
  Variable *createBuiltinUnaryOp(OverloadableOp op, Type *x);
  Variable *createBuiltinBinaryOp(OverloadableOp op, Type *x, Type *y);

  // several steps of the declaration creation process, broken apart
  // to aid sharing among D_name_tcheck and makeUsingAliasFor; best
  // to look at their implementations and the comments therein
  Variable *lookupVariableForDeclaration
    (Scope *scope, StringRef name, Type *type, CVFlags this_cv);
  OverloadSet *getOverloadForDeclaration(Variable *&prior, Type *type);
  Variable *createDeclaration(
    SourceLoc loc,
    StringRef name,
    Type *type,
    DeclFlags dflags,
    Scope *scope,
    CompoundType *enclosingClass,
    Variable *prior,
    OverloadSet *overloadSet
  );

  // create a "using declaration" alias
  void makeUsingAliasFor(SourceLoc loc, Variable *origVar);
                                    
  // pass Variable* through this before storing in the AST, so
  // that the AST only has de-aliased pointers (if desired)
  Variable *storeVar(Variable *var);

  // this version will do pass-through if 'var' or the thing to which
  // it is aliased has an overload set; it's for those cases where
  // subsequent overload resolution needs to pick from the set, before
  // de-aliasing happens
  Variable *storeVarIfNotOvl(Variable *var);

  // true if the current linkage type is "C"
  bool linkageIs_C() const { return currentLinkage == quote_C_quote; }

  // points of extension: These functions do nothing in the base
  // Elsa parser, but can be overridden in client analyses to
  // hook into the type checking process.  See their call sites in
  // cc_tcheck.cc for more info on when they're called.

  virtual void checkFuncAnnotations(FunctionType *ft, D_func *syntax);

  // this is called after all the fields of 'ct' have been set, and
  // we've popped its scope off the stack
  virtual void addedNewCompound(CompoundType *ct);

  // return # of array elements initialized
  virtual int countInitializers(SourceLoc loc, Type *type, IN_compound const *cpd);

  // called when a variable is successfully added; note that there
  // is a similar mechanism in Scope itself, which can be used when
  // less context is necessary
  virtual void addedNewVariable(Scope *s, Variable *v);

  // -------------- stuff for elaboration support ---------------
  // change 'tv' into a shadow typedef var
  void makeShadowTypedef(Scope *scope, Variable *tv);
  
  // true if 'tv' is a shadow typedef made by the above function
  bool isShadowTypedef(Variable *tv);

  // make a unique name for a new temporary variable
  virtual PQ_name *makeTempName();
  // make a unique name for a new E_new variable
  virtual StringRef makeE_newVarName();
  // make a unique name for a new throw clause variable
  virtual StringRef makeThrowClauseVarName();

  // return a PQName that will typecheck in the current environment to
  // find (the typedef for) the 'name0' member of the 's' scope, or
  // 's' itself if 'name0 is NULL; the return value is maximally
  // qualified
  PQName *make_PQ_fullyQualifiedName(Scope *s, PQName *name0 = NULL);
        
  // go from "A" to "A::~A"
  PQName *make_PQ_fullyQualifiedDtorName(CompoundType *ct);

  // given a type, return an ASTTypeId AST node that denotes that
  // type in the current environment
  ASTTypeId *buildASTTypeId(Type *type);
  ASTTypeId *inner_buildASTTypeId     // effectively private to buildASTTypeId
    (Type *type, IDeclarator *surrounding);
                                                   
  // look among the typedef aliases of 'type' for one that maps to
  // 'type' in the current environment, and wrap that name in a
  // TS_name; or, return NULL if it has no such aliases
  TS_name *buildTypedefSpecifier(Type *type);
                                            
  // make an AST node for an integer literal expression
  Expression *buildIntegerLiteralExp(int i);
};


// set/reset 'disambiguateOnly'
class DisambiguateOnlyTemp {
private:
  Env &env;       // relevant environment
  bool prev;      // previous value of 'disambiguateOnly'
  bool active;    // when false, we do nothing

public:
  DisambiguateOnlyTemp(Env &e, bool disOnly)
    : env(e),
      active(disOnly) {
    if (active) {
      prev = e.setDisambiguateOnly(true);
    }
  }

  ~DisambiguateOnlyTemp() {
    if (active) {
      env.setDisambiguateOnly(prev);
    }
  }
};


// little hack to suppress errors in a given segment
class SuppressErrors {
private:
  Env &env;               // relevant environment
  ErrorList existing;     // errors before the operation

public:
  SuppressErrors(Env &e) : env(e) {
    // squirrel away the good messages
    existing.takeMessages(env.errors);
  }

  ~SuppressErrors() {
    // get rid of any messages added in the meantime
    env.errors.deleteAll();

    // put back the good ones
    env.errors.takeMessages(existing);
  }
};


#endif // CC_ENV_H
