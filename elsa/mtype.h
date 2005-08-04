// mtype.h
// new implementation of type matcher

// 2005-07-24: The plan for this module is it will eventually replace
// the 'matchtype' module, and also Type::equals.  However, right now
// it is still being put through its paces and so just exists on the
// side as an auxiliary module, only reachable via the __test_mtype
// internal testing hook function, exercised for the moment only by
// in/t0511.cc.

#ifndef MTYPE_H
#define MTYPE_H

#include "mflags.h"             // MatchFlags
#include "objmap.h"             // ObjMap
#include "cc_type.h"            // Type
#include "cc_ast.h"             // C++ AST
#include "template.h"           // STemplateArgument


class MType {
private:      // types
  // A template variable binding.  A name can be bound to one of three
  // things:
  //   - a Type object and CVFlags to modify it
  //   - an AtomicType object (with no cv)
  //   - a non-type value given by 'sarg'
  // The first two options are similar, differing only because it avoids
  // having to wrap a Type object around an AtomicType object.
  //
  // Public clients don't have to know about this trickery though; as
  // far as they are concerned, a name is simply bound to an
  // STemplateArgument (though they have to pass a TypeFactory to
  // allow the internal representation to be converted).
  class Binding {
  public:     // data
    STemplateArgument sarg;     // most of the binding info
    CVFlags cv;                 // if sarg.isType(), we are bound to the 'cv' version of it

  public:
    Binding() : sarg(), cv(CV_NONE) {}
    
    // Though I am using 'STemplateArgument', I want to treat its
    // 'Type*' as being const.
    void setType(Type const *t) { sarg.setType(const_cast<Type*>(t)); }
    Type const *getType() const { return sarg.getType(); }
                            
    // debugging
    string asString() const;
  };

private:      // data
  // set of bindings
  typedef ObjMap<char const /*StringRef*/, Binding> BindingMap;
  BindingMap bindings;
  
  // true if the client is using the non-const interface
  bool nonConst;

private:      // funcs
  bool imatch(Type const *conc, Type const *pat, MatchFlags flags);
  static bool canUseAsVariable(Variable *var, MatchFlags flags);
  bool matchAtomicType(AtomicType const *conc, AtomicType const *pat, MatchFlags flags);
  bool matchTypeVariable(TypeVariable const *conc, TypeVariable const *pat, MatchFlags flags);
  bool matchPseudoInstantiation(PseudoInstantiation const *conc,
                                       PseudoInstantiation const *pat, MatchFlags flags);
public://maybe temporary
  bool matchSTemplateArguments(ObjList<STemplateArgument> const &conc,
                               ObjList<STemplateArgument> const &pat,
                               MatchFlags flags);
private://maybe temporary
  bool matchSTemplateArgument(STemplateArgument const *conc,
                                     STemplateArgument const *pat, MatchFlags flags);
  bool matchNontypeWithVariable(STemplateArgument const *conc,
                                       E_variable *pat, MatchFlags flags);
public://temporary
  bool matchDependentQType(DependentQType const *conc,
                                  DependentQType const *pat, MatchFlags flags);
private://temporary
  bool matchPQName(PQName const *conc, PQName const *pat, MatchFlags flags);
  bool matchType(Type const *conc, Type const *pat, MatchFlags flags);
  bool matchTypeWithVariable(Type const *conc, TypeVariable const *pat,
                                    CVFlags tvCV, MatchFlags flags);
  bool equalWithAppliedCV(Type const *conc, Binding *binding, CVFlags cv, MatchFlags flags);
  bool matchTypeWithSpecifiedCV(Type const *conc, Type const *pat, CVFlags cv, MatchFlags flags);
  bool addTypeBindingWithoutCV(StringRef tvName, Type const *conc, 
                               CVFlags tvcv, MatchFlags flags);
  bool matchTypeWithPolymorphic(Type const *conc, SimpleTypeId polyId, MatchFlags flags);
  bool matchAtomicTypeWithVariable(AtomicType const *conc,
                                   TypeVariable const *pat,
                                   MatchFlags flags);
  bool matchCVAtomicType(CVAtomicType const *conc, CVAtomicType const *pat, MatchFlags flags);
  bool matchCVFlags(CVFlags conc, CVFlags pat, MatchFlags flags);
  bool matchPointerType(PointerType const *conc, PointerType const *pat, MatchFlags flags);
  bool matchReferenceType(ReferenceType const *conc, ReferenceType const *pat, MatchFlags flags);
  bool matchFunctionType(FunctionType const *conc, FunctionType const *pat, MatchFlags flags);
  bool matchParameterLists(FunctionType const *conc, FunctionType const *pat,
                                  MatchFlags flags);
  bool matchExceptionSpecs(FunctionType const *conc, FunctionType const *pat, MatchFlags flags);
  bool matchArrayType(ArrayType const *conc, ArrayType const *pat, MatchFlags flags);
  bool matchPointerToMemberType(PointerToMemberType const *conc,
                                       PointerToMemberType const *pat, MatchFlags flags);
  bool matchExpression(Expression const *conc, Expression const *pat, MatchFlags flags);
  
  // stuff for DQT resolution
  Type *lookupPQInScope(Scope const *scope, PQName const *name);
  Variable *lookupNameInScope(Scope const *scope0, StringRef name);
  Variable *applyTemplateArgs(Variable *primary,
                              ObjList<STemplateArgument> const &sargs);
  Variable *searchForInstantiation(TemplateInfo *ti,
                                   ObjList<STemplateArgument> const &sargs);

public:       // funcs
  // only when 'allowNonConst' is true can 'matchNC' be invoked
  MType(bool allowNonConst = false);
  ~MType();

  // return true if 'conc' is an instance of 'pat', in which case this
  // object will have a record of the instantiation bindings; the
  // const version can only be called when 'nonConst' is false
  bool match(Type const *conc, Type const *pat, MatchFlags flags);

  // non-const version
  bool matchNC(Type *conc, Type *pat, MatchFlags flags);

  // how many bindings are currently active?
  int getNumBindings() const;
  
  // get the binding for 'name', or return STA_NONE if none
  STemplateArgument getBoundValue(StringRef name, TypeFactory &tfac);
  
  // set a binding; 'value' must not be STA_NONE
  void setBoundValue(StringRef name, STemplateArgument const &value);
};


#endif // MTYPE_H
