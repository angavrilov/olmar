// cc_type.cc            see license.txt for copyright and terms of use
// code for cc_type.h

#include "cc_type.h"    // this module
#include "trace.h"      // tracingSys
#include "variable.h"   // Variable
#include "strutil.h"    // copyToStaticBuffer
#include "sobjset.h"    // SObjSet
#include "hashtbl.h"    // lcprngTwoSteps
#include "matchtype.h"  // MatchTypes

// Do *not* add a dependency on cc.ast or cc_env.  cc_type is about
// the intrinsic properties of types, independent of any particular
// syntax for denoting them.  (toString() uses C's syntax, but that's
// just for debugging.)

#include <stdlib.h>     // getenv


// sm: this is at least the second time this idea has cropped up,
// and it still a mistake
#warning global_tfac is a mistake
TypeFactory * global_tfac = NULL;


// ------------------ AtomicType -----------------
ALLOC_STATS_DEFINE(AtomicType)

AtomicType::AtomicType()
{
  ALLOC_STATS_IN_CTOR
}


AtomicType::~AtomicType()
{
  ALLOC_STATS_IN_DTOR
}


DOWNCAST_IMPL(AtomicType, SimpleType)
DOWNCAST_IMPL(AtomicType, NamedAtomicType)
DOWNCAST_IMPL(AtomicType, CompoundType)
DOWNCAST_IMPL(AtomicType, EnumType)
DOWNCAST_IMPL(AtomicType, TypeVariable)


void AtomicType::gdb() const
{
  cout << toString() << endl;
}


string AtomicType::toString() const
{
  if (Type::printAsML) {
    return toMLString();
  }
  else {
    return toCString();
  }
}


bool AtomicType::isNamedAtomicType() const 
{
  // default to false; NamedAtomicType overrides
  return false;
}


bool AtomicType::equals(AtomicType const *obj) const
{
  // one exception to the below: when checking for
  // equality, TypeVariables are all equivalent
  if (this->isTypeVariable() &&
      obj->isTypeVariable()) {
    return true;
  }

  // sm: Something is really wrong here.  AtomicType::equals
  // is supposed to be simple.  The caller must be misusing it.
  #warning AtomicType::equals should be simple

  // all of the AtomicTypes are unique-representation,
  // so pointer equality suffices
  //
  // it is *important* that we don't do structural equality
  // here, because then we'd be confused by types with the
  // same name that appear in different scopes!
  //
  // dsw: but if they are instantiations of the same class template in
  // TemplTcheckMode TTM_2TEMPL_FUNC_DECL (the parameter list of a
  // function template) they might not be '==', but still be
  // isomorphic if they were instantiated with a template argument
  // that is the template parameter (type variable) of the function
  // template
  if (this->isCompoundType() && this->asCompoundTypeC()->templateInfo() &&
      obj->isCompoundType()  &&  obj->asCompoundTypeC()->templateInfo()) {
    xassert(global_tfac);
    MatchTypes match(*global_tfac, MatchTypes::MM_ISO);
    // FIX: this is gross, but to make matchtype such that I don't
    // have to do it adds a lot of complexity that I don't want to
    // test and debug right now; eventually match type should have an
    // API that takes AtomicTypes as arguments.
    //
    // I don't know how to not use the const_cast given the APIs.
    CVAtomicType *this_cva = global_tfac->makeCVAtomicType
      (SL_UNKNOWN, const_cast<AtomicType*>(this), CV_NONE);
    CVAtomicType *obj_cva  = global_tfac->makeCVAtomicType
      (SL_UNKNOWN, const_cast<AtomicType*>(obj), CV_NONE);
    return match.match_Type(this_cva, obj_cva);
  } else {
    return this == obj;
  }
}


// ------------------ SimpleType -----------------
SimpleType SimpleType::fixed[NUM_SIMPLE_TYPES] = {
  SimpleType(ST_CHAR),
  SimpleType(ST_UNSIGNED_CHAR),
  SimpleType(ST_SIGNED_CHAR),
  SimpleType(ST_BOOL),
  SimpleType(ST_INT),
  SimpleType(ST_UNSIGNED_INT),
  SimpleType(ST_LONG_INT),
  SimpleType(ST_UNSIGNED_LONG_INT),
  SimpleType(ST_LONG_LONG),
  SimpleType(ST_UNSIGNED_LONG_LONG),
  SimpleType(ST_SHORT_INT),
  SimpleType(ST_UNSIGNED_SHORT_INT),
  SimpleType(ST_WCHAR_T),
  SimpleType(ST_FLOAT),
  SimpleType(ST_DOUBLE),
  SimpleType(ST_LONG_DOUBLE),
  SimpleType(ST_VOID),

  SimpleType(ST_ELLIPSIS),
  SimpleType(ST_CDTOR),
  SimpleType(ST_ERROR),
  SimpleType(ST_DEPENDENT),
  
  SimpleType(ST_PROMOTED_INTEGRAL),
  SimpleType(ST_PROMOTED_ARITHMETIC),
  SimpleType(ST_INTEGRAL),
  SimpleType(ST_ARITHMETIC),
  SimpleType(ST_ARITHMETIC_NON_BOOL),
  SimpleType(ST_ANY_OBJ_TYPE),
  SimpleType(ST_ANY_NON_VOID),
  SimpleType(ST_ANY_TYPE),
};

string SimpleType::toCString() const
{
  return simpleTypeName(type);
}


int SimpleType::reprSize() const
{
  return simpleTypeReprSize(type);
}


// ------------------ NamedAtomicType --------------------
NamedAtomicType::NamedAtomicType(StringRef n)
  : name(n),
    typedefVar(NULL),
    access(AK_PUBLIC)
{}

NamedAtomicType::~NamedAtomicType()
{
  if (typedefVar) {
    delete typedefVar;
  }
}


bool NamedAtomicType::isNamedAtomicType() const 
{
  return true;
}


// ---------------- BaseClassSubobj ----------------
BaseClassSubobj::BaseClassSubobj(BaseClass const &base)
  : BaseClass(base),
    parents(),       // start with an empty list
    visited(false)   // may as well init; clients expected to clear as needed
{}


BaseClassSubobj::~BaseClassSubobj()
{
  // I don't think this code has been tested..

  // virtual parent objects are owned by the containing class'
  // 'virtualBases' list, but nonvirtual parents are owned
  // by the hierarchy nodes themselves (i.e. me)
  while (parents.isNotEmpty()) {
    BaseClassSubobj *parent = parents.removeFirst();

    if (!parent->isVirtual) {
      delete parent;
    }
  }
}


string BaseClassSubobj::canonName() const
{
  return stringc << ct->name << " (" << (void*)this << ")";
}


// ------------------ CompoundType -----------------
CompoundType::CompoundType(Keyword k, StringRef n)
  : NamedAtomicType(n),
    Scope(SK_CLASS, 0 /*changeCount*/, SL_UNKNOWN /*dummy loc*/),
    forward(true),
    keyword(k),
    bases(),
    virtualBases(),
    subobj(BaseClass(this, AK_PUBLIC, false /*isVirtual*/)),
    conversionOperators(),
    instName(n),
    syntax(NULL)
{
  curCompound = this;
  curAccess = (k==K_CLASS? AK_PRIVATE : AK_PUBLIC);
}

CompoundType::~CompoundType()
{
  //bases.deleteAll();    // automatic, and I'd need a cast to do it explicitly because it's 'const' now
  if (templateInfo()) {
    delete templateInfo();
    setTemplateInfo(NULL);      // dsw: I don't like pointers to deleted objects
  }
}


STATICDEF char const *CompoundType::keywordName(Keyword k)
{
  switch (k) {
    default:          xfailure("bad keyword");
    case K_STRUCT:    return "struct";
    case K_CLASS:     return "class";
    case K_UNION:     return "union";
  }
}

bool CompoundType::isTemplate() const
{
  TemplateInfo *tinfo = templateInfo();
  return tinfo != NULL &&
         tinfo->params.isNotEmpty();
}


Variable *CompoundType::getTypedefVar() const
{
  xassert(typedefVar);
  xassert(typedefVar->type);
  xassert(typedefVar->type->asCompoundTypeC() == this);
  return typedefVar;
}


TemplateInfo *CompoundType::templateInfo() const
{
  return getTypedefVar()->templateInfo();
}

void CompoundType::setTemplateInfo(TemplateInfo *templInfo0)
{
  return getTypedefVar()->setTemplateInfo(templInfo0);
}


bool CompoundType::hasVirtualFns() const
{
  // TODO: this fails to consider members inherited from base classes ...

  for (StringSObjDict<Variable>::IterC iter(getVariableIter());
       !iter.isDone(); iter.next()) {
    Variable *var0 = iter.value();
    if (var0->hasFlag(DF_VIRTUAL)) {
      xassert(var0->getType()->asRval()->isFunctionType());
      return true;
    }
  }
  return false;
}


string CompoundType::toCString() const
{
  stringBuilder sb;

  TemplateInfo *tinfo = templateInfo();
  bool hasParams = tinfo && tinfo->params.isNotEmpty();
  if (hasParams) {
    sb << tinfo->paramsToCString() << " ";
  }

  if (!tinfo || hasParams) {   
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
    sb << keywordName(keyword) << " ";
  }

  sb << (name? name : "/*anonymous*/");

  // template arguments are now in the name
  // 4/22/04: they were removed from the name a long time ago;
  //          but I'm just now uncommenting this code
  if (tinfo && tinfo->arguments.isNotEmpty()) {
    sb << sargsToString(tinfo->arguments);
  }
   
  return sb;
}


int CompoundType::reprSize() const
{
  int total = 0;
  for (StringSObjDict<Variable>::IterC iter(getVariableIter());
       !iter.isDone(); iter.next()) {
    Variable *v = iter.value();
    // count nonstatic data members
    if (!v->type->isFunctionType() &&
        !v->hasFlag(DF_TYPEDEF) &&
        !v->hasFlag(DF_STATIC)) {
      int membSize = v->type->reprSize();
      if (keyword == K_UNION) {
        // representation size is max over field sizes
        total = max(total, membSize);
      }
      else {
        // representation size is sum over field sizes
        total += membSize;
      }
    }
  }
  return total;
}


