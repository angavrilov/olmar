// cc_type.cc            see license.txt for copyright and terms of use
// code for cc_type.h

#include "cc_type.h"    // this module
#include "trace.h"      // tracingSys
#include "variable.h"   // Variable
#include "strutil.h"    // copyToStaticBuffer
#include "sobjset.h"    // SObjSet
#include "hashtbl.h"    // lcprngTwoSteps
#include "matchtype.h"  // MatchTypes
#include "asthelp.h"    // ind

// TemplateInfo, etc... it would be sort of nice if this module didn't
// have to #include template.h, since most of what it does is
// independent of that, but there are enough little dependencies that
// for now I just live with it.
#include "template.h"

// Do *not* add a dependency on cc.ast or cc_env.  cc_type is about
// the intrinsic properties of types, independent of any particular
// syntax for denoting them.  (toString() uses C's syntax, but that's
// just for debugging.)

#include <stdlib.h>     // getenv


// FIX: for debugging only; remove
bool global_mayUseTypeAndVarToCString = true;


// ------------------- TypeVisitor ----------------
bool TypeVisitor::visitType(Type *obj)
  { return true; }
void TypeVisitor::postvisitType(Type *obj)
  {  }
bool TypeVisitor::visitAtomicType(AtomicType *obj)
  { return true; }
void TypeVisitor::postvisitAtomicType(AtomicType *obj)
  {  }
bool TypeVisitor::visitSTemplateArgument(STemplateArgument *obj)
  { return true; }
void TypeVisitor::postvisitSTemplateArgument(STemplateArgument *obj)
  {  }
bool TypeVisitor::visitExpression(Expression *obj)
  {
    // if this ever fires, at the same location be sure to put a
    // postvisitExpression()
    xfailure("wanted to find out if this is ever called; I can't find it if it is");
    return true;
  }
void TypeVisitor::postvisitExpression(Expression *obj)
  {  }


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
DOWNCAST_IMPL(AtomicType, PseudoInstantiation)
DOWNCAST_IMPL(AtomicType, DependentQType)


void AtomicType::gdb() const
{
  cout << toString() << endl;
}


string AtomicType::toString() const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  // The design of this type system is such that this function is
  // supposed to be simple.  If you are inclined to add any special
  // cases, you're probably misusing the interface.
  //
  // For example, if you want to weaken the check for template-related
  // reason, consider using MatchTypes instead.

  // experiment: Disallow non-concrete types
  //
  // This is mostly a good idea, and I think I found and fixed several
  // real problems by turning this on.  However, there's a call to
  // 'equals' in CompoundType::addLocalConversionOp where a
  // non-concrete type can appear as an argument (t0225.cc), but it's
  // not clear that isomorphic matching is right.  I'm not really sure
  // how things are supposed to work with conversion operator return
  // types, etc.  So I'm just going to turn this check off, and let
  // the dogs sleep for now.
  #if 0
  if (!( this->isConcrete() && obj->isConcrete() )) {
    xfailure("AtomicType::equals called with a non-concrete argument");
  }
  #endif // 0

  if (this == obj) {
    return true;
  }
  
  if (getTag() != obj->getTag()) {
    return false;
  }
  
  // for the template-related types, we have a degree of structural
  // equality; each type knows how to compare itself
  switch (getTag()) {
    default: xfailure("bad tag");

    case T_SIMPLE:
    case T_COMPOUND:
    case T_ENUM:  
      // non-template-related type, physical equality only
      return false;

    #define C(tag,type) \
      case tag: return ((type const*)this)->innerEquals((type const*)obj);
    C(T_TYPEVAR, TypeVariable)
    C(T_PSEUDOINSTANTIATION, PseudoInstantiation)
    C(T_DEPENDENTQTYPE, DependentQType)
    #undef C
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
  SimpleType(ST_FLOAT_COMPLEX),
  SimpleType(ST_DOUBLE_COMPLEX),
  SimpleType(ST_LONG_DOUBLE_COMPLEX),
  SimpleType(ST_FLOAT_IMAGINARY),
  SimpleType(ST_DOUBLE_IMAGINARY),
  SimpleType(ST_LONG_DOUBLE_IMAGINARY),
  SimpleType(ST_VOID),

  SimpleType(ST_ELLIPSIS),
  SimpleType(ST_CDTOR),
  SimpleType(ST_ERROR),
  SimpleType(ST_DEPENDENT),
  SimpleType(ST_IMPLINT),
  SimpleType(ST_NOTFOUND),

  SimpleType(ST_PROMOTED_INTEGRAL),
  SimpleType(ST_PROMOTED_ARITHMETIC),
  SimpleType(ST_INTEGRAL),
  SimpleType(ST_ARITHMETIC),
  SimpleType(ST_ARITHMETIC_NON_BOOL),
  SimpleType(ST_ANY_OBJ_TYPE),
  SimpleType(ST_ANY_NON_VOID),
  SimpleType(ST_ANY_TYPE),
  
  SimpleType(ST_PRET_STRIP_REF),
  SimpleType(ST_PRET_PTM),
  SimpleType(ST_PRET_ARITH_CONV),
  SimpleType(ST_PRET_FIRST),
  SimpleType(ST_PRET_FIRST_PTR2REF),
  SimpleType(ST_PRET_SECOND),
  SimpleType(ST_PRET_SECOND_PTR2REF),
};

