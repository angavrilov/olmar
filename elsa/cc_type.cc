// cc_type.cc            see license.txt for copyright and terms of use
// code for cc_type.h

#include "cc_type.h"    // this module
#include "trace.h"      // tracingSys
#include "variable.h"   // Variable
#include "strutil.h"    // copyToStaticBuffer

#include <assert.h>     // assert


string makeIdComment(int id)
{
  if (tracingSys("type-ids")) {
    return stringc << "/""*" << id << "*/";
                     // ^^ this is to work around an Emacs highlighting bug
  }
  else {
    return "";
  }
}


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
DOWNCAST_IMPL(AtomicType, CompoundType)
DOWNCAST_IMPL(AtomicType, EnumType)
DOWNCAST_IMPL(AtomicType, TypeVariable)


bool AtomicType::equals(AtomicType const *obj) const
{
  // one exception to the below: when checking for
  // equality, TypeVariables are all equivalent
  if (this->isTypeVariable() &&
      obj->isTypeVariable()) {
    return true;
  }

  // all of the AtomicTypes are unique-representation,
  // so pointer equality suffices
  //
  // it is *important* that we don't do structural equality
  // here, because then we'd be confused by types with the
  // same name that appear in different scopes!
  return this == obj;
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
  
  SimpleType(ST_PROMOTED_ARITHMETIC),
  SimpleType(ST_ANY_OBJ_TYPE),
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
    templateInfo(NULL),
    instName(n),
    syntax(NULL)
{
  curCompound = this;
  curAccess = (k==K_CLASS? AK_PRIVATE : AK_PUBLIC);
}

CompoundType::~CompoundType()
{
  //bases.deleteAll();    // automatic, and I'd need a cast to do it explicitly because it's 'const' now
  if (templateInfo) {
    delete templateInfo;
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
  return templateInfo != NULL  &&
         templateInfo->params.isNotEmpty();
}


string CompoundType::toCString() const
{
  stringBuilder sb;
      
  bool hasParams = templateInfo && templateInfo->params.isNotEmpty();
  if (hasParams) {
    sb << templateInfo->toString() << " ";
  }

  if (!templateInfo || hasParams) {   
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
    sb << keywordName(keyword) << " ";
  }

  sb << (name? name : "/*anonymous*/");

  // template arguments are now in the name
  //if (templateInfo && templateInfo->specialArguments) {
  //  sb << "<" << templateInfo->specialArgumentsRepr << ">";
  //}
   
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
  values.append(v);
  valueIndex.add(name, v);
  
  return v;
}


EnumType::Value const *EnumType::getValue(StringRef name) const
{
  Value *v;
  if (valueIndex.query(name, v)) { 
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
  return string(name);
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
DOWNCAST_IMPL(BaseType, FunctionType)
DOWNCAST_IMPL(BaseType, ArrayType)
DOWNCAST_IMPL(BaseType, PointerToMemberType)


bool BaseType::equals(BaseType const *obj, EqFlags flags) const
{
  if (getTag() != obj->getTag()) {
    return false;
  }

  switch (getTag()) {
    default: xfailure("bad tag");
    #define C(tag,type) \
      case tag: return ((type const*)this)->innerEquals((type const*)obj, flags);
    C(T_ATOMIC, CVAtomicType)
    C(T_POINTER, PointerType)
    C(T_FUNCTION, FunctionType)
    C(T_ARRAY, ArrayType)
    C(T_POINTERTOMEMBER, PointerToMemberType)
    #undef C
  }
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
    return stringc << leftString() << rightString();
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

string BaseType::rightString(bool /*innerParen*/) const
{
  return "";
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
         simpleTypeInfo(asSimpleTypeC()->type).isInteger;
}
                       

bool BaseType::isEnumType() const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC()->atomic->isEnumType();
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

CompoundType *BaseType::ifCompoundType()
{
  return isCompoundType()? asCompoundType() : NULL;
}

CompoundType const *BaseType::asCompoundTypeC() const
{
  return asCVAtomicTypeC()->atomic->asCompoundTypeC();
}

bool BaseType::isOwnerPtr() const
{
  return isPointer() && ((asPointerTypeC()->cv & CV_OWNER) != 0);
}

bool BaseType::isPointer() const
{
  return isPointerType() && asPointerTypeC()->op == PO_POINTER;
}

bool BaseType::isReference() const
{
  return isPointerType() && asPointerTypeC()->op == PO_REFERENCE;
}

BaseType const *BaseType::asRvalC() const
{
  if (isReference()) {
    // note that due to the restriction about stacking reference
    // types, unrolling more than once is never necessary
    return asPointerTypeC()->atType;
  }
  else {
    return this;       // this possibility is one reason to not make 'asRval' return 'Type*' in the first place
  }
}


TypeVariable *BaseType::asTypeVariable() 
{ 
  return asCVAtomicType()->atomic->asTypeVariable(); 
}


bool BaseType::isCVAtomicType(AtomicType::Tag tag) const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC()->atomic->getTag() == tag;
}

bool BaseType::isTemplateFunction() const
{
  return isFunctionType() &&
         asFunctionTypeC()->isTemplate();
}

bool BaseType::isTemplateClass() const
{
  return isCompoundType() &&
         asCompoundTypeC()->isTemplate();
}


bool typeIsError(Type const *t)
{
  return t->isError();
}

bool BaseType::containsErrors() const
{
  // hmm.. bit of hack..
  return static_cast<Type const *>(this)->
    anyCtorSatisfies(typeIsError);
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
    anyCtorSatisfies(typeHasTypeVariable);
}


// ----------------- CVAtomicType ----------------
bool CVAtomicType::innerEquals(CVAtomicType const *obj, EqFlags flags) const
{
  return atomic->equals(obj->atomic) &&
         ((flags & EF_IGNORE_TOP_CV) || (cv == obj->cv));
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


bool CVAtomicType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this);
}


CVFlags CVAtomicType::getCVFlags() const
{
  return cv;
}


// ------------------- PointerType ---------------
PointerType::PointerType(PtrOper o, CVFlags c, Type *a)
  : op(o), cv(c), atType(a)
{
  // since references are always immutable, it makes no sense to
  // apply const or volatile to them
  xassert(op==PO_REFERENCE? cv==CV_NONE : true);

  // it also makes no sense to stack reference operators underneath
  // other indirections (i.e. no ptr-to-ref, nor ref-to-ref)
  xassert(!a->isReference());
}


bool PointerType::innerEquals(PointerType const *obj, EqFlags flags) const
{
  // note how EF_IGNORE_TOP_CV allows *this* type's cv flags to differ,
  // but it's immediately suppressed once we go one level down; this
  // behavior repeated in all 'innerEquals' methods

  return op == obj->op &&
         ((flags & EF_IGNORE_TOP_CV) || (cv == obj->cv)) &&
         atType->equals(obj->atType, flags & EF_PROP);
}


string PointerType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << (op==PO_POINTER? "*" : "&");
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


bool PointerType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags PointerType::getCVFlags() const
{
  return cv;
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


bool FunctionType::ExnSpec::anyCtorSatisfies(Type::TypePred pred) const
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
    exnSpec(NULL),
    templateParams(NULL)
{}


FunctionType::~FunctionType()
{
  if (exnSpec) {
    delete exnSpec;
  }
  if (templateParams) {
    delete templateParams;
  }
}


bool FunctionType::innerEquals(FunctionType const *obj, EqFlags flags) const
{
  // I do not compare the special-function flags because I see no
  // need, and this code didn't used to do that check because it
  // didn't know about those properties

  if (flags & EF_SIGNATURE) {
    if (isMember() == obj->isMember()) {
      // if both are nonstatic members, or neither is, then we
      // simply compare all parameters (unless explicitly told
      // to EF_SKIPTHIS by the caller), so flags are unchanged
    }
    else {
      // if one is a nonstatic member and the other is a static member,
      // then comparison ignores the 'this' param on the nonstatic one
      // [cppstd 13.1 para 2]
      flags |= EF_SKIPTHIS;
    }
  }

  if (retType->equals(obj->retType, flags & EF_PROP) &&
      ((flags & EF_SKIPTHIS) || (isMember() == obj->isMember())) &&
      acceptsVarargs() == obj->acceptsVarargs()) {
    // so far so good, try the parameters
    return equalParameterLists(obj, flags) &&
           equalExceptionSpecs(obj);
  }
  else {
    return false;
  }
}

bool FunctionType::equalParameterLists(FunctionType const *obj,
                                       EqFlags flags) const
{
  SObjListIter<Variable> iter1(this->params);
  SObjListIter<Variable> iter2(obj->params);

  // skip the 'this' parameters if desired
  if (flags & EF_SKIPTHIS) {
    if (this->isMember()) iter1.adv();
    if (obj->isMember()) iter2.adv();
  }

  // this takes care of FunctionType::innerEquals' obligation
  // to suppress non-propagated flags after consumption
  flags &= EF_PROP;

  if (flags & EF_SIGNATURE) {
    // allow toplevel cv flags to differ
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



void FunctionType::addParam(Variable *param)
{
  xassert(param->hasFlag(DF_PARAMETER));
  params.append(param);
}

void FunctionType::addThisParam(Variable *param)
{
  xassert(param->type->isReference());
  xassert(param->hasFlag(DF_PARAMETER));
  xassert(!isMember());
  params.prepend(param);
  flags |= FF_MEMBER;
}

void FunctionType::doneParams()
{}


Variable const *FunctionType::getThisC() const
{
  xassert(isMember());
  return params.firstC();
}

CVFlags FunctionType::getThisCV() const
{
  if (isMember()) {
    // expect 'this' to be of type 'SomeClass cv &', and
    // dig down to get that 'cv'
    return getThisC()->type->asPointerType()->atType->asCVAtomicType()->cv;
  }
  else {
    return CV_NONE;
  }
}


string FunctionType::leftString(bool innerParen) const
{
  stringBuilder sb;

  // template parameters
  if (templateParams) {
    sb << templateParams->toString() << " ";
  }

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
    if (isMember() && ct==1) {
      // don't actually print the first parameter
      // the 'm' stands for nonstatic member function
      sb << "/*m*/ ";
      continue;
    }
    if (ct >= 3 || (!isMember() && ct>=2)) {
      sb << ", ";
    }
    sb << iter.data()->toStringAsParameter();
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

  CVFlags cv = getThisCV();
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
      sb << iter.data()->toString();
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


bool parameterListCtorSatisfies(Type::TypePred pred, 
                                SObjList<Variable> const &params)
{
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (iter.data()->type->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}

bool FunctionType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         retType->anyCtorSatisfies(pred) ||
         parameterListCtorSatisfies(pred, params) ||
         (exnSpec && exnSpec->anyCtorSatisfies(pred)) ||
         (templateParams && templateParams->anyCtorSatisfies(pred));
}



// ----------------- TemplateParams --------------
TemplateParams::TemplateParams(TemplateParams const &obj)
{
  // copy list contents
  params = obj.params;
}


TemplateParams::~TemplateParams()
{}


string TemplateParams::toString() const
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
      sb << p->toStringAsParameter();
    }
  }
  sb << ">";
  return sb;
}


bool TemplateParams::equalTypes(TemplateParams const *obj) const
{
  SObjListIter<Variable> iter1(params), iter2(obj->params);
  for (; !iter1.isDone() && !iter2.isDone();
       iter1.adv(), iter2.adv()) {
    if (iter1.data()->type->equals(iter2.data()->type)) {
      // ok
    }
    else {
      return false;
    }
  }

  return iter1.isDone() == iter2.isDone();
}


bool TemplateParams::anyCtorSatisfies(Type::TypePred pred) const
{
  return parameterListCtorSatisfies(pred, params);
}


// ------------------- STemplateArgument ---------------
STemplateArgument::STemplateArgument(STemplateArgument const &obj)
  : kind(obj.kind)
{
  // take advantage of representation uniformity
  value.i = obj.value.i;
  value.v = obj.value.v;    // just in case ptrs and ints are diff't size
}

bool STemplateArgument::equals(STemplateArgument const *obj) const
{
  if (kind != obj->kind) {
    return false;
  }

  switch (kind) {
    case STA_TYPE:     return value.t->equals(obj->value.t);
    case STA_INT:      return value.i == obj->value.i;
    default:           return value.v == obj->value.v;
  }
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


// ------------------ ClassTemplateInfo -------------
ClassTemplateInfo::ClassTemplateInfo(StringRef name)
  : baseName(name),
    instantiations(),         // empty list
    arguments(),              // empty list
    argumentSyntax(NULL)
{}

ClassTemplateInfo::~ClassTemplateInfo()
{}


bool ClassTemplateInfo::equalArguments
  (SObjList<STemplateArgument> const &list) const
{
  ObjListIter<STemplateArgument> iter1(arguments);
  SObjListIter<STemplateArgument> iter2(list);
  
  while (!iter1.isDone() && !iter2.isDone()) {
    if (!iter1.data()->equals(iter2.data())) {
      return false;
    }
    
    iter1.adv();
    iter2.adv();
  }
  
  return iter1.isDone() && iter2.isDone();
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
  if (hasSize()) {
    return eltType->reprSize() * size;
  }
  else {
    // or should I throw an exception ..?
    cout << "warning: reprSize of a sizeless array\n";
    return 0;
  }
}


bool ArrayType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         eltType->anyCtorSatisfies(pred);
}


// ---------------- PointerToMemberType ---------------
PointerToMemberType::PointerToMemberType(CompoundType *i, CVFlags c, Type *a)
  : inClass(i), cv(c), atType(a)
{
  // cannot have pointer to reference type
  xassert(!a->isReference());
  
  // there are some other semantic restrictions, but I let the
  // type checker enforce them
}


bool PointerToMemberType::innerEquals(PointerToMemberType const *obj, EqFlags flags) const
{
  return inClass == obj->inClass &&
         ((flags & EF_IGNORE_TOP_CV) || (cv == obj->cv)) &&
         atType->equals(obj->atType, flags & EF_PROP);
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
  s << inClass->name << "::*";
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


bool PointerToMemberType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags PointerToMemberType::getCVFlags() const
{
  return cv;
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
    case Type::T_FUNCTION:  return cloneFunctionType(src->asFunctionType());
    case Type::T_ARRAY:     return cloneArrayType(src->asArrayType());
    case Type::T_POINTERTOMEMBER: return clonePointerToMemberType(src->asPointerToMemberType());
  }
}


Type *TypeFactory::applyCVToType(SourceLoc loc, CVFlags cv, Type *baseType,
                                 TypeSpecifier *)
{
  if (baseType->isError()) {
    return baseType;
  }

  if (cv == CV_NONE) {
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

  // first, check for special cases
  switch (baseType->getTag()) {
    case Type::T_ATOMIC: {
      CVAtomicType *atomic = baseType->asCVAtomicType();
      if ((atomic->cv | cv) == atomic->cv) {
        // the given type already contains 'cv' as a subset,
        // so no modification is necessary
        return baseType;
      }
      else {
        // we have to add another CV, so that means creating
        // a new CVAtomicType with the same AtomicType as 'baseType',
        // but with the new flags added
        return makeCVAtomicType(loc, atomic->atomic, atomic->cv | cv);
      }
      break;
    }

    case Type::T_POINTER: {
      // logic here is nearly identical to the T_ATOMIC case
      PointerType *ptr = baseType->asPointerType();
      if (ptr->op == PO_REFERENCE) {
        return NULL;     // can't apply CV to references
      }
      if ((ptr->cv | cv) == ptr->cv) {
        return baseType;
      }
      else {
        return makePointerType(loc, ptr->op, ptr->cv | cv, ptr->atType);
      }
      break;
    }

    case Type::T_POINTERTOMEMBER: {
      // again similar to the above
      PointerToMemberType *ptm = baseType->asPointerToMemberType();
      if ((ptm->cv | cv) == ptm->cv) {
        return baseType;    // no-op
      }
      else {
        return makePointerToMemberType(loc, ptm->inClass, ptm->cv | cv, ptm->atType);
      }
      break;
    }

    default:    // silence warning
    case Type::T_FUNCTION:
    case Type::T_ARRAY:
      // can't apply CV to either of these
      return NULL;
  }
}


Type *TypeFactory::makeRefType(SourceLoc loc, Type *underlying)
{
  if (underlying->isError()) {
    return underlying;
  }
  else {
    return makePointerType(loc, PO_REFERENCE, CV_NONE, underlying);
  }
}


PointerType *TypeFactory::syntaxPointerType(SourceLoc loc,
  PtrOper op, CVFlags cv, Type *type, D_pointer *)
{
  return makePointerType(loc, op, cv, type);
}


FunctionType *TypeFactory::syntaxFunctionType(SourceLoc loc,
  Type *retType, D_func *syntax, TranslationUnit *tunit)
{
  return makeFunctionType(loc, retType);
}


PointerToMemberType *TypeFactory::syntaxPointerToMemberType(SourceLoc loc,
  CompoundType *inClass, CVFlags cv, Type *atType, D_ptrToMember *syntax)
{
  return makePointerToMemberType(loc, inClass, cv, atType);
}


PointerType *TypeFactory::makeTypeOf_this(SourceLoc loc,
  CompoundType *classType, CVFlags cv, D_func *syntax)
{
  CVAtomicType *at = makeCVAtomicType(loc, classType, cv);
  return makePointerType(loc, PO_REFERENCE, CV_NONE, at);
}


FunctionType *TypeFactory::makeSimilarFunctionType(SourceLoc loc,
  Type *retType, FunctionType *similar)
{
  FunctionType *ret = 
    makeFunctionType(loc, retType);
  ret->flags = similar->flags & ~FF_MEMBER;     // isMember is like a parameter
  if (similar->exnSpec) {
    ret->exnSpec = new FunctionType::ExnSpec(*similar->exnSpec);
  }
  if (similar->templateParams) {
    ret->templateParams = new TemplateParams(*similar->templateParams);
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

  CVAT(ST_PROMOTED_ARITHMETIC)
  CVAT(ST_ANY_OBJ_TYPE)
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
  PtrOper op, CVFlags cv, Type *atType)
{
  return new PointerType(op, cv, atType);
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
  CompoundType *inClass, CVFlags cv, Type *atType)
{
  return new PointerToMemberType(inClass, cv, atType);
}


// Types are immutable, so cloning is pointless
CVAtomicType *BasicTypeFactory::cloneCVAtomicType(CVAtomicType *src)
  { return src; }
PointerType *BasicTypeFactory::clonePointerType(PointerType *src)
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
  return new Variable(L, n, t, f);
}

Variable *BasicTypeFactory::cloneVariable(Variable *src)
{
  // immutable => don't clone
  return src;
}


// ------------------ test -----------------
void cc_type_checker()
{
  #ifndef NDEBUG
  // verify the fixed[] arrays
  // it turns out this is probably redundant, since the compiler will
  // complain if I leave out an entry, now that the classes do not have
  // default constructors! yes!
  for (int i=0; i<NUM_SIMPLE_TYPES; i++) {
    assert(SimpleType::fixed[i].type == i);

    #if 0    // disabled for now
    SimpleType const *st = (SimpleType const*)(CVAtomicType::fixed[i].atomic);
    assert(st && st->type == i);
    #endif // 0
  }
  #endif // NDEBUG
}


// --------------- debugging ---------------
char *type_toString(Type const *t)
{
  // defined in smbase/strutil.cc
  return copyToStaticBuffer(t->toString());
}