int CompoundType::numFields() const
{                                                
  int ct = 0;
  for (StringSObjDict<Variable>::IterC iter(getVariableIter());
       !iter.isDone(); iter.next()) {
    Variable *v = iter.value();
           
    // count nonstatic data members
    if (!v->type->isFunctionType() &&
        !v->hasFlag(DF_TYPEDEF) &&
        !v->hasFlag(DF_STATIC)) {
      ct++;                        
    }
  }

  return ct;
}


void CompoundType::addField(Variable *v)
{
  addVariable(v);
}


void CompoundType::afterAddVariable(Variable *v)
{
  // if is a data member, not a method, static data, or a typedef
  if (!v->type->isFunctionType() &&
      !(v->flags & (DF_STATIC | DF_TYPEDEF | DF_ENUMERATOR))) {
    dataMembers.append(v);
  }
}


int CompoundType::getDataMemberPosition(StringRef name) const
{        
  int index=0;
  SFOREACH_OBJLIST(Variable, dataMembers, iter) {
    if (iter.data()->name == name) {
      return index;
    }
    index++;
  }
  return -1;     // not found
}


void CompoundType::addBaseClass(BaseClass * /*owner*/ newBase)
{
  xassert(newBase->access != AK_UNSPECIFIED);

  // add the new base; override 'const' so we can modify the list
  const_cast<ObjList<BaseClass>&>(bases).append(newBase);

  // replicate 'newBase's inheritance hierarchy in the subobject
  // hierarchy, representing virtual inheritance with explicit sharing
  makeSubobjHierarchy(&subobj, newBase);
}

// all of this is done in the context of one class, the one whose
// tree we're building, so we can access its 'virtualBases' list
void CompoundType::makeSubobjHierarchy(
  // the root of the subobject hierarchy we're adding to
  BaseClassSubobj *subobj,

  // the base class to add to subobj's hierarchy
  BaseClass const *newBase)
{
  if (newBase->isVirtual) {
    // does 'newBase' correspond to an already-existing virtual base?
    BaseClassSubobj *vbase = findVirtualSubobject(newBase->ct);
    if (vbase) {
      // yes it's already a virtual base; just point 'subobj'
      // at the existing instance
      subobj->parents.append(vbase);

      // we're incorporating 'newBase's properties into 'vbase';
      // naturally both should already be marked as virtual
      xassert(vbase->isVirtual);

      // grant access according to the least-restrictive link
      if (newBase->access < vbase->access) {
        vbase->access = newBase->access;    // make it less restrictive
      }

      // no need to recursively examine 'newBase' since the presence
      // of 'vbase' in 'virtualBases' means we already incorporated
      // its inheritance structure
      return;
    }
  }

  // represent newBase in the subobj hierarchy
  BaseClassSubobj *newBaseObj = new BaseClassSubobj(*newBase);
  subobj->parents.append(newBaseObj);

  // if we just added a new virtual base, remember it
  if (newBaseObj->isVirtual) {
    virtualBases.append(newBaseObj);
  }

  // put all of newBase's base classes into newBaseObj
  FOREACH_OBJLIST(BaseClass, newBase->ct->bases, iter) {
    makeSubobjHierarchy(newBaseObj, iter.data());
  }
}


// simple scan of 'virtualBases'
BaseClassSubobj const *CompoundType::findVirtualSubobjectC
  (CompoundType const *ct) const
{
  FOREACH_OBJLIST(BaseClassSubobj, virtualBases, iter) {
    if (iter.data()->ct == ct) {
      return iter.data();
    }
  }
  return NULL;   // not found
}

  
// fundamentally, this takes advantage of the ownership scheme,
// where nonvirtual bases form a tree, and the 'virtualBases' list
// gives us additional trees of internally nonvirtual bases
void CompoundType::clearSubobjVisited() const
{
  // clear the 'visited' flags in the nonvirtual bases
  clearVisited_helper(&subobj);

  // clear them in the virtual bases
  FOREACH_OBJLIST(BaseClassSubobj, virtualBases, iter) {
    clearVisited_helper(iter.data());
  }
}

STATICDEF void CompoundType::clearVisited_helper
  (BaseClassSubobj const *subobj)
{
  subobj->visited = false;
  
  // recursively clear flags in the *nonvirtual* bases
  SFOREACH_OBJLIST(BaseClassSubobj, subobj->parents, iter) {
    if (!iter.data()->isVirtual) {
      clearVisited_helper(iter.data());
    }
  }
}


// interpreting the subobject hierarchy recursively is sometimes a bit
// of a pain, especially when the client doesn't care about the access
// paths, so this allows a more natural iteration by collecting them
// all into a list; this function provides a prototypical example of
// how to interpret the structure recursively, when that is necessary
void CompoundType::getSubobjects(SObjList<BaseClassSubobj const> &dest) const
{
  // reverse before also, in case there happens to be elements
  // already on the list, so those won't change order
  dest.reverse();

  clearSubobjVisited();
  getSubobjects_helper(dest, &subobj);

  // reverse the list since it was constructed in reverse order
  dest.reverse();
}

STATICDEF void CompoundType::getSubobjects_helper
  (SObjList<BaseClassSubobj const> &dest, BaseClassSubobj const *subobj)
{
  if (subobj->visited) return;
  subobj->visited = true;

  dest.prepend(subobj);

  SFOREACH_OBJLIST(BaseClassSubobj, subobj->parents, iter) {
    getSubobjects_helper(dest, iter.data());
  }
}


string CompoundType::renderSubobjHierarchy() const
{
  stringBuilder sb;
  sb << "// subobject hierarchy for " << name << "\n"
     << "digraph \"Subobjects\" {\n"
     ;

  SObjList<BaseClassSubobj const> objs;
  getSubobjects(objs);

  // look at all the subobjects
  SFOREACH_OBJLIST(BaseClassSubobj const, objs, iter) {
    BaseClassSubobj const *obj = iter.data();

    sb << "  \"" << obj->canonName() << "\" [\n"
       << "    label = \"" << obj->ct->name
                  << "\\n" << ::toString(obj->access) << "\"\n"
       << "  ]\n"
       ;

    // render 'obj's base class links
    SFOREACH_OBJLIST(BaseClassSubobj, obj->parents, iter) {
      BaseClassSubobj const *parent = iter.data();

      sb << "  \"" << parent->canonName() << "\" -> \"" 
                   << obj->canonName() << "\" [\n";
      if (parent->isVirtual) {
        sb << "    style = dashed\n";    // virtual inheritance: dashed link
      }
      sb << "  ]\n";
    }
  }

  sb << "}\n\n";

  return sb;
}


string toString(CompoundType::Keyword k)
{
  xassert((unsigned)k < (unsigned)CompoundType::NUM_KEYWORDS);
  return string(typeIntrNames[k]);    // see cc_type.h
}


int CompoundType::countBaseClassSubobjects(CompoundType const *ct) const
{
  SObjList<BaseClassSubobj const> objs;
  getSubobjects(objs);
    
  int count = 0;
  SFOREACH_OBJLIST(BaseClassSubobj const, objs, iter) {
    if (iter.data()->ct == ct) {
      count++;
    }
  }

  return count;
}

  
// simple recursive computation
void getBaseClasses(SObjSet<CompoundType*> &bases, CompoundType *ct)
{
  bases.add(ct);

  FOREACH_OBJLIST(BaseClass, ct->bases, iter) {
    getBaseClasses(bases, iter.data()->ct);
  }
}

STATICDEF CompoundType *CompoundType::lub
  (CompoundType *t1, CompoundType *t2, bool &wasAmbig)
{
  wasAmbig = false;

  // might be a common case?
  if (t1 == t2) {
    return t1;
  }

  // compute the set of base classes for each class
  SObjSet<CompoundType*> t1Bases, t2Bases;
  getBaseClasses(t1Bases, t1);
  getBaseClasses(t2Bases, t2);

  // look for an element in the intersection that has nothing below it
  // (hmm.. this will be quadratic due to linear-time 'hasBaseClass'...)
  CompoundType *least = NULL;
  {
    for (SObjSetIter<CompoundType*> iter(t1Bases); !iter.isDone(); iter.adv()) {
      if (!t2Bases.contains(iter.data())) continue;       // filter for intersection

      if (!least ||
          iter.data()->hasBaseClass(least)) {
        // new least
        least = iter.data();
      }
    }
  }

  if (!least) {
    return NULL;        // empty intersection
  }

  // check that it's the unique least
  for (SObjSetIter<CompoundType*> iter(t1Bases); !iter.isDone(); iter.adv()) {
    if (!t2Bases.contains(iter.data())) continue;       // filter for intersection

    if (least->hasBaseClass(iter.data())) {
      // least is indeed less than (or equal to) this one
    }
    else {
      // counterexample; 'least' is not in fact the least
      wasAmbig = true;
      return NULL;
    }
  }

  // good to go
  return least;
}


void CompoundType::finishedClassDefinition(StringRef specialName)
{
  // get inherited conversions
  FOREACH_OBJLIST(BaseClass, bases, iter) {
    conversionOperators.appendAll(iter.data()->ct->conversionOperators);
  }

  // add those declared locally; they hide any that are inherited
  Variable *localOps = rawLookupVariable(specialName);
  if (!localOps) {
    return;      // nothing declared locally
  }
  
  if (!localOps->overload) {
    addLocalConversionOp(localOps);
  }
  else {
    SFOREACH_OBJLIST_NC(Variable, localOps->overload->set, iter) {
      addLocalConversionOp(iter.data());
    }
  }
}

void CompoundType::addLocalConversionOp(Variable *op)
{
  Type *opRet = op->type->asFunctionTypeC()->retType;

  // remove any existing conversion operators that yield the same type
  {
    SObjListMutator<Variable> mut(conversionOperators);
    while (!mut.isDone()) {
      Type *mutRet = mut.data()->type->asFunctionTypeC()->retType;

      if (mutRet->equals(opRet)) {
        mut.remove();    // advances the iterator
      }
      else {
        mut.adv();
      }
    }
  }
  
  // add 'op'
  conversionOperators.append(op);
}