string SimpleType::toCString() const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  return simpleTypeName(type);
}


int SimpleType::reprSize() const
{
  return simpleTypeReprSize(type);
}


void SimpleType::traverse(TypeVisitor &vis)
{
  vis.visitAtomicType(this);
  vis.postvisitAtomicType(this);
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
    syntax(NULL),
    parameterizingScope(NULL),
    selfType(NULL)
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
         tinfo->hasParameters();
}


bool CompoundType::isInstantiation() const
{
  TemplateInfo *tinfo = templateInfo();
  return tinfo != NULL &&
         tinfo->isInstantiation();
}


Variable *CompoundType::getTypedefVar() const
{
  xassert(typedefVar);
  xassert(typedefVar->type);
  if (typedefVar->type->isError()) {
    // this is the error compound type
  }
  else {
    xassert(typedefVar->type->asCompoundTypeC() == this);
  }
  return typedefVar;
}


TemplateInfo *CompoundType::templateInfo() const
{
  return getTypedefVar()->templateInfo();
}

void CompoundType::setTemplateInfo(TemplateInfo *templInfo0)
{
  if (templInfo0) {
    getTypedefVar()->setTemplateInfo(templInfo0);
  }
}


bool CompoundType::hasVirtualFns() const
{
  // TODO: this fails to consider members inherited from base classes
  // ...
  // UPDATE dsw: I think the code in Declarator::mid_tcheck() is
  // supposed to push the virtuality down at least for those function
  // members that implicitly inherit their virtuality from the fact
  // that they override a virtual member in their superclass.

  for (StringRefMap<Variable>::Iter iter(getVariableIter());
       !iter.isDone(); iter.adv()) {
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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

  sb << (instName? instName : "/*anonymous*/");

  // template arguments are now in the name
  // 4/22/04: they were removed from the name a long time ago;
  //          but I'm just now uncommenting this code
  // 8/03/04: restored the original purpose of 'instName', so
  //          once again that is name+args, so this code is not needed
  //if (tinfo && tinfo->arguments.isNotEmpty()) {
  //  sb << sargsToString(tinfo->arguments);
  //}

  return sb;
}


int CompoundType::reprSize() const
{
  int total = 0;
  for (StringRefMap<Variable>::Iter iter(getVariableIter());
       !iter.isDone(); iter.adv()) {
    Variable *v = iter.value();
    // count nonstatic data members
    if (!v->type->isFunctionType() &&
        !v->hasFlag(DF_TYPEDEF) &&
        !v->hasFlag(DF_STATIC)) {
      int membSize = 0;
      if (v->type->isArrayType() &&
          v->type->asArrayType()->size == ArrayType::NO_SIZE) {
        // if the type checker let this in, we must be in a mode that
        // allows "open arrays", in which case the computed size will
        // be zero
      }
      else {
        membSize = v->type->reprSize();
      }
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


void CompoundType::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }

  if (isTemplate()) {
    templateInfo()->traverseArguments(vis);
  }

  vis.postvisitAtomicType(this);
}


int CompoundType::numFields() const
{
  int ct = 0;
  for (StringRefMap<Variable>::Iter iter(getVariableIter());
       !iter.isDone(); iter.adv()) {
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
      !(v->flags & (DF_STATIC | DF_TYPEDEF | DF_ENUMERATOR | DF_USING_ALIAS))) {
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
  // (this is the one function allowed to do this)
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

  // remove duplicates (in/t0483.cc); if there is an ambiguity because
  // of inheriting something multiple times, it should be detected when
  // we go to actually *use* the conversion operator
  conversionOperators.removeDuplicatesAsPointerMultiset();

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


// ---------------- EnumType ------------------
EnumType::~EnumType()
{}


string EnumType::toCString() const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  return stringc << "enum " << (name? name : "/*anonymous*/");
}


int EnumType::reprSize() const
{
  // this is the usual choice
  return simpleTypeReprSize(ST_INT);
}


void EnumType::traverse(TypeVisitor &vis)
{
  vis.visitAtomicType(this);
  vis.postvisitAtomicType(this);
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


// -------------------- TypePred ----------------------
bool StatelessTypePred::operator() (Type const *t)
{
  return f(t);
}


// -------------------- BaseType ----------------------
ALLOC_STATS_DEFINE(BaseType)

bool BaseType::printAsML = false;


BaseType::BaseType()
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  if (printAsML) {
    return toMLString();
  }
  else {
    return toCString();
  }
}


string BaseType::toCString() const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  if (isCVAtomicType()) {
    // special case a single atomic type, so as to avoid
    // printing an extra space
    CVAtomicType const *cvatomic = asCVAtomicTypeC();
    return stringc
      << cvatomic->atomic->toCString()
      << cvToString(cvatomic->cv);
  }
  else {
    return stringc
      << leftString()
      << rightString();
  }
}

string BaseType::toCString(char const *name) const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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

bool BaseType::isStringType() const
{
  return isArrayType() &&
    (asArrayTypeC()->eltType->isSimpleCharType() ||
     asArrayTypeC()->eltType->isSimpleWChar_tType());
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

bool BaseType::isGeneralizedDependent() const
{
  if (isSimple(ST_DEPENDENT) ||
      isTypeVariable() ||
      isPseudoInstantiation() ||
      isDependentQType()) {
    return true;
  }

  // 2005-05-06: I removed the following because I think it is wrong.
  // The name 'A' (if found in, say, the global scope) is not dependent,
  // though 'A<T>' might be (if T is an uninst template param).  I think
  // in/d0113.cc was just invalid, and I fixed it.
  #if 0     // removed
  if (isCompoundType()) {
    // 10/09/04: (in/d0113.cc) I want to regard A<T>::B, where B is an
    // inner class of template class A, as being dependent.  For that
    // matter, A itself should be regarded as dependent.  So if 'ct'
    // has template parameters (including inherited), say it is
    // dependent.
    TemplateInfo *ti = asCompoundTypeC()->templateInfo();
    if (ti && ti->hasMainOrInheritedParameters()) {
      return true;
    }
  }
  #endif // 0

  return false;
}

static bool cGD_helper(Type const *t)
{
  return t->isGeneralizedDependent();
}

// What is the intended difference in usage between
// 'isGeneralizedDependent' and 'containsGeneralizedDependent'?
// Mostly, the latter includes types that cannot be classes even when
// template args are supplied, whereas the former includes only types
// that *might* be classes when targs are supplied.  (TODO: write up
// the entire design of dependent type representation)
bool BaseType::containsGeneralizedDependent() const
{
  return anyCtorSatisfiesF(cGD_helper);
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

PseudoInstantiation const *BaseType::asPseudoInstantiationC() const
{
  return asCVAtomicTypeC()->atomic->asPseudoInstantiationC();
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

bool BaseType::isReferenceToConst() const {
  return isReferenceType() && asReferenceTypeC()->atType->isConst();
}

bool BaseType::isPointerOrArrayRValueType() const {
  return asRvalC()->isPointerType() || asRvalC()->isArrayType();
}

// FIX: this is silly; it should be made into a virtual dispatch
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


TypeVariable const *BaseType::asTypeVariableC() const
{
  return asCVAtomicTypeC()->atomic->asTypeVariableC();
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
  return anyCtorSatisfiesF(typeIsError);
}


bool typeHasTypeVariable(Type const *t)
{
  return t->isTypeVariable() ||
         (t->isCompoundType() &&
          t->asCompoundTypeC()->isTemplate()) ||
         t->isPseudoInstantiation();
}

bool BaseType::containsTypeVariables() const
{
  return anyCtorSatisfiesF(typeHasTypeVariable);
}


bool atomicTypeHasVariable(AtomicType const *t)
{
  if (t->isTypeVariable() ||
      t->isPseudoInstantiation() ||
      t->isDependentQType()) {
    return true;
  }

  if (t->isCompoundType() && t->asCompoundTypeC()->isTemplate()) {
    return t->asCompoundTypeC()->templateInfo()->argumentsContainVariables();
  }


  return false;
}

// TODO: sm: I think it's wrong to have both 'hasTypeVariable' and
// 'hasVariable', since I think any use of the former actually wants
// to be a use of the latter.  Moreover, assuming their functionality
// collapses, I prefer the former *name* since it's more suggestive of
// use in templates (even if it would then be slightly inaccurate).
bool hasVariable(Type const *t)
{
  if (t->isCVAtomicType()) {
    return atomicTypeHasVariable(t->asCVAtomicTypeC()->atomic);
  }
  else if (t->isPointerToMemberType()) {
    return atomicTypeHasVariable(t->asPointerToMemberTypeC()->inClassNAT);
  }

  return false;
}

bool BaseType::containsVariables() const
{
  return anyCtorSatisfiesF(hasVariable);
}


string toString(Type *t)
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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


void CVAtomicType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  atomic->traverse(vis);

  vis.postvisitType(this);
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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


void PointerType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  atType->traverse(vis);

  vis.postvisitType(this);
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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


void ReferenceType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  atType->traverse(vis);

  vis.postvisitType(this);
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

  if ((this->flags | obj->flags) & FF_NO_PARAM_INFO) {
    // at least one of the types does not have parameter info,
    // so no further comparison is possible
    return true;
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

  // 2005-05-27: Pretend EF_IGNORE_PARAM_CV is always set.
  if (true /*flags & EF_IGNORE_PARAM_CV*/) {
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
  setFlag(FF_METHOD);
}


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

NamedAtomicType *FunctionType::getNATOfMember()
{
  return getReceiver()->type->asReferenceType()->atType->asNamedAtomicType();
}


string FunctionType::leftString(bool innerParen) const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  // I split this into two pieces because the Cqual++ concrete
  // syntax puts $tainted into the middle of my rightString,
  // since it's following the placement of 'const' and 'volatile'
  return stringc
    << rightStringUpToQualifiers(innerParen)
    << rightStringQualifiers(getReceiverCV())
    << rightStringAfterQualifiers();
}

string FunctionType::rightStringUpToQualifiers(bool innerParen) const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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

STATICDEF string FunctionType::rightStringQualifiers(CVFlags cv)
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  if (cv) {
    return stringc << " " << ::toString(cv);
  }
  else {
    return "";
  }
}

string FunctionType::rightStringAfterQualifiers() const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  stringBuilder sb;

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
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
}


string FunctionType::toString_withCV(CVFlags cv) const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  return stringc
    << leftString(true /*innerParen*/)
    << rightStringUpToQualifiers(true /*innerParen*/)
    << rightStringQualifiers(cv)
    << rightStringAfterQualifiers();
}


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


void FunctionType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  retType->traverse(vis);

  SFOREACH_OBJLIST(Variable, params, iter) {
    // I am choosing not to traverse into any of the other fields of
    // the parameters, including default args.  For the application I
    // have in mind right now (matching definitions to declarations),
    // I do not need or want anything other than the parameter type.
    // So, if this code is later extended to traverse into default
    // args, there should be a flag to control that, and it should
    // default to off (or change the existing call sites).
    iter.data()->type->traverse(vis);
  }

  // similarly, I don't want traversal into exception specs right now

  vis.postvisitType(this);
}


// -------------------- ArrayType ------------------
void ArrayType::checkWellFormedness() const
{
  // you can't have an array of references
  xassert(!eltType->isReference());
}


bool ArrayType::innerEquals(ArrayType const *obj, EqFlags flags) const
{       
  // what flags to propagate?
  EqFlags propFlags = (flags & EF_PROP);

  if (flags & EF_IGNORE_ELT_CV) {
    if (eltType->isArrayType()) {
      // propagate the ignore-elt down through as many ArrayTypes
      // as there are
      propFlags |= EF_IGNORE_ELT_CV;
    }
    else {
      // the next guy is the element type, ignore *his* cv only
      propFlags |= EF_IGNORE_TOP_CV;
    }
  }

  if (!( eltType->equals(obj->eltType, propFlags) &&
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
  return eltType->leftString();
}

string ArrayType::rightString(bool /*innerParen*/) const
{
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
    throw_XReprSize(size == DYN_SIZE /*isDynamic*/);
  }

  return eltType->reprSize() * size;
}


bool ArrayType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         eltType->anyCtorSatisfies(pred);
}



void ArrayType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  eltType->traverse(vis);

  vis.postvisitType(this);
}


