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
#include "cc.ast.gen.h"   // C++ ast components
#include "variable.h"     // Variable (r)
#include "cc_scope.h"     // Scope
#include "cc_err.h"       // ErrorMsg

class StringTable;        // strtable.h
class CCLang;             // cc_lang.h

// the entire semantic analysis state
class Env {
private:     // data
  // stack of lexical scopes; first is innermost
  // NOTE: if a scope has curCompound!=NULL, then this list does *not* own
  // it.  otherwise it does own it.
  ObjList<Scope> scopes;

  // list of named scopes (i.e. namespaces)
  //StringObjDict<Scope> namespaces;    // not implemented yet

  // when true, all errors are ignored (dropped on floor) except:
  //   - errors with the 'disambiguates' flag set
  //   - unimplemented functionality
  // this is used when processing bodies of template classes and
  // functions, where we don't know anything about the type
  // parameters
  bool disambiguateOnly;

  // counter for constructing names for anonymous types
  int anonTypeCounter;

public:      // data
  // stack of error messages; the first one is the latest
  // one inserted; during disambiguation, I'll remember where
  // the top was before each alternative, so I can leave this
  // stack with only the ones from one particular interpretation
  ObjList<ErrorMsg> errors;

  // string table for making new strings
  StringTable &str;

  // language options in effect
  CCLang &lang;

  // interface for making types
  TypeFactory &tfac;

  // client analyses may need to get ahold of all the Variables that I
  // made up, so this is a list of them; these don't include Variables
  // built for parameters of function types, but the functions
  // themselves appear here so the parameters are reachable
  SObjList<Variable> madeUpVariables;

  // type for typeid expression
  Type *type_info_const_ref;      // (serf)

  // special names
  StringRef conversionOperatorName;
  StringRef constructorSpecialName;
  StringRef functionOperatorName;

  // special variables associated with particular types
  Variable *dependentTypeVar;           // (serf)
  Variable *errorVar;                   // (serf)

private:     // funcs
  CompoundType *instantiateClass(
    CompoundType *tclass, FakeList<TemplateArgument> *args);

  void declareFunction1arg(Type *retType, char const *funcName,
                           Type *arg1Type, char const *arg1Name,
                           Type *exnType);

public:      // funcs
  Env(StringTable &str, CCLang &lang, TypeFactory &tfac);
  ~Env();

  int getChangeCount() const { return scopeC()->getChangeCount(); }

  // scopes
  Scope *enterScope();            // returns new Scope
  void exitScope(Scope *s);       // paired with enterScope()
  void extendScope(Scope *s);     // push onto stack, but don't own
  void retractScope(Scope *s);    // paired with extendScope()

  // the current, innermost scope
  Scope *scope() { return scopes.first(); }
  Scope const *scopeC() const { return scopes.firstC(); }

  // innermost scope that can accept names
  Scope *acceptingScope();

  // innermost non-class, non-template, non-function-prototype scope
  Scope *outerScope();

  // innermost scope that can accept names, *other* than
  // the one we're in now
  Scope *enclosingScope();

  // source location tracking
  void setLoc(SourceLocation const &loc);    // sets scope()->curLoc
  SourceLocation const &loc() const;         // gets scope()->curLoc
  string locStr() const { return loc().toString(); }

  // insertion into the current scope; return false if the
  // name collides with one that is already there (but if
  // 'forceReplace' true, silently replace instead)
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

  // like 'addVariable' in that the 'scope' field gets set, but
  // nothing is added to the maps
  void registerVariable(Variable *v);

  // lookup in the environment (all scopes)
  Variable *lookupPQVariable(PQName const *name, LookupFlags f=LF_NONE);
  Variable *lookupVariable  (StringRef name,     LookupFlags f=LF_NONE);

  CompoundType *lookupPQCompound(PQName const *name, LookupFlags f=LF_NONE);
  CompoundType *lookupCompound  (StringRef name,     LookupFlags f=LF_NONE);

  EnumType *lookupPQEnum(PQName const *name, LookupFlags f=LF_NONE);
  EnumType *lookupEnum  (StringRef name,     LookupFlags f=LF_NONE);

  // look up a particular scope; the 'name' part of the PQName
  // will be ignored; if we can't find this scope, return NULL
  // *and* report it as an error; there must be at least one
  // qualified in 'name'; 'dependent' is set to true if the lookup
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
  ClassTemplateInfo * /*owner*/ takeTemplateClassInfo();

  // return a new name for an anonymous type; 'keyword' says
  // which kind of type we're naming
  StringRef getAnonName(TypeIntr keyword);

  // introduce a new compound type name; return the constructed
  // CompoundType's pointer in 'ct', after inserting it into 'scope'
  Type *makeNewCompound(CompoundType *&ct, Scope *scope,
                        StringRef name, SourceLocation const &loc,
                        TypeIntr keyword, bool forward);

  // diagnostic reports; all return ST_ERROR type
  Type *error(char const *msg, bool disambiguates=false);
  Type *warning(char const *msg);
  Type *unimp(char const *msg);

  // diagnostics involving type clashes; will be suppressed
  // if the type is ST_ERROR
  Type *error(Type *t, char const *msg);

  // set 'disambiguateOnly' to 'val', returning prior value
  bool setDisambiguateOnly(bool val);
  bool onlyDisambiguating() const { return disambiguateOnly; }
  
  // TypeFactory funcs; all of these simply delegate to 'tfac'
  CVAtomicType *makeCVAtomicType(AtomicType *atomic, CVFlags cv)
    { return tfac.makeCVAtomicType(atomic, cv); }
  PointerType *makePointerType(PtrOper op, CVFlags cv, Type *atType)
    { return tfac.makePointerType(op, cv, atType); }
  FunctionType *makeFunctionType(Type *retType, CVFlags cv)
    { return tfac.makeFunctionType(retType, cv); }
  ArrayType *makeArrayType(Type *eltType, int size = -1)
    { return tfac.makeArrayType(eltType, size); }

  Variable *makeVariable(SourceLocation const &L, StringRef n,
                         Type *t, DeclFlags f)
    { return tfac.makeVariable(L, n, t, f); }

  CVAtomicType *getSimpleType(SimpleTypeId st, CVFlags cv = CV_NONE)
    { return tfac.getSimpleType(st, cv); }
  CVAtomicType *makeType(AtomicType *atomic)
    { return tfac.makeType(atomic); }
  Type *makePtrType(Type *type)
    { return tfac.makePtrType(type); }

  // others are more obscure, so I'll just call into 'tfac' directly
  // in the places I call them
};


#endif // CC_ENV_H