// a filter on elements of an overload set of members in 'ct'
typedef bool (*OverloadFilter)(CompoundType *ct, FunctionType *ft);

Variable *overloadSetFilter(CompoundType *ct, Variable *start, OverloadFilter func)
{
  if (!start || !start->type->isFunctionType()) {
    return NULL;               // name maps to no function
  }

  if (!start->overload) {
    if (func(ct, start->type->asFunctionType())) {
      return start;            // name maps to one thing, it passes
    }
    else {
      return NULL;             // name maps to one thing, it fails
    }
  }

  // name maps to multiple things, consider each
  SFOREACH_OBJLIST_NC(Variable, start->overload->set, iter) {
    if (func(ct, iter.data()->type->asFunctionType())) {
      return iter.data();      // the first one that passes
    }
  }

  return NULL;                 // none pass
}


static bool defaultCtorTest(CompoundType *ct, FunctionType *ft)
{
  // every parameter must have a default value
  return ft->paramsHaveDefaultsPast(0);
}

Variable *CompoundType::getDefaultCtor()
{
  return overloadSetFilter(this, rawLookupVariable("constructor-special"),
                           defaultCtorTest);
}


// true if parameter 'n' is a reference to 'ct'
static bool nthIsCtReference(CompoundType *ct, FunctionType *ft, int n)
{
  if (ft->params.count() <= n) {
    return false;
  }
  Type *t = ft->params.nth(n)->type;
  if (t->isReference() &&
      t->asRval()->ifCompoundType() == ct) {
    return true;
  }
  else {
    return false;
  }
}

static bool copyCtorTest(CompoundType *ct, FunctionType *ft)
{
  return
    nthIsCtReference(ct, ft, 0) &&   // first param must be a reference to 'ct'
    ft->paramsHaveDefaultsPast(1);   // subsequent have defaults
}

Variable *CompoundType::getCopyCtor()
{
  return overloadSetFilter(this, rawLookupVariable("constructor-special"),
                           copyCtorTest);
}


static bool assignOperatorTest(CompoundType *ct, FunctionType *ft)
{
  return
    ft->isMethod() &&                // first param is receiver object
    nthIsCtReference(ct, ft, 1) &&   // second param must be a reference to 'ct'
    ft->paramsHaveDefaultsPast(2);   // subsequent have defaults
}

Variable *CompoundType::getAssignOperator()
{
  return overloadSetFilter(this, rawLookupVariable("operator="),
                           assignOperatorTest);
}


Variable *CompoundType::getDtor()
{
  // synthesize the dtor name... maybe I should be using
  // "destructor-special" or something
  return rawLookupVariable(stringc << "~" << name);
}


// ---------------- EnumType ------------------
EnumType::~EnumType()
{}


string EnumType::toCString() const
{
  return stringc << "enum " << (name? name : "/*anonymous*/");
}


int EnumType::reprSize() const
{
  // this is the usual choice
  return simpleTypeReprSize(ST_INT);
}


EnumType::Value *EnumType::addValue(StringRef name, int value, Variable *decl)
{
  xassert(!valueIndex.isMapped(name));

  Value *v = new Value(name, this, value, decl);
  valueIndex.add(name, v);
  
  // 7/22/04: At one point I was also maintaining a linked list of
  // the Value objects.  Daniel pointed out this was quadratic b/c
  // I was using 'append()'.  Since I never used the list anyway,
  // I just dropped it in favor of the dictionary (only).
  
  return v;
}


EnumType::Value const *EnumType::getValue(StringRef name) const
{
  Value const *v;
  if (valueIndex.queryC(name, v)) { 
    return v;
  }
  else {
    return NULL;
  }
}


// ---------------- EnumType::Value --------------
EnumType::Value::Value(StringRef n, EnumType *t, int v, Variable *d)
  : name(n), type(t), value(v), decl(d)
{}

EnumType::Value::~Value()
{}


// ------------------ TypeVariable ----------------
TypeVariable::~TypeVariable()
{}

string TypeVariable::toCString() const
{
  // use the "typename" syntax instead of "class", to distinguish
  // this from an ordinary class, and because it's syntax which
  // more properly suggests the ability to take on *any* type,
  // not just those of classes
  //
  // but, the 'typename' syntax can only be used in some specialized
  // circumstances.. so I'll suppress it in the general case and add
  // it explicitly when printing the few constructs that allow it
  //return stringc << "/*typename*/ " << name;
  return stringc << "/*typevar "
//                   << "typedefVar->serialNumber:"
//                   << (typedefVar ? typedefVar->serialNumber : -1)
                 << "*/:" << name;
}

int TypeVariable::reprSize() const
{
  //xfailure("you can't ask a type variable for its size");

  // this happens when we're typechecking a template class, without
  // instantiating it, and we want to verify that some size expression
  // is constant.. so make up a number
  return 4;
}


// -------------------- BaseType ----------------------
ALLOC_STATS_DEFINE(BaseType)

bool BaseType::printAsML = false;


BaseType::BaseType()
  : typedefAliases()     // initially empty
{
  ALLOC_STATS_IN_CTOR
}

BaseType::~BaseType()
{
  ALLOC_STATS_IN_DTOR
}


DOWNCAST_IMPL(BaseType, CVAtomicType)
DOWNCAST_IMPL(BaseType, PointerType)
DOWNCAST_IMPL(BaseType, ReferenceType)
DOWNCAST_IMPL(BaseType, FunctionType)
DOWNCAST_IMPL(BaseType, ArrayType)
DOWNCAST_IMPL(BaseType, PointerToMemberType)


bool BaseType::equals(BaseType const *obj, EqFlags flags) const
{
  if (flags & EF_POLYMORPHIC) {
    // check for polymorphic match
    if (obj->isSimpleType()) {
      SimpleTypeId objId = obj->asSimpleTypeC()->type;
      if (!(ST_PROMOTED_INTEGRAL <= objId && objId <= ST_ANY_TYPE)) {
        goto not_polymorphic;
      }

      // check those that can match any type constructor
      if (objId == ST_ANY_TYPE) {
        return true;
      }

      if (objId == ST_ANY_NON_VOID) {
        return !this->isVoid();
      }

      if (objId == ST_ANY_OBJ_TYPE) {
        return !this->isFunctionType() &&
               !this->isVoid();
      }

      // check those that only match atomics
      if (this->isSimpleType()) {
        SimpleTypeId thisId = this->asSimpleTypeC()->type;
        SimpleTypeFlags flags = simpleTypeInfo(thisId).flags;

        // see cppstd 13.6 para 2
        if (objId == ST_PROMOTED_INTEGRAL) {
          return (flags & (STF_INTEGER | STF_PROM)) == (STF_INTEGER | STF_PROM);
        }

        if (objId == ST_PROMOTED_ARITHMETIC) {
          return (flags & (STF_INTEGER | STF_PROM)) == (STF_INTEGER | STF_PROM) ||
                 (flags & STF_FLOAT);      // need not be promoted!
        }

        if (objId == ST_INTEGRAL) {
          return (flags & STF_INTEGER) != 0;
        }

        if (objId == ST_ARITHMETIC) {
          return (flags & (STF_INTEGER | STF_FLOAT)) != 0;
        }

        if (objId == ST_ARITHMETIC_NON_BOOL) {
          return thisId != ST_BOOL &&
                 (flags & (STF_INTEGER | STF_FLOAT)) != 0;
        }
      }

      // polymorphic type failed to match
      return false;
    }
  }

not_polymorphic:
  if (getTag() != obj->getTag()) {
    return false;
  }

  switch (getTag()) {
    default: xfailure("bad tag");
    #define C(tag,type) \
      case tag: return ((type const*)this)->innerEquals((type const*)obj, flags);
    C(T_ATOMIC, CVAtomicType)
    C(T_POINTER, PointerType)
    C(T_REFERENCE, ReferenceType)
    C(T_FUNCTION, FunctionType)
    C(T_ARRAY, ArrayType)
    C(T_POINTERTOMEMBER, PointerToMemberType)
    #undef C
  }
}


unsigned BaseType::hashValue() const
{
  unsigned h = innerHashValue();
  
  // 'h' now has quite a few bits of entropy, but they're mostly
  // in the high bits.  push it through a PRNG to mix it better.
  return lcprngTwoSteps(h);
}


string cvToString(CVFlags cv)
{
  if (cv != CV_NONE) {
    return stringc << " " << toString(cv);
  }
  else {
    return string("");
  }
}


void BaseType::gdb() const
{
  cout << toString() << endl;
}

string BaseType::toString() const 
{
  if (printAsML) {
    return toMLString();
  }
  else {
    return toCString();
  }
}


string BaseType::toCString() const
{
  if (isCVAtomicType()) {
    // special case a single atomic type, so as to avoid
    // printing an extra space
    CVAtomicType const *atomic = asCVAtomicTypeC();
    return stringc
      << atomic->atomic->toCString()
      << cvToString(atomic->cv);
  }
  else {
    return stringc
      << leftString()
      << rightString();
  }
}

string BaseType::toCString(char const *name) const
{
  // print the inner parentheses if the name is omitted
  bool innerParen = (name && name[0])? false : true;

  #if 0    // wrong
  // except, if this type is a pointer, then omit the parens anyway;
  // we only need parens when the type is a function or array and
  // the name is missing
  if (isPointerType()) {
    innerParen = false;
  }
  #endif // 0

  stringBuilder s;
  s << leftString(innerParen);
  s << (name? name : "/*anon*/");
  s << rightString(innerParen);
  return s;
}

// this is only used by CVAtomicType.. all others override it
string BaseType::rightString(bool /*innerParen*/) const
{
  return "";
}


bool BaseType::anyCtorSatisfiesF(TypePredFunc f) const 
{
  StatelessTypePred stp(f);
  return anyCtorSatisfies(stp);
}