// ---------------- PointerToMemberType ---------------
PointerToMemberType::PointerToMemberType(NamedAtomicType *inClassNAT0, CVFlags c, Type *a)
  : inClassNAT(inClassNAT0), cv(c), atType(a)
{
  // 'inClassNAT' should always be a compound or something dependent
  // on a type variable; basically, it should just not be an enum,
  // given that it is a named atomic type
  xassert(!inClassNAT->isEnumType());

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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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
  if (!global_mayUseTypeAndVarToCString) xfailure("suspended during TypePrinterC::print");
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


void PointerToMemberType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  inClassNAT->traverse(vis);
  atType->traverse(vis);

  vis.postvisitType(this);
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

string PseudoInstantiation::toMLString() const
{
  return stringc << "psuedoinstantiation-'" << primary->name << "'"
                 << sargsToString(args);      // sm: ?
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
  // arg, you can't do this due to the way Scott handles retVar
//    if (retVar) {
//      sb << ", " << retVar->toMLString();
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


Type *TypeFactory::shallowCloneType(Type *baseType)
{
  switch (baseType->getTag()) {
    default:
      xfailure("bad tag");
      break;

    case Type::T_ATOMIC: {
      CVAtomicType *atomic = baseType->asCVAtomicType();

      // make a new CVAtomicType with the same AtomicType as 'baseType',
      // but with the new flags
      return makeCVAtomicType(atomic->atomic, atomic->cv);
    }

    case Type::T_POINTER: {
      PointerType *ptr = baseType->asPointerType();
      return makePointerType(ptr->cv, ptr->atType);
    }

    case Type::T_REFERENCE: {
      ReferenceType *ref = baseType->asReferenceType();
      return makeReferenceType(ref->atType);
    }

    case Type::T_FUNCTION: {
      FunctionType *ft = baseType->asFunctionType();
      FunctionType *ret = makeFunctionType(ft->retType);
      ret->flags = ft->flags;
      SFOREACH_OBJLIST_NC(Variable, ft->params, paramiter) {
        ret->params.append(paramiter.data());
      }
      ret->exnSpec = ft->exnSpec;
      return ret;
    }

    case Type::T_ARRAY: {
      ArrayType *arr = baseType->asArrayType();
      return makeArrayType(arr->eltType, arr->size);
    }

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType *ptm = baseType->asPointerToMemberType();
      return makePointerToMemberType(ptm->inClassNAT, ptm->cv, ptm->atType);
    }
  }
}


// the idea is we're trying to apply 'cv' to 'baseType'; for
// example, we could have gotten baseType like
//   typedef unsigned char byte;     // baseType == unsigned char
// and want to apply const:
//   byte const b;                   // cv = CV_CONST
// yielding final type
//   unsigned char const             // return value from this fn
Type *TypeFactory::setQualifiers(SourceLoc loc, CVFlags cv, Type *baseType,
                                 TypeSpecifier *)
{
  if (baseType->isError()) {
    return baseType;
  }

  if (cv == baseType->getCVFlags()) {
    // keep what we've got
    return baseType;
  }

  Type *ret = NULL;

  if (baseType->isCVAtomicType()) {
    ret = shallowCloneType(baseType);
    ret->asCVAtomicType()->cv = cv;
  } else if (baseType->isPointerType()) {
    ret = shallowCloneType(baseType);
    ret->asPointerType()->cv = cv;
  } else if (baseType->isPointerToMemberType()) {
    ret = shallowCloneType(baseType);
    ret->asPointerToMemberType()->cv = cv;
  } else {
    // anything else cannot have a cv applied to it anyway; the NULL
    // will result in an error message in the client
    ret = NULL;
  }

  return ret;
}


Type *TypeFactory::applyCVToType(SourceLoc loc, CVFlags cv, Type *baseType,
                                 TypeSpecifier *syntax)
{
  CVFlags now = baseType->getCVFlags();
  if (now | cv == now) {
    // no change, 'cv' already contained in the existing flags
    return baseType;
  }
  else if (baseType->isArrayType()) {
    // 8.3.4 para 1: apply cv to the element type
    ArrayType *at = baseType->asArrayType();
    return makeArrayType(applyCVToType(loc, cv, at->eltType, NULL /*syntax*/),
                         at->size);
  }
  else {
    // change to the union; setQualifiers will take care of catching
    // inappropriate application (e.g. 'const' to a reference)
    return setQualifiers(loc, now | cv, baseType, syntax);
  }
}


Type *TypeFactory::syntaxPointerType(SourceLoc loc,
  CVFlags cv, Type *type, D_pointer *)
{
  return makePointerType(cv, type);
}

Type *TypeFactory::syntaxReferenceType(SourceLoc loc,
  Type *type, D_reference *)
{
  return makeReferenceType(type);
}


FunctionType *TypeFactory::syntaxFunctionType(SourceLoc loc,
  Type *retType, D_func *syntax, TranslationUnit *tunit)
{
  return makeFunctionType(retType);
}


PointerToMemberType *TypeFactory::syntaxPointerToMemberType(SourceLoc loc,
  NamedAtomicType *inClassNAT, CVFlags cv, Type *atType, D_ptrToMember *syntax)
{
  return makePointerToMemberType(inClassNAT, cv, atType);
}


Type *TypeFactory::makeTypeOf_receiver(SourceLoc loc,
  NamedAtomicType *classType, CVFlags cv, D_func *syntax)
{
  CVAtomicType *at = makeCVAtomicType(classType, cv);
  return makeReferenceType(at);
}


FunctionType *TypeFactory::makeSimilarFunctionType(SourceLoc loc,
  Type *retType, FunctionType *similar)
{
  FunctionType *ret = 
    makeFunctionType(retType);
  ret->flags = similar->flags & ~FF_METHOD;     // isMethod is like a parameter
  if (similar->exnSpec) {
    ret->exnSpec = new FunctionType::ExnSpec(*similar->exnSpec);
  }
  return ret;
}


CVAtomicType *TypeFactory::getSimpleType(SimpleTypeId st, CVFlags cv)
{
  xassert((unsigned)st < (unsigned)NUM_SIMPLE_TYPES);
  return makeCVAtomicType(&SimpleType::fixed[st], cv);
}


ArrayType *TypeFactory::setArraySize(SourceLoc loc, ArrayType *type, int size)
{
  return makeArrayType(type->eltType, size);
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
  CVAT(ST_FLOAT_COMPLEX)
  CVAT(ST_DOUBLE_COMPLEX)
  CVAT(ST_LONG_DOUBLE_COMPLEX)
  CVAT(ST_FLOAT_IMAGINARY)
  CVAT(ST_DOUBLE_IMAGINARY)
  CVAT(ST_LONG_DOUBLE_IMAGINARY)
  CVAT(ST_VOID)

  CVAT(ST_ELLIPSIS)
  CVAT(ST_CDTOR)
  CVAT(ST_ERROR)
  CVAT(ST_DEPENDENT)
  CVAT(ST_IMPLINT)
  CVAT(ST_NOTFOUND)

  CVAT(ST_PROMOTED_INTEGRAL)
  CVAT(ST_PROMOTED_ARITHMETIC)
  CVAT(ST_INTEGRAL)
  CVAT(ST_ARITHMETIC)
  CVAT(ST_ARITHMETIC_NON_BOOL)
  CVAT(ST_ANY_OBJ_TYPE)
  CVAT(ST_ANY_NON_VOID)
  CVAT(ST_ANY_TYPE)
  
  CVAT(ST_PRET_STRIP_REF)
  CVAT(ST_PRET_PTM)
  CVAT(ST_PRET_ARITH_CONV)
  CVAT(ST_PRET_FIRST)
  CVAT(ST_PRET_FIRST_PTR2REF)
  CVAT(ST_PRET_SECOND)
  CVAT(ST_PRET_SECOND_PTR2REF)
  #undef CVAT
};


CVAtomicType *BasicTypeFactory::makeCVAtomicType(AtomicType *atomic, CVFlags cv)
{
  // dsw: we now need to avoid this altogether since in
  // setQualifiers() I mutate the cv value on a type after doing the
  // shallowClone(); NOTE: the #ifndef OINK is no longer needed as
  // Oink no longer delegates to this method.
//  #ifndef OINK
//    // FIX: We need to avoid doing this in Oink
//    if (cv==CV_NONE && atomic->isSimpleType()) {
//      // since these are very common, and ordinary Types are immutable,
//      // share them
//      SimpleType *st = atomic->asSimpleType();
//      xassert((unsigned)(st->type) < (unsigned)NUM_SIMPLE_TYPES);
//      return &(unqualifiedSimple[st->type]);
//    }
//  #endif

  return new CVAtomicType(atomic, cv);
}


PointerType *BasicTypeFactory::makePointerType(CVFlags cv, Type *atType)
{
  return new PointerType(cv, atType);
}


Type *BasicTypeFactory::makeReferenceType(Type *atType)
{
  return new ReferenceType(atType);
}


FunctionType *BasicTypeFactory::makeFunctionType(Type *retType)
{
  return new FunctionType(retType);
}

void BasicTypeFactory::doneParams(FunctionType *)
{}


ArrayType *BasicTypeFactory::makeArrayType(Type *eltType, int size)
{
  return new ArrayType(eltType, size);
}


PointerToMemberType *BasicTypeFactory::makePointerToMemberType
  (NamedAtomicType *inClassNAT, CVFlags cv, Type *atType)
{
  return new PointerToMemberType(inClassNAT, cv, atType);
}


Variable *BasicTypeFactory::makeVariable(
  SourceLoc L, StringRef n, Type *t, DeclFlags f, TranslationUnit *)
{
  // the TranslationUnit parameter is ignored by default; it is passed
  // only for the possible benefit of an extension analysis
  Variable *var = new Variable(L, n, t, f);
  return var;
}


// -------------------- XReprSize -------------------
XReprSize::XReprSize(bool d)
  : xBase(stringc << "reprSize of a " << (d ? "dynamically-sized" : "sizeless")
                  << " array"),
    isDynamic(d)
{}

XReprSize::XReprSize(XReprSize const &obj)
  : xBase(obj),
    isDynamic(obj.isDynamic)
{}

XReprSize::~XReprSize()
{}


void throw_XReprSize(bool d)
{
  XReprSize x(d);
  THROW(x);
}


// -------------------- ToXMLTypeVisitor -------------------

void ToXMLTypeVisitor::printIndentation() {
  if (indent) {
    for (int i=0; i<depth; ++i) cout << " ";
  }
}

bool ToXMLTypeVisitor::visitType(Type *obj) {
  printIndentation();

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC: {
    CVAtomicType *atom = obj->asCVAtomicType();
    out << "<AtomicType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "cv=\"" << toXml(atom->cv) << "\">\n";
    break;
  }
  case Type::T_POINTER: {
    PointerType *ptr = obj->asPointerType();
    out << "<PointerType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "cv=\"" << toXml(ptr->cv) << "\"\n";
    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ptr->atType) << "\">\n";
    break;
  }
  case Type::T_REFERENCE: {
    ReferenceType *ref = obj->asReferenceType();
    out << "<ReferenceType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ref->atType) << "\">\n";
    break;
  }
  case Type::T_FUNCTION: {
    FunctionType *func = obj->asFunctionType();
    out << "<FunctionType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    // FIX:
//      printIndentation();
//      out << "flags=\"" << toXml(func->flags) << "\"";
    printIndentation();
    out << "retType=\"TY" << static_cast<void const*>(func->retType) << "\"\n";
    // FIX: skip this for now
    // SObjList<Variable> params;
    // FIX: skip this for now
    // ExnSpec *exnSpec;                  // (nullable owner)

    // FIX: remove this when we do the above attributes
    printIndentation();
    cout << ">\n";

    break;
  }
  case Type::T_ARRAY: {
    ArrayType *arr = obj->asArrayType();
    out << "<ArrayType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "eltType=\"TY" << static_cast<void const*>(arr->eltType) << "\"\n";
    printIndentation();
    out << "size=\"" << arr->size << "\">";
    break;
  }
  case Type::T_POINTERTOMEMBER: {
    PointerToMemberType *ptm = obj->asPointerToMemberType();
    out << "<PointerToMemberType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "inClassNAT=\"TY" << static_cast<void const*>(ptm->inClassNAT) << "\"\n";
    printIndentation();
    out << "cv=\"" << toXml(ptm->cv) << "\"\n";
    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ptm->atType) << "\">";
    break;
  }
  }

  return true;
}