CVFlags BaseType::getCVFlags() const
{
  return CV_NONE;
}


bool BaseType::isSimpleType() const
{
  if (isCVAtomicType()) {
    AtomicType const *at = asCVAtomicTypeC()->atomic;
    return at->isSimpleType();
  }
  else {
    return false;
  }
}

SimpleType const *BaseType::asSimpleTypeC() const
{
  return asCVAtomicTypeC()->atomic->asSimpleTypeC();
}

bool BaseType::isSimple(SimpleTypeId id) const
{
  return isSimpleType() &&
         asSimpleTypeC()->type == id;
}

bool BaseType::isIntegerType() const
{
  return isSimpleType() &&
         ::isIntegerType(asSimpleTypeC()->type);
}


bool BaseType::isEnumType() const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC()->atomic->isEnumType();
}


bool BaseType::isDependent() const 
{
  // 14.6.2.1: type variables (template parameters) are the base case
  // for dependent types
  return isSimple(ST_DEPENDENT) ||
         isTypeVariable();
}


bool BaseType::isCompoundTypeOf(CompoundType::Keyword keyword) const
{
  if (isCVAtomicType()) {
    CVAtomicType const *a = asCVAtomicTypeC();
    return a->atomic->isCompoundType() &&
           a->atomic->asCompoundTypeC()->keyword == keyword;
  }
  else {
    return false;
  }
}

NamedAtomicType *BaseType::ifNamedAtomicType()
{
  return isNamedAtomicType()? asNamedAtomicType() : NULL;
}

CompoundType *BaseType::ifCompoundType()
{
  return isCompoundType()? asCompoundType() : NULL;
}

NamedAtomicType const *BaseType::asNamedAtomicTypeC() const
{
  return asCVAtomicTypeC()->atomic->asNamedAtomicTypeC();
}

CompoundType const *BaseType::asCompoundTypeC() const
{
  return asCVAtomicTypeC()->atomic->asCompoundTypeC();
}

bool BaseType::isOwnerPtr() const
{
  return isPointer() && ((asPointerTypeC()->cv & CV_OWNER) != 0);
}

bool BaseType::isMethod() const
{
  return isFunctionType() &&
         asFunctionTypeC()->isMethod();
}

#if 0     // obsolete, delete me
bool BaseType::isPointer() const
{
  return isPointerType() && asPointerTypeC()->op == PO_POINTER;
}

bool BaseType::isReference() const
{
  return isPointerType() && asPointerTypeC()->op == PO_REFERENCE;
}
#endif // 0

bool BaseType::isReferenceToConst() const {
  return isReferenceType() && asReferenceTypeC()->atType->isConst();
}

Type *BaseType::getAtType() const
{
  if (isPointerType()) {
    return asPointerTypeC()->atType;
  }
  else if (isReferenceType()) {
    return asReferenceTypeC()->atType;
  }
  else if (isArrayType()) {
    return asArrayTypeC()->eltType;
  }
  else if (isPointerToMemberType()) {
    return asPointerToMemberTypeC()->atType;
  }
  else {
    xfailure("illegal call to getAtType");
    return NULL;       // silence warning
  }
}

BaseType const *BaseType::asRvalC() const
{
  if (isReference()) {
    // note that due to the restriction about stacking reference
    // types, unrolling more than once is never necessary
    return asReferenceTypeC()->atType;
  }
  else {
    return this;       // this possibility is one reason to not make 'asRval' return 'Type*' in the first place
  }
}


bool BaseType::isCVAtomicType(AtomicType::Tag tag) const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC()->atomic->getTag() == tag;
}


TypeVariable *BaseType::asTypeVariable() 
{ 
  return asCVAtomicType()->atomic->asTypeVariable(); 
}


bool BaseType::isNamedAtomicType() const {
  return isCVAtomicType() && asCVAtomicTypeC()->atomic->isNamedAtomicType();
}


bool typeIsError(Type const *t)
{
  return t->isError();
}

bool BaseType::containsErrors() const
{
  // hmm.. bit of hack..
  return static_cast<Type const *>(this)->
    anyCtorSatisfiesF(typeIsError);
}


bool typeHasTypeVariable(Type const *t)
{
  return t->isTypeVariable() ||
         (t->isCompoundType() &&
          t->asCompoundTypeC()->isTemplate());
}

bool BaseType::containsTypeVariables() const
{
  return static_cast<Type const *>(this)->
    anyCtorSatisfiesF(typeHasTypeVariable);
}


bool hasVariable(Type const *t)
{
  if (t->isTypeVariable()) return true;
  if (t->isCompoundType() && t->asCompoundTypeC()->isTemplate()) {
    return t->asCompoundTypeC()->templateInfo()->argumentsContainVariables();
  }
  // FIX: Extend for function types
  return false;
}

bool BaseType::containsVariables() const
{
  return static_cast<Type const *>(this)->
    anyCtorSatisfiesF(hasVariable);
}


Variable *BaseType::typedefName()
{
  return typedefAliases.isEmpty()? NULL : typedefAliases.first();
}


string toString(Type *t)
{
  return t->toString();
}


// ----------------- CVAtomicType ----------------
bool CVAtomicType::innerEquals(CVAtomicType const *obj, EqFlags flags) const
{
  return atomic->equals(obj->atomic) &&
         ((flags & EF_OK_DIFFERENT_CV) || (cv == obj->cv));
}


// map cv down to {0,1,2,3}
inline unsigned cvHash(CVFlags cv)
{
  return cv >> CV_SHIFT_AMOUNT;
}

unsigned CVAtomicType::innerHashValue() const
{
  // underlying atomic is pointer-based equality
  return (unsigned)atomic + 
         cvHash(cv);
         // T_ATOMIC is zero anyway
}


string CVAtomicType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atomic->toCString();
  s << cvToString(cv);

  // this is the only mandatory space in the entire syntax
  // for declarations; it separates the type specifier from
  // the declarator(s)
  s << " ";

  return s;
}


int CVAtomicType::reprSize() const
{
  return atomic->reprSize();
}


bool CVAtomicType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this);
}


CVFlags CVAtomicType::getCVFlags() const
{
  return cv;
}


// ------------------- PointerType ---------------
PointerType::PointerType(CVFlags c, Type *a)
  : cv(c), atType(a)
{
  // it makes no sense to stack reference operators underneath
  // other indirections (i.e. no ptr-to-ref, nor ref-to-ref)
  xassert(!a->isReference());
}


bool PointerType::innerEquals(PointerType const *obj, EqFlags flags) const
{
  // note how EF_IGNORE_TOP_CV allows *this* type's cv flags to differ,
  // but it's immediately suppressed once we go one level down; this
  // behavior repeated in all 'innerEquals' methods

  return ((flags & EF_OK_DIFFERENT_CV) || (cv == obj->cv)) &&
         atType->equals(obj->atType, flags & EF_PTR_PROP);
}


enum {
  // enough to leave room for a composed type to describe itself
  // (essentially 5 bits)
  HASH_KICK = 33,

  // put the tag in the upper part of those 5 bits
  TAG_KICK = 7
};

unsigned PointerType::innerHashValue() const
{
  // The essential strategy for composed types is to multiply the
  // underlying type's hash by HASH_KICK, pushing the existing bits up
  // (but with mixing), and then add our local info, including the
  // tag.
  //
  // I don't claim to be an expert at designing hash functions.  There
  // are probably better ones; if you think you know one, let me know.
  // My plan is to watch the hash outputs produced during operator
  // overload resolution, and tweak the function if I see an obvious
  // avenue for improvement.

  return atType->innerHashValue() * HASH_KICK +
         cvHash(cv) +
         T_POINTER * TAG_KICK;
}


string PointerType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << "*";
  if (cv) {
    // 1/03/03: added this space so "Foo * const arf" prints right (t0012.cc)
    s << cvToString(cv) << " ";
  }
  return s;
}

string PointerType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}


int PointerType::reprSize() const
{
  // a typical value .. (architecture-dependent)
  return 4;
}


bool PointerType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags PointerType::getCVFlags() const
{
  return cv;
}


// ------------------- ReferenceType ---------------
ReferenceType::ReferenceType(Type *a)
  : atType(a)
{
  // it makes no sense to stack reference operators underneath
  // other indirections (i.e. no ptr-to-ref, nor ref-to-ref)
  xassert(!a->isReference());
}


bool ReferenceType::innerEquals(ReferenceType const *obj, EqFlags flags) const
{
  return atType->equals(obj->atType, flags & EF_PTR_PROP);
}


unsigned ReferenceType::innerHashValue() const
{
  return atType->innerHashValue() * HASH_KICK +
         T_REFERENCE * TAG_KICK;
}


string ReferenceType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << "&";
  return s;
}

string ReferenceType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}


int ReferenceType::reprSize() const
{
  return 4;
}


bool ReferenceType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags ReferenceType::getCVFlags() const
{
  return CV_NONE;
}


// -------------------- FunctionType::ExnSpec --------------
FunctionType::ExnSpec::ExnSpec(ExnSpec const &obj)
{                                   
  // copy list contents
  types = obj.types;
}


FunctionType::ExnSpec::~ExnSpec()
{
  types.removeAll();
}


bool FunctionType::ExnSpec::anyCtorSatisfies(TypePred &pred) const
{
  SFOREACH_OBJLIST(Type, types, iter) {
    if (iter.data()->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}


// -------------------- FunctionType -----------------
FunctionType::FunctionType(Type *r)
  : flags(FF_NONE),
    retType(r),
    params(),
    exnSpec(NULL)
{}


FunctionType::~FunctionType()
{
  if (exnSpec) {
    delete exnSpec;
  }
}


// sm: I moved 'isCopyConstructorFor' and 'isCopyAssignOpFor' out into
// cc_tcheck.cc since the rules are fairly specific to the analysis
// being performed there


// NOTE:  This function (and the parameter comparison function that
// follows) is at the core of the overloadability analysis; it's
// complicated.  The set of EqFlags has been carefully chosen to try
// to make it as transparent as possible, but there is still
// significant subtlety.
//
// If you modify what goes on here, be sure to
//   (a) add your own regression test (or modify an existing one) to 
//       check your intended functionality, for the benefit of future
//       maintenance efforts, and
//   (b) make sure the existing tests still pass!
//
// Of course, this advice holds for almost any code, but this one is
// sufficiently central to warrant a reminder.

bool FunctionType::innerEquals(FunctionType const *obj, EqFlags flags) const
{
  // I do not compare 'FunctionType::flags' explicitly since their
  // meaning is generally a summary of other information, or of the
  // name (which is irrelevant to the type)

  if (!(flags & EF_IGNORE_RETURN)) {
    // check return type
    if (!retType->equals(obj->retType, flags & EF_PROP)) {
      return false;
    }
  }

  // see comment at definition of EF_ALLOW_KR_PARAM_OMIT
  if (flags & EF_ALLOW_KR_PARAM_OMIT) {
    if (acceptsVarargs() || obj->acceptsVarargs()) {
      return true;
    }
  }

  if (!(flags & EF_STAT_EQ_NONSTAT)) {
    // check that either both are nonstatic methods, or both are not
    if (isMethod() != obj->isMethod()) {
      return false;
    }
  }

  // check that both are variable-arg, or both are not
  if (acceptsVarargs() != obj->acceptsVarargs()) {
    return false;
  }

  // check the parameter lists
  if (!equalParameterLists(obj, flags)) {
    return false;
  }
  
  if (!(flags & EF_IGNORE_EXN_SPEC)) {
    // check the exception specs
    if (!equalExceptionSpecs(obj)) {
      return false;
    }
  }
  
  return true;
}

bool FunctionType::equalParameterLists(FunctionType const *obj,
                                       EqFlags flags) const
{
  SObjListIter<Variable> iter1(this->params);
  SObjListIter<Variable> iter2(obj->params);

  // skip the 'this' parameter(s) if desired, or if one has it
  // but the other does not (can arise if EF_STAT_EQ_NONSTAT has
  // been specified)
  {
    bool m1 = this->isMethod();
    bool m2 = obj->isMethod();
    bool ignore = flags & EF_IGNORE_IMPLICIT;
    if (m1 && (!m2 || ignore)) {
      iter1.adv();
    }
    if (m2 && (!m1 || ignore)) {
      iter2.adv();
    }
  }

  // this takes care of FunctionType::innerEquals' obligation
  // to suppress non-propagated flags after consumption
  flags &= EF_PROP;

  if (flags & EF_IGNORE_PARAM_CV) {
    // allow toplevel cv flags on parameters to differ
    flags |= EF_IGNORE_TOP_CV;
  }

  for (; !iter1.isDone() && !iter2.isDone();
       iter1.adv(), iter2.adv()) {
    // parameter names do not have to match, but
    // the types do
    if (iter1.data()->type->equals(iter2.data()->type, flags)) {
      // ok
    }
    else {
      return false;
    }
  }

  return iter1.isDone() == iter2.isDone();
}


// almost identical code to the above.. list comparison is
// always a pain..
bool FunctionType::equalExceptionSpecs(FunctionType const *obj) const
{
  if (exnSpec==NULL && obj->exnSpec==NULL)  return true;
  if (exnSpec==NULL || obj->exnSpec==NULL)  return false;

  // hmm.. this is going to end up requiring that exception specs
  // be listed in the same order, whereas I think the semantics
  // imply they're more like a set.. oh well
  SObjListIter<Type> iter1(exnSpec->types);
  SObjListIter<Type> iter2(obj->exnSpec->types);
  for (; !iter1.isDone() && !iter2.isDone();
       iter1.adv(), iter2.adv()) {
    if (iter1.data()->equals(iter2.data())) {
      // ok
    }
    else {
      return false;
    }
  }

  return iter1.isDone() == iter2.isDone();
}


bool FunctionType::paramsHaveDefaultsPast(int startParam) const
{
  SObjListIter<Variable> iter(params);

  // count off 'startParams' parameters that are not tested
  while (!iter.isDone() && startParam>0) {
    iter.adv();
    startParam--;
  }

  // all remaining parameters must accept defaults
  for (; !iter.isDone(); iter.adv()) {
    if (!iter.data()->value) {
      return false;     // doesn't have a default value
    }
  }

  return true;          // all params after 'startParam' have defaults
}


unsigned FunctionType::innerHashValue() const
{
  // process return type similarly to how other constructed types
  // handle their underlying type
  unsigned val = retType->innerHashValue() * HASH_KICK +
                 params.count() +
                 T_FUNCTION * TAG_KICK;
                 
  // now factor in the parameter types
  int ct = 1;
  SFOREACH_OBJLIST(Variable, params, iter) { 
    // similar to return value
    unsigned p = iter.data()->type->innerHashValue() * HASH_KICK +
                 (ct + T_FUNCTION) * TAG_KICK;
                 
    // multiply with existing hash, sort of like each parameter
    // is another dimension and we're constructing a tuple in
    // some space
    val *= p;
  }          
  
  // don't consider exnSpec or templateInfo; I don't think I'll be
  // encountering situations where I want the hash to be sensitive to
  // those
  
  // the 'flags' information is mostly redundant with the parameter
  // list, so don't bother folding that in

  return val;
}


void FunctionType::addParam(Variable *param)
{
  xassert(param->hasFlag(DF_PARAMETER));
  params.append(param);
}

void FunctionType::addReceiver(Variable *param)
{
  xassert(param->type->isReference());
  xassert(param->hasFlag(DF_PARAMETER));
  xassert(!isConstructor());    // ctors don't get a __receiver param
  xassert(!isMethod());         // this is about to change below
  params.prepend(param);
  flags |= FF_METHOD;
}

void FunctionType::doneParams()
{}


void FunctionType::registerRetVal(Variable *retVal)
{}


Variable const *FunctionType::getReceiverC() const
{
  xassert(isMethod());
  Variable const *ret = params.firstC();
  xassert(ret->getTypeC()->isReference());
  return ret;
}

CVFlags FunctionType::getReceiverCV() const
{
  if (isMethod()) {
    // expect 'this' to be of type 'SomeClass cv &', and
    // dig down to get that 'cv'
    return getReceiverC()->type->asReferenceType()->atType->asCVAtomicType()->cv;
  }
  else {
    return CV_NONE;
  }
}

CompoundType *FunctionType::getClassOfMember()
{
  return getReceiver()->type->asReferenceType()->atType->asCompoundType();
}


string FunctionType::leftString(bool innerParen) const
{
  stringBuilder sb;

  // FIX: FUNC TEMPLATE LOSS
  // template parameters
//    if (templateInfo) {
//      sb << templateInfo->paramsToCString() << " ";
//    }

  // return type and start of enclosing type's description
  if (flags & (/*FF_CONVERSION |*/ FF_CTOR | FF_DTOR)) {
    // don't print the return type, it's implicit       

    // 7/18/03: changed so we print ret type for FF_CONVERSION,
    // since otherwise I can't tell what it converts to!
  }
  else {
    sb << retType->leftString();
  }

  // NOTE: we do *not* propagate 'innerParen'!
  if (innerParen) {
    sb << "(";
  }

  return sb;
}

string FunctionType::rightString(bool innerParen) const
{
  // I split this into two pieces because the Cqual++ concrete
  // syntax puts $tainted into the middle of my rightString,
  // since it's following the placement of 'const' and 'volatile'
  return stringc
    << rightStringUpToQualifiers(innerParen)
    << rightStringAfterQualifiers();
}

string FunctionType::rightStringUpToQualifiers(bool innerParen) const
{
  // finish enclosing type
  stringBuilder sb;
  if (innerParen) {
    sb << ")";
  }

  // arguments
  sb << "(";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    ct++;
    if (isMethod() && ct==1) {
      // don't actually print the first parameter;
      // the 'm' stands for nonstatic member function
      sb << "/""*m: " << iter.data()->type->toCString() << " *""/ ";
      continue;
    }
    if (ct >= 3 || (!isMethod() && ct>=2)) {
      sb << ", ";
    }
    sb << iter.data()->toCStringAsParameter();
  }

  if (acceptsVarargs()) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << "...";
  }

  sb << ")";

  return sb;
}

string FunctionType::rightStringAfterQualifiers() const
{
  stringBuilder sb;

  CVFlags cv = getReceiverCV();
  if (cv) {
    sb << " " << ::toString(cv);
  }

  // exception specs
  if (exnSpec) {
    sb << " throw(";                
    int ct=0;
    SFOREACH_OBJLIST(Type, exnSpec->types, iter) {
      if (ct++ > 0) {
        sb << ", ";
      }
      sb << iter.data()->toCString();
    }
    sb << ")";
  }

  // hook for verifier syntax
  extraRightmostSyntax(sb);

  // finish up the return type
  sb << retType->rightString();

  return sb;
}

void FunctionType::extraRightmostSyntax(stringBuilder &) const
{}


int FunctionType::reprSize() const
{
  // thinking here about how this works when we're summing
  // the fields of a class with member functions ..
  return 0;
}


bool parameterListCtorSatisfies(TypePred &pred,
                                SObjList<Variable> const &params)
{
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (iter.data()->type->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}

bool FunctionType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         retType->anyCtorSatisfies(pred) ||
         parameterListCtorSatisfies(pred, params) ||
         (exnSpec && exnSpec->anyCtorSatisfies(pred));
  // FIX: FUNC TEMPLATE LOSS
  //
  // UPDATE: this is actually symmetric with the way that compound
  // type templates are dealt with, which is to say we do not recurse
  // on the parameters
//      || (templateInfo && templateInfo->anyParamCtorSatisfies(pred));
}


// ------------------ TemplateParams ---------------
TemplateParams::TemplateParams(TemplateParams const &obj)
  : params(obj.params)
{}

TemplateParams::~TemplateParams()
{}


string TemplateParams::paramsToCString() const
{
  stringBuilder sb;
  sb << "template <";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *p = iter.data();
    if (ct++ > 0) {
      sb << ", ";
    }

    if (p->type->isTypeVariable()) {
      sb << "class " << p->type->asTypeVariable()->name;
    }
    else {
      // non-type parameter
      sb << p->toCStringAsParameter();
    }
  }
  sb << ">";
  return sb;
}