void ToXMLTypeVisitor::postvisitType(Type *obj) {
  --depth;

  printIndentation();

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC:
    out << "</AtomicType>\n";
    break;
  case Type::T_POINTER:
    out << "</PointerType>\n";
    break;
  case Type::T_REFERENCE:
    out << "</ReferenceType>\n";
    break;
  case Type::T_FUNCTION:
    out << "</FunctionType>\n";
    break;
  case Type::T_ARRAY:
    out << "</ArrayType>\n";
    break;
  case Type::T_POINTERTOMEMBER:
    out << "</PointerToMemberType>\n";
    break;
  }
}

//  bool ToXMLTypeVisitor::preVisitVariable(Variable *var) {
//    xassert(var);
//    out << "<Variable";

//    if (var->name) {
//      // FIX: it is not clear that we want to ask for the fully
//      // qualified name of variables that aren't linker visible, as it
//      // is not well defined.
//      out << " fqname='" << var->fullyQualifiedName() << "'";
//    }
//    // FIX: do I want the mangled name?

//    out << ">";
    
//    return true;
//  }

//  void ToXMLTypeVisitor::postVisitVariable(Variable *var) {
//    out << "</Variable>";

//  }


// --------------- debugging ---------------
char *type_toString(Type const *t)
{
  // defined in smbase/strutil.cc
  return copyToStaticBuffer(t->toString().c_str());
}


// EOF