bool TemplateParams::anyParamCtorSatisfies(TypePred &pred) const
{
  return parameterListCtorSatisfies(pred, params);
}


// ------------------ TemplateInfo -------------
TemplateInfo::TemplateInfo(StringRef name, SourceLoc il)
  : TemplateParams(),
    baseName(name),
    declSyntax(NULL),
    myPrimary(NULL),
    instantiatedFrom(NULL),
    instantiations(),         // empty list
    arguments(),              // empty list
    instLoc(il)
{}


TemplateInfo::TemplateInfo(TemplateInfo const &obj)
  : TemplateParams(obj),
    baseName(obj.baseName),
    myPrimary(obj.myPrimary),
    instantiations(obj.instantiations),      // suspicious... oh well
    arguments(),                             // copied below
    instLoc(obj.instLoc)
{
  // arguments
  FOREACH_OBJLIST(STemplateArgument, obj.arguments, iter) {
    arguments.prepend(new STemplateArgument(*(iter.data())));
  }
  arguments.reverse();
}


TemplateInfo::~TemplateInfo()
{}


void TemplateInfo::setMyPrimary(TemplateInfo *prim) 
{
  xassert(prim);                // argument must be non-NULL
  xassert(prim->isPrimary());   // myPrimary can only be a primary
  xassert(!myPrimary);          // can only do this once
  xassert(isNotPrimary());      // can't set myPrimary of a primary
  myPrimary = prim;
}


// this one is idempotent
TemplateInfo *TemplateInfo::getMyPrimaryIdem() const
{
  // I know, this is weird; you have to protect isPrimary() from being
  // called on a mutant.
  if (isMutant()) {
    xassert(myPrimary);
    return myPrimary;
  } else if (isPrimary()) {
    xassert(!myPrimary);
    return const_cast<TemplateInfo*>(this);
  } else {
    xassert(myPrimary);
    return myPrimary;
  }
}


Variable *TemplateInfo::addInstantiation(Variable *inst0, bool suppressDup)
{
  xassert(inst0);
  xassert(inst0->templateInfo());
  inst0->templateInfo()->setMyPrimary(this);
  // check that we don't match something that is already on the list
  // FIX: I suppose we should turn this off since it makes it quadratic

  // FIX: could do this if we had an Env
//    xassert(!getInstThatMatchesArgs(env, inst->templateInfo()->arguments));

  bool inst0IsMutant = inst0->templateInfo()->isMutant();
  SFOREACH_OBJLIST_NC(Variable, getInstantiations(), iter) {
    Variable *inst1 = iter.data();
    MatchTypes match(*global_tfac, MatchTypes::MM_ISO);
    bool unifies = match.match_Lists2
      (inst1->templateInfo()->arguments,
       inst0->templateInfo()->arguments,
       2 /*matchDepth*/);
    if (unifies) {
      if (inst0IsMutant) {
        xassert(inst1->templateInfo()->isMutant());
        // Note: Never remove duplicate mutants; isomorphic mutants
        // are not interchangable.
      } else {
        xassert(!inst1->templateInfo()->isMutant());
        // other, non-mutant instantiations should never be duplicated
        //        xassert(!unifies);
        xassert(suppressDup);
        return inst1;
      }
    }
  }

  instantiations.append(inst0);
  return inst0;
}


SObjList<Variable> &TemplateInfo::getInstantiations()
{
  return instantiations;
}


Variable *TemplateInfo::getInstantiationOfVar(Variable *var)
{
  xassert(var->templateInfo());
  ObjList<STemplateArgument> &varTArgs = var->templateInfo()->arguments;
  Variable *matchingVar = NULL;
  SFOREACH_OBJLIST_NC(Variable, getInstantiations(), iter) {
    Variable *candidate = iter.data();
    MatchTypes match(*global_tfac, MatchTypes::MM_ISO);
    // FIX: I use MT_NONE only because the matching is supposed to be
    // exact.  If you wanted the standard effect of const/volatile not
    // making a difference at the top of a parameter type, you would
    // have to match each parameter separately
    if (!match.match_Type(var->type, candidate->type, 2 /*matchDepth*/)) continue;
    if (!match.match_Lists2(varTArgs, candidate->templateInfo()->arguments, 2 /*matchDepth*/)) {
      continue;
    }
    xassert(!matchingVar);      // shouldn't be in there twice
    matchingVar = iter.data();
  }
  return matchingVar;
}


bool TemplateInfo::equalArguments
  (SObjList<STemplateArgument> const &list) const
{
  ObjListIter<STemplateArgument> iter1(arguments);
  SObjListIter<STemplateArgument> iter2(list);

  while (!iter1.isDone() && !iter2.isDone()) {
    STemplateArgument const *sta1 = iter1.data();
    STemplateArgument const *sta2 = iter2.data();
    if (!sta1->equals(sta2)) {
      return false;
    }

    iter1.adv();
    iter2.adv();
  }

  return iter1.isDone() && iter2.isDone();
}


bool TemplateInfo::argumentsContainTypeVariables() const
{
  FOREACH_OBJLIST(STemplateArgument, arguments, iter) {
    STemplateArgument const *sta = iter.data();
    if (sta->kind == STemplateArgument::STA_TYPE) {
      if (sta->value.t->containsTypeVariables()) return true;
    }
    // FIX: do other kinds
  }
  return false;
}


bool TemplateInfo::argumentsContainVariables() const
{
  FOREACH_OBJLIST(STemplateArgument, arguments, iter) {
    if (iter.data()->containsVariables()) return true;
  }
  return false;
}


// You may ask why I don't implement isMutant() as simply "return
// isPrimary() || isPartialSpec() || isCompleteSpecOrInstantiation()".
// I don't because I want the assertions in the three other methods,
// as they are used directly and always in cases where there should
// never be a mutant, and I have to avoid circularity, therefore
// isMutant() does not depend on them.  I have also avoided references
// to the instantiations list, as that is somehow a different
// consideration from mutantness
bool TemplateInfo::isMutant() const {
  // primary
  if (params.isNotEmpty() && arguments.isEmpty()) return false;
  // partial specialization
  if (params.isNotEmpty() && argumentsContainVariables()) return false;
  // complete specialization / instantiation
  if (params.isEmpty() && !argumentsContainVariables()) return false;
  // mutant!
  xassert(params.isEmpty());
  xassert(instantiations.isEmpty());
  xassert(arguments.isNotEmpty());
  xassert(argumentsContainVariables());

  return true;
}


bool TemplateInfo::isPrimary() const {
  xassert(!isMutant());
  return params.isNotEmpty() && arguments.isEmpty();
}


bool TemplateInfo::isNotPrimary() const {
  if (isMutant()) return true;
  return !isPrimary();
}


bool TemplateInfo::isPartialSpec() const {
  xassert(!isMutant());
  bool ret = params.isNotEmpty() && argumentsContainVariables();
  if (ret) {
    xassert(instantiations.isEmpty());
  }
  return ret;
}


bool TemplateInfo::isCompleteSpecOrInstantiation() const {
  xassert(!isMutant());
  bool ret = params.isEmpty() && !argumentsContainVariables();
  if (ret) {
    xassert(instantiations.isEmpty());
  }
  return ret;
}


void TemplateInfo::gdb()
{
  debugPrint(0);
}


void TemplateInfo::debugPrint(int depth)
{
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "TemplateInfo";
  if (baseName) {
    cout << ", baseName: " << baseName;
  } else {
    cout << ", null base name";
  }
  cout << endl;
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "params:" << endl;
  SFOREACH_OBJLIST_NC(Variable, params, iter) {
    Variable *var = iter.data();
    cout << "\t'" << var->name << "' ";
    printf("%p ", var);
//      cout << "var->serialNumber " << var->serialNumber << endl;
//      cout << iter.data()->toString();
    cout << endl;
  }
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "instantiations:" << endl;
  SFOREACH_OBJLIST_NC(Variable, instantiations, iter) {
    Variable *var = iter.data();
    for (int i=0; i<depth; ++i) cout << "  ";
    cout << var->toString() << endl;
    var->templateInfo()->debugPrint(depth+1);
  }
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "arguments:" << endl;
  FOREACH_OBJLIST_NC(STemplateArgument, arguments, iter) {
    iter.data()->debugPrint(depth+1);
  }
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "TemplateInfo end" << endl;
}


// ------------------- STemplateArgument ---------------
STemplateArgument::STemplateArgument(STemplateArgument const &obj)
  : kind(obj.kind)
{
  #if 0     // old
    // take advantage of representation uniformity
    // dsw: Ugh, a hidden reinterpret_cast!
    // sm: not really.. it's just a bit-for-bit copy, with the
    // same interpretation to be applied to the source and
    // destination
    value.i = obj.value.i;
    value.v = obj.value.v;    // just in case ptrs and ints are diff't size
  #else
    // sm: ok, fine; rather than have two copy ctors, I'll just change
    // this one so there are no borderline language abuses...
    if (kind == STA_TYPE) {
      value.t = obj.value.t;
    }
    else if (kind == STA_INT) {
      value.i = obj.value.i;
    }
    else {
      value.v = obj.value.v;
    }
  #endif
}


STemplateArgument *STemplateArgument::shallowClone() const
{
  #if 0    // old; can we delete this now?
    STemplateArgument *ret = new STemplateArgument();
    switch (kind) {
    default:
    case STA_NONE:
      xfailure("STemplateArgument with illegal or undefined kind");
      break;
    case STA_TYPE:
      ret->setType(getType());
      break;
    case STA_INT:
      ret->setInt(getInt());
      break;
    case STA_REFERENCE:
      ret->setReference(getReference());
      break;
    case STA_POINTER:
      ret->setPointer(getPointer());
      break;
    case STA_MEMBER:
      ret->setMember(getMember());
      break;
    case STA_TEMPLATE:
      xfailure("STemplateArgument STA_MEMBER unimplemented");
      break;
    }
    return ret;
  #else
    return new STemplateArgument(*this);
  #endif
}


bool STemplateArgument::isObject() {
  switch (kind) {
  default:
    xfailure("illegal STemplateArgument kind"); break;

  case STA_TYPE:                // type argument
    return false; break;

  case STA_INT:                 // int or enum argument
  case STA_REFERENCE:           // reference to global object
  case STA_POINTER:             // pointer to global object
  case STA_MEMBER:              // pointer to class member
    return true; break;

  case STA_TEMPLATE:            // template argument (not implemented)
    return false; break;
  }
}


bool STemplateArgument::equals(STemplateArgument const *obj) const
{
  if (kind != obj->kind) {
    return false;
  }

  // At one point I tried making type arguments unequal if they had
  // different typedefs, but that does not work, because I need A<int>
  // to be the *same* type as A<my_int>, for example to do base class
  // constructor calls.

  switch (kind) {
    case STA_TYPE:     return value.t->equals(obj->value.t);
    case STA_INT:      return value.i == obj->value.i;
    default:           return value.v == obj->value.v;
  }
}


bool STemplateArgument::containsVariables() const
{
  if (kind == STemplateArgument::STA_TYPE) {
    if (value.t->containsVariables()) {
      return true;
    }
  } else if (kind == STemplateArgument::STA_REFERENCE) {
    // FIX: I am not at all sure that my interpretation of what
    // STemplateArgument::kind == STA_REFERENCE means; I think it
    // means it is a non-type non-template (object) variable in an
    // argument list
    return true;
  }
  // FIX: do other kinds
  return false;
}


string STemplateArgument::toString() const
{
  switch (kind) {
    default: xfailure("bad kind");
    case STA_NONE:      return string("STA_NONE");
    case STA_TYPE:      return value.t->toString();   // assume 'type' if no comment
    case STA_INT:       return stringc << "/*int*/ " << value.i;
    case STA_REFERENCE: return stringc << "/*ref*/ " << value.v->name;
    case STA_POINTER:   return stringc << "/*ptr*/ &" << value.v->name;
    case STA_MEMBER:    return stringc
      << "/*member*/ &" << value.v->scope->curCompound->name 
      << "::" << value.v->name;
    case STA_TEMPLATE:  return string("template (?)");
  }
}


void STemplateArgument::gdb()
{
  debugPrint(0);
}


void STemplateArgument::debugPrint(int depth)
{
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "STemplateArgument: " << toString() << endl;
}


SObjList<STemplateArgument> *cloneSArgs(SObjList<STemplateArgument> const &sargs)
{
  SObjList<STemplateArgument> *ret = new SObjList<STemplateArgument>();
  SFOREACH_OBJLIST(STemplateArgument, sargs, iter) {
    ret->append(iter.data()->shallowClone());
  }
  return ret;
}


string sargsToString(SObjList<STemplateArgument> const &list)
{
  stringBuilder sb;
  sb << "<";

  int ct=0;
  SFOREACH_OBJLIST(STemplateArgument, list, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter.data()->toString();
  }

  sb << ">";
  return sb;
}


// -------------------- ArrayType ------------------
void ArrayType::checkWellFormedness() const
{
  // you can't have an array of references
  xassert(!eltType->isReference());
}


bool ArrayType::innerEquals(ArrayType const *obj, EqFlags flags) const
{
  if (!( eltType->equals(obj->eltType, flags & EF_PROP) &&
         hasSize() == obj->hasSize() )) {
    return false;
  }

  if (hasSize()) {
    return size == obj->size;
  }
  else {
    return true;
  }
}


unsigned ArrayType::innerHashValue() const
{
  // similar to PointerType
  return eltType->innerHashValue() * HASH_KICK +
         size +
         T_ARRAY * TAG_KICK;
}


string ArrayType::leftString(bool /*innerParen*/) const
{
  return eltType->leftString();
}

string ArrayType::rightString(bool /*innerParen*/) const
{
  stringBuilder sb;

  if (hasSize()) {
    sb << "[" << size << "]";
  }
  else {
    sb << "[]";
  }

  sb << eltType->rightString();

  return sb;
}


int ArrayType::reprSize() const
{
  if (!hasSize()) {
    throw_XReprSize();
  }

  return eltType->reprSize() * size;
}


bool ArrayType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         eltType->anyCtorSatisfies(pred);
}


// ---------------- PointerToMemberType ---------------
PointerToMemberType::PointerToMemberType(NamedAtomicType *inClassNAT0, CVFlags c, Type *a)
  : inClassNAT(inClassNAT0), cv(c), atType(a)
{
  // 'inClassNAT' should always be a compound or a type variable
  xassert(inClassNAT->isCompoundType() ||
          inClassNAT->isTypeVariable());

  // cannot have pointer to reference type
  xassert(!a->isReference());
  
  // there are some other semantic restrictions, but I let the
  // type checker enforce them
}


bool PointerToMemberType::innerEquals(PointerToMemberType const *obj, EqFlags flags) const
{
  return inClassNAT == obj->inClassNAT &&
         ((flags & EF_OK_DIFFERENT_CV) || (cv == obj->cv)) &&
         atType->equals(obj->atType, flags & EF_PTR_PROP);
}


CompoundType *PointerToMemberType::inClass() const {
  if (inClassNAT->isCompoundType()) {
    return inClassNAT->asCompoundType();
  }
  xfailure("Request for the inClass member of a PointerToMemberType "
           "as if it were a CompoundType which it is not in this case.");
}


unsigned PointerToMemberType::innerHashValue() const
{
  return atType->innerHashValue() * HASH_KICK +
         cvHash(cv) +
         T_POINTERTOMEMBER * TAG_KICK +
         (unsigned)inClass();   // we'll see...
}


string PointerToMemberType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  s << " ";
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << inClassNAT->name << "::*";
  s << cvToString(cv);
  return s;
}

string PointerToMemberType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}


int PointerToMemberType::reprSize() const
{
  // a typical value .. (architecture-dependent)
  return 4;
}


bool PointerToMemberType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags PointerToMemberType::getCVFlags() const
{
  return cv;
}


// ---------------------- toMLString ---------------------
// print out a type as an ML-style string

//  Atomic
string SimpleType::toMLString() const
{
  return simpleTypeName(type);
}

string CompoundType::toMLString() const
{
  stringBuilder sb;
      
//    bool hasParams = templateInfo && templateInfo->params.isNotEmpty();
//    if (hasParams) {
  TemplateInfo *tinfo = templateInfo();
  if (tinfo) {
    sb << tinfo->paramsToMLString();
  }

//    if (!templateInfo || hasParams) {   
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
  sb << keywordName(keyword) << "-";
//    }

  if (name) {
    sb << "'" << name << "'";
  } else {
    sb << "*anonymous*";
  }

  // template arguments are now in the name
  //if (templateInfo && templateInfo->specialArguments) {
  //  sb << "<" << templateInfo->specialArgumentsRepr << ">";
  //}
   
  return sb;
}

string EnumType::toMLString() const
{
  stringBuilder sb;
  sb << "enum-";
  if (name) {
    sb << "'" << name << "'";
  } else {
    sb << "*anonymous*";
  }
  return sb;
}

string TypeVariable::toMLString() const
{
  return stringc << "typevar-'" << string(name) << "'";
}

//  Constructed

// I don't like #if-s everywhere, and Steve Fink agrees with me.
void BaseType::putSerialNo(stringBuilder &sb) const
{
  #if USE_SERIAL_NUMBERS
    sb << printSerialNo("t", serialNumber, "-");
  #endif
}

string CVAtomicType::toMLString() const
{
  stringBuilder sb;
  sb << cvToString(cv) << " ";
  putSerialNo(sb);
  sb << atomic->toMLString();
  return sb;
}

string PointerType::toMLString() const
{
  stringBuilder sb;
  if (cv) {
    sb << cvToString(cv) << " ";
  }
  putSerialNo(sb);
  sb << "ptr(" << atType->toMLString() << ")";
  return sb;
}

string ReferenceType::toMLString() const
{
  stringBuilder sb;
  putSerialNo(sb);
  sb << "ref(" << atType->toMLString() << ")";
  return sb;
}

string FunctionType::toMLString() const
{
  stringBuilder sb;
  // FIX: FUNC TEMPLATE LOSS
//    if (templateInfo) {
//      sb << templateInfo->paramsToMLString();
//    }
  putSerialNo(sb);
  if (flags & FF_CTOR) {
    sb << "ctor-";
  }
  sb << "fun";

  sb << "(";

  sb << "retType: " << retType->toMLString();
  // arg, you can't do this due to the way Scott handles retVal
//    if (retVal) {
//      sb << ", " << retVal->toMLString();
//    }

  int ct=0;
//    bool firstTime = true;
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *v = iter.data();
    ct++;
//      if (firstTime) {
//        firstTime = false;
//      } else {
    sb << ", ";                 // retType is first
//      }
//      if (isMethod() && ct==1) {
//        // print "this" param
//        sb << "this: " << v->type->toMLString();
//        continue;
//      }
    sb << v->toMLString();
//      sb << "'" << v->name << "'->" << v->type->toMLString();
  }
  sb << ")";
  return sb;
}

string ArrayType::toMLString() const
{
  stringBuilder sb;
  putSerialNo(sb);
  sb << "array(";
  sb << "size:";
  if (hasSize()) {
    sb << size;
  } else {
    sb << "unspecified";
  }
  sb << ", ";
  sb << eltType->toMLString();
  sb << ")";
  return sb;
}

string PointerToMemberType::toMLString() const
{
  stringBuilder sb;
  if (cv) {
    sb << cvToString(cv) << " ";
  }
  putSerialNo(sb);
  sb << "ptm(";
  sb << inClassNAT->name;
  sb << ", " << atType->toMLString();
  sb << ")";
  return sb;
}

string TemplateParams::paramsToMLString() const
{
  stringBuilder sb;
  sb << "template <";
#if 0
  int ct=0;
  // FIX: incomplete
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *p = iter.data();
    if (ct++ > 0) {
      sb << ", ";
    }

    // FIX: is there any difference here?
    // NOTE: use var->toMLString()
    if (p->type->isTypeVariable()) {
//        sb << "var(name:" << p->name << ", " << v->type->toMLString() << ")";
    }
    else {
      // non-type parameter
//        sb << p->toStringAsParameter();
    }
  }
#endif
  sb << ">";
  return sb;
}


// ---------------------- TypeFactory ---------------------
CompoundType *TypeFactory::makeCompoundType
  (CompoundType::Keyword keyword, StringRef name)
{
  return new CompoundType(keyword, name);
}


Type *TypeFactory::cloneType(Type *src)
{
  switch (src->getTag()) {
    default: xfailure("bad type tag");
    case Type::T_ATOMIC:    return cloneCVAtomicType(src->asCVAtomicType());
    case Type::T_POINTER:   return clonePointerType(src->asPointerType());
    case Type::T_REFERENCE: return cloneReferenceType(src->asReferenceType());
    case Type::T_FUNCTION:  return cloneFunctionType(src->asFunctionType());
    case Type::T_ARRAY:     return cloneArrayType(src->asArrayType());
    case Type::T_POINTERTOMEMBER: return clonePointerToMemberType(src->asPointerToMemberType());
  }
}


Type *TypeFactory::setCVQualifiers(SourceLoc loc, CVFlags cv, Type *baseType,
                                   TypeSpecifier *)
{
  if (baseType->isError()) {
    return baseType;
  }

  if (cv == baseType->getCVFlags()) {
    // keep what we've got
    return baseType;
  }

  // the idea is we're trying to apply 'cv' to 'baseType'; for
  // example, we could have gotten baseType like
  //   typedef unsigned char byte;     // baseType == unsigned char
  // and want to apply const:
  //   byte const b;                   // cv = CV_CONST
  // yielding final type
  //   unsigned char const             // return value from this fn

  switch (baseType->getTag()) {
    case Type::T_ATOMIC: {
      CVAtomicType *atomic = baseType->asCVAtomicType();

      // make a new CVAtomicType with the same AtomicType as 'baseType',
      // but with the new flags
      return makeCVAtomicType(loc, atomic->atomic, cv);
    }

    case Type::T_POINTER: {
      PointerType *ptr = baseType->asPointerType();
      return makePointerType(loc, cv, ptr->atType);
    }

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType *ptm = baseType->asPointerToMemberType();
      return makePointerToMemberType(loc, ptm->inClassNAT, cv, ptm->atType);
    }

    default:    // silence warning
    case Type::T_REFERENCE:
    case Type::T_FUNCTION:
    case Type::T_ARRAY:
      // can't apply CV to either of these, and since baseType's
      // original cv flags must have been CV_NONE, then 'cv' must
      // not be CV_NONE
      return NULL;
  }
}


Type *TypeFactory::applyCVToType(SourceLoc loc, CVFlags cv, Type *baseType,
                                 TypeSpecifier *syntax)
{
  CVFlags now = baseType->getCVFlags();
  if (now | cv == now) {
    // no change, 'cv' already contained in the existing flags
    return baseType;
  }
  else {
    // change to the union; setCVQualifiers will take care of catching
    // inappropriate application (e.g. 'const' to a reference)
    return setCVQualifiers(loc, now | cv, baseType, syntax);
  }
}


Type *TypeFactory::syntaxPointerType(SourceLoc loc,
  bool isPtr, CVFlags cv, Type *type, D_pointer *)
{
  if (isPtr) {
    return makePointerType(loc, cv, type);
  }
  else {
    return makeReferenceType(loc, type);
  }
}


FunctionType *TypeFactory::syntaxFunctionType(SourceLoc loc,
  Type *retType, D_func *syntax, TranslationUnit *tunit)
{
  return makeFunctionType(loc, retType);
}


PointerToMemberType *TypeFactory::syntaxPointerToMemberType(SourceLoc loc,
  NamedAtomicType *inClassNAT, CVFlags cv, Type *atType, D_ptrToMember *syntax)
{
  return makePointerToMemberType(loc, inClassNAT, cv, atType);
}


Type *TypeFactory::makeTypeOf_receiver(SourceLoc loc,
  NamedAtomicType *classType, CVFlags cv, D_func *syntax)
{
  CVAtomicType *at = makeCVAtomicType(loc, classType, cv);
  return makeReferenceType(loc, at);
}


FunctionType *TypeFactory::makeSimilarFunctionType(SourceLoc loc,
  Type *retType, FunctionType *similar)
{
  FunctionType *ret = 
    makeFunctionType(loc, retType);
  ret->flags = similar->flags & ~FF_METHOD;     // isMethod is like a parameter
  if (similar->exnSpec) {
    ret->exnSpec = new FunctionType::ExnSpec(*similar->exnSpec);
  }
  return ret;
}


CVAtomicType *TypeFactory::getSimpleType(SourceLoc loc, 
  SimpleTypeId st, CVFlags cv)
{
  xassert((unsigned)st < (unsigned)NUM_SIMPLE_TYPES);
  return makeCVAtomicType(loc, &SimpleType::fixed[st], cv);
}


ArrayType *TypeFactory::setArraySize(SourceLoc loc, ArrayType *type, int size)
{
  return makeArrayType(loc, type->eltType, size);
}


// -------------------- BasicTypeFactory ----------------------
// this is for when I split Type from Type_Q
CVAtomicType BasicTypeFactory::unqualifiedSimple[NUM_SIMPLE_TYPES] = {
  #define CVAT(id) \
    CVAtomicType(&SimpleType::fixed[id], CV_NONE),
  CVAT(ST_CHAR)
  CVAT(ST_UNSIGNED_CHAR)
  CVAT(ST_SIGNED_CHAR)
  CVAT(ST_BOOL)
  CVAT(ST_INT)
  CVAT(ST_UNSIGNED_INT)
  CVAT(ST_LONG_INT)
  CVAT(ST_UNSIGNED_LONG_INT)
  CVAT(ST_LONG_LONG)
  CVAT(ST_UNSIGNED_LONG_LONG)
  CVAT(ST_SHORT_INT)
  CVAT(ST_UNSIGNED_SHORT_INT)
  CVAT(ST_WCHAR_T)
  CVAT(ST_FLOAT)
  CVAT(ST_DOUBLE)
  CVAT(ST_LONG_DOUBLE)
  CVAT(ST_VOID)

  CVAT(ST_ELLIPSIS)
  CVAT(ST_CDTOR)
  CVAT(ST_ERROR)
  CVAT(ST_DEPENDENT)

  CVAT(ST_PROMOTED_INTEGRAL)
  CVAT(ST_PROMOTED_ARITHMETIC)
  CVAT(ST_INTEGRAL)
  CVAT(ST_ARITHMETIC)
  CVAT(ST_ARITHMETIC_NON_BOOL)
  CVAT(ST_ANY_OBJ_TYPE)
  CVAT(ST_ANY_NON_VOID)
  CVAT(ST_ANY_TYPE)
  #undef CVAT
};


CVAtomicType *BasicTypeFactory::makeCVAtomicType(SourceLoc,
  AtomicType *atomic, CVFlags cv)
{
  if (cv==CV_NONE && atomic->isSimpleType()) {
    // since these are very common, and ordinary Types are immutable,
    // share them
    SimpleType *st = atomic->asSimpleType();
    xassert((unsigned)(st->type) < (unsigned)NUM_SIMPLE_TYPES);
    return &(unqualifiedSimple[st->type]);
  }

  return new CVAtomicType(atomic, cv);
}


PointerType *BasicTypeFactory::makePointerType(SourceLoc,
  CVFlags cv, Type *atType)
{
  return new PointerType(cv, atType);
}


Type *BasicTypeFactory::makeReferenceType(SourceLoc,
  Type *atType)
{
  return new ReferenceType(atType);
}


FunctionType *BasicTypeFactory::makeFunctionType(SourceLoc,
  Type *retType)
{
  return new FunctionType(retType);
}


ArrayType *BasicTypeFactory::makeArrayType(SourceLoc,
  Type *eltType, int size)
{
  return new ArrayType(eltType, size);
}


PointerToMemberType *BasicTypeFactory::makePointerToMemberType(SourceLoc,
  NamedAtomicType *inClassNAT, CVFlags cv, Type *atType)
{
  return new PointerToMemberType(inClassNAT, cv, atType);
}


// Types are immutable, so cloning is pointless
CVAtomicType *BasicTypeFactory::cloneCVAtomicType(CVAtomicType *src)
  { return src; }
PointerType *BasicTypeFactory::clonePointerType(PointerType *src)
  { return src; }
ReferenceType *BasicTypeFactory::cloneReferenceType(ReferenceType *src)
  { return src; }
FunctionType *BasicTypeFactory::cloneFunctionType(FunctionType *src)
  { return src; }
ArrayType *BasicTypeFactory::cloneArrayType(ArrayType *src)
  { return src; }
PointerToMemberType *BasicTypeFactory::clonePointerToMemberType(PointerToMemberType *src)
  { return src; }


Variable *BasicTypeFactory::makeVariable(
  SourceLoc L, StringRef n, Type *t, DeclFlags f, TranslationUnit *)
{
  // the TranslationUnit parameter is ignored by default; it is passed
  // only for the possible benefit of an extension analysis
  Variable *var = new Variable(L, n, t, f);
  return var;
}

Variable *BasicTypeFactory::cloneVariable(Variable *src)
{
  // immutable => don't clone
  return src;
}


// -------------------- XReprSize -------------------
XReprSize::XReprSize()
  : xBase("reprSize of a sizeless array")
{}

XReprSize::XReprSize(XReprSize const &obj)
  : xBase(obj)
{}

XReprSize::~XReprSize()
{}


void throw_XReprSize()
{
  XReprSize x;
  THROW(x);
}


// --------------- debugging ---------------
char *type_toString(Type const *t)
{
  // defined in smbase/strutil.cc
  return copyToStaticBuffer(t->toString());
}


// EOF
