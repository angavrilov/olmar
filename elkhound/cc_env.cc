// cc_env.cc
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys
#include "dataflow.h"    // DataflowEnv
#include "ckheap.h"      // heapCheck
#include "cc_tree.h"     // CCTreeNode


// ----------------------- Variable ---------------------------
string Variable::toString() const
{
  // don't care about printing the declflags right now
  return type->toString(name);
}


// --------------------------- Env ----------------------------
Env::Env(DataflowEnv *d, TypeEnv *te)
  : parent(NULL),
    typeEnv(te),
    nameCounter(1),
    compounds(),
    enums(),
    typedefs(),
    variables(),
    errors(),
    trialBalloon(false),
    denv(d),
    referenceCt(0)
{
  // init by inserting the builtin types
  loopi(NUM_SIMPLE_TYPES) {
    SimpleType *st = new SimpleType((SimpleTypeId)i);
    grabAtomic(st);
    
    // I want the first indices to match the SimpleTypeIds
    // so I can use them as ids directly
    xassert(st->type == i);

    Type *t = makeType(st);
    xassert(t->id == i);
  }

  declareVariable(NULL, "__builtin_constant_p", DF_NONE,
    makeFunctionType_1arg(
      getSimpleType(ST_INT),                          // return type
      CV_NONE,
      getSimpleType(ST_INT), "potentialConstant"),    // arg type
      true /*initialized*/);

  trace("refct") << "created toplevel Env at " << this << "\n";
}


Env::Env(Env *p, TypeEnv *te)
  : parent(p),
    typeEnv(te),
    nameCounter(-1000),    // so it will be obvious if I use it (I don't intend to)
    compounds(),
    enums(),
    typedefs(),
    variables(),
    errors(),
    trialBalloon(false),
    denv(p? p->denv : NULL),
    referenceCt(0)
{
  if (p) {
    p->referenceCt++;
    trace("refct") << "created Env at " << this << "; parent is "
                   << p << " with new refct=" << p->referenceCt << endl;
  }
  else {
    // at the moment, this happens when I make the environments
    // which hold info about struct members .. I will probably
    // redesign the Env ctors at some point
    trace("refct") << "created struct interior Env at " << this << endl;
  }
}


Env::~Env()
{
  killParentLink();
  
  // explicitly free things .. this isn't necessary except that
  // some of them have Environments internally, and that makes
  // my refct nonzero if I don't free them now
  compounds.empty();
  enums.empty();
  typedefs.empty();
  variables.empty();
  errors.deleteAll();

  if (referenceCt != 0) {
    // print a message so I know what the reference count was even if
    // I'm not in the debugger
    cout << "ABOUT TO FAIL: destroying Env at " << this << " with refct="
         << referenceCt << endl;
  }

  // not fixing this led to a very long search for a memory corruption
  // bug; I thought letting 'parent' pointers to me dangle wouldn't
  // cause a problem, but that was very wrong ..
  // NOTE: throwing an exception is somewhat dangerous, because
  // if we happen to be here because we're unwinding the stack
  // due to *another* exception throw, this will abort(3) the program
  xassert(referenceCt == 0);
}


void Env::killParentLink()
{
  if (parent) {
    // if we're carrying any errors, deliver them to the
    // containing environment
    parent->errors.concat(errors);

    // and decrement its refct
    parent->referenceCt--;
    trace("refct") << "destroying Env at " << this << "; parent is "
                   << parent << " with new refct=" << parent->referenceCt << endl;

    // parent link is now gone; this is the only line that's allowed
    // to reassign 'parent', so I leave it declared const and just
    // cast it here
    const_cast<Env*&>(parent) = NULL;
  }
  else {
    // may as well print errors
    flushLocalErrors(cout);
  }
}


// at one point I started switching all the calls over so they
// did grab inline, but that downgrades the return type ..
void Env::grab(Type *t)
{
  typeEnv->grab(t);
}

void Env::grabAtomic(AtomicType *t)
{
  typeEnv->grabAtomic(t);
}


CVAtomicType *Env::makeType(AtomicType const *atomic)
{
  CVAtomicType *ret = new CVAtomicType(atomic, CV_NONE);
  grab(ret);
  return ret;
}


Type const *Env::applyCVToType(CVFlags cv, Type const *baseType)
{
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
      CVAtomicType const &atomic = baseType->asCVAtomicTypeC();
      if ((atomic.cv | cv) == atomic.cv) {
        // the given type already contains 'cv' as a subset,
        // so no modification is necessary
        return baseType;
      }
      else {
        // we have to add another CV, so that means creating
        // a new CVAtomicType with the same AtomicType as 'baseType'
        CVAtomicType *ret = new CVAtomicType(atomic);
        grab(ret);

        // but with the new flags added
        ret->cv = (CVFlags)(ret->cv | cv);

        return ret;
      }
      break;
    }

    case Type::T_POINTER: {
      // logic here is nearly identical to the T_ATOMIC case
      PointerType const &ptr = baseType->asPointerTypeC();
      if (ptr.op == PO_REFERENCE) {
        return NULL;     // can't apply CV to references
      }
      if ((ptr.cv | cv) == ptr.cv) {
        return baseType;
      }
      else {
        PointerType *ret = new PointerType(ptr);
        grab(ret);
        ret->cv = (CVFlags)(ret->cv | cv);
        return ret;
      }
      break;
    }

    default:    // silence warning
    case Type::T_FUNCTION:
    case Type::T_ARRAY:
      // can't apply CV to either of these (function has CV, but
      // can't get it after the fact)
      return NULL;
  }
}


ArrayType const *Env::setArraySize(ArrayType const *type, int size)
{                                      
  ArrayType *ret = new ArrayType(type->eltType, size);
  grab(ret);
  return ret;
}


PointerType *Env::makePtrOperType(PtrOper op, CVFlags cv, Type const *type)
{
  PointerType *ret = new PointerType(op, cv, type);
  grab(ret);
  return ret;
}


FunctionType *Env::makeFunctionType(Type const *retType, CVFlags cv)
{
  FunctionType *ret = new FunctionType(retType, cv);
  grab(ret);
  return ret;
}


FunctionType *Env::makeFunctionType_1arg(
  Type const *retType, CVFlags cv,
  Type const *arg1Type, char const *arg1Name)
{
  FunctionType *ret = makeFunctionType(retType, cv);
  ret->addParam(new Parameter(arg1Type, arg1Name));
  return ret;
}


ArrayType *Env::makeArrayType(Type const *eltType, int size)
{
  ArrayType *ret = new ArrayType(eltType, size);
  grab(ret);
  return ret;
}

ArrayType *Env::makeArrayType(Type const *eltType)
{
  ArrayType *ret = new ArrayType(eltType);
  grab(ret);
  return ret;
}


CompoundType *Env::lookupOrMakeCompound(char const *name, CompoundType::Keyword keyword)
{
  bool anon = (strlen(name) == 0);

  CompoundType *ret;
  if (!anon) {
    // see if it exists
    ret = lookupCompound(name);
    if (ret) {
      if (ret->keyword != keyword) {
        return NULL;     // keyword mismatch
      }
      else {
        return ret;
      }
    }
  }

  // does not exist -- make a new one

  string newName;
  if (!anon) {
    newName = name;
  }
  else {
    newName = makeAnonName();
  }

  ret = new CompoundType(keyword, newName);
  grabAtomic(ret);
  compounds.add(newName, ret);

  // debugging: print it
  if (tracingSys("env-declare")) {
    // print declaration
    indent(cout);
    cout << CompoundType::keywordName(keyword) << ": "
         << newName << endl;
  }
  return ret;
}


int Env::makeFreshInteger()
{
  // make the parent get it so it's essentially a global counter
  if (parent) {
    return parent->makeFreshInteger();
  }
  else {
    return nameCounter++;
  }
}


string Env::makeFreshName(char const *prefix)
{
  return stringc << prefix << makeFreshInteger();
}


string Env::makeAnonName()
{
  return makeFreshName("__anon");
}


// NOTE: the name lookup rules in this code have not been
// carefully checked against what the standard requires,
// so they are likely wrong; I intend to go through and
// make the correct at some point


CompoundType *Env::lookupCompound(char const *name)
{
  if (compounds.isMapped(name)) {
    return compounds.queryf(name);
  }
  else if (parent) {
    return parent->lookupCompound(name);
  }
  else {
    return NULL;
  }
}


EnumType *Env::lookupEnum(char const *name)
{
  if (enums.isMapped(name)) {
    return enums.queryf(name);
  }
  else if (parent) {
    return parent->lookupEnum(name);
  }
  else {
    return NULL;
  }
}


EnumType *Env::makeEnumType(char const *name)
{                              
  EnumType *ret = new EnumType(name);
  enums.add(name, ret);
  grabAtomic(ret);
  
  // debugging: print it
  if (tracingSys("env-declare")) {
    indent(cout);
    cout << "enum: " << name << endl;
  }

  return ret;
}


void Env::addEnumValue(CCTreeNode const *node, char const *name, 
                       EnumType const *type, int value)
{
  Variable *val = declareVariable(node, name, DF_ENUMVAL, makeType(type),
                                  true /*initialized*/);
  xassert(val->isEnumValue());
  val->enumValue = value;
}


CVAtomicType const *Env::getSimpleType(SimpleTypeId st)
{
  xassert(isValid(st));
  Type *ret = typeEnv->lookup(st);
  return &( ret->asCVAtomicTypeC() );
}


Type const *Env::lookupLocalType(char const *name)
{
  if (typedefs.isMapped(name)) {
    return typedefs.queryf(name);
  }

  if (compounds.isMapped(name)) {
    return makeType(compounds.queryf(name));
  }

  if (enums.isMapped(name)) {
    return makeType(enums.queryf(name));
  }

  return NULL;
}

Type const *Env::lookupType(char const *name)
{
  // search the current environment for the type
  Type const *ret = lookupLocalType(name);
  if (ret) { return ret; }

  // search it for a variable name, which would shadow
  // the type
  if (isLocalDeclaredVar(name)) {
    return NULL;
  }

  // now search parents
  if (parent) {
    return parent->lookupType(name);
  }
  else {
    return NULL;
  }
}


ostream &Env::indent(ostream &os) const
{
  // indent proportional to nesting level
  for (Env *p = parent; p != NULL; p = p->parent) {
    os << "  ";
  }   
  return os;
}


Variable *Env::declareVariable(CCTreeNode const *node, char const *name,
                               DeclFlags flags, Type const *type,
                               bool initialized)
{
  Variable *ret = NULL;
  if (!( flags & DF_TYPEDEF )) {
    // declare a variable
    if (variables.isMapped(name)) {
      // duplicate name
      Variable *prev = getVariable(name);

      // if the old decl and new are the same array type,
      // except the old was missing a size, replace the
      // old with the new
      if (type->isArrayType() &&
          prev->type->isArrayType()) {
        ArrayType const *arr = &( type->asArrayTypeC() );
        ArrayType const *parr = &( prev->type->asArrayTypeC() );
        if (arr->eltType->equals(parr->eltType) &&
            arr->hasSize &&
            !parr->hasSize) {
          // replace old with new
          prev->type = type;
        }
      }

      // no way we allow it if the types don't match
      if (!type->equals(prev->type)) {
        node->throwError(stringc
          << "conflicting declaration for `" << name
          << "'; previous type was `" << prev->type->toString()
          << "', this type is `" << type->toString() << "'");
      }

      // but it's ok if both were functions
      // and/or both were extern or static (TODO: what are the
      // real rules??); and, there can be at most one initializer
      if ( ( type->isFunctionType() ||
             ((flags & DF_EXTERN) && (prev->declFlags & DF_EXTERN)) ||
             ((flags & DF_STATIC) && (prev->declFlags & DF_STATIC))
           )
           && (!prev->isInitialized() || !initialized)
         ) {
        // ok
        ret = prev;
      }
      else {
        SemanticError err(node, SE_DUPLICATE_VAR_DECL);
        err.varName = name;
        node->throwError(err);
      }
    }

    else /*not already mapped*/ {
      if (isGlobalEnv()) {
        flags = (DeclFlags)(flags | DF_GLOBAL);
      }

      ret = addVariable(name, flags, type);
    }
         
    if (initialized) {
      ret->sayItsInitialized();
    }
  }

  else {
    // declare a typedef
    if (typedefs.isMapped(name)) {
      node->throwError(stringc
        << "duplicate typedef for `" << name << "'");
    }
    typedefs.add(name, const_cast<Type*>(type));
  }

  // debugging: print it
  if (tracingSys("env-declare")) {
    // print declaration
    indent(cout);
    cout << ((flags&DF_TYPEDEF) ? "typedef: " : "variable: ");
    cout << type->toString(name) << endl;
  }

  return ret;
}


Variable *Env::addVariable(char const *name, DeclFlags flags, Type const *type)
{
  Variable *var = new Variable(name, flags, type);
  variables.add(name, var);

  // add this to the dataflow environment too (if it wants it)
  if (denv) {
    denv->addVariable(var);
  }
  
  return var;
}


bool Env::isLocalDeclaredVar(char const *name)
{
  return variables.isMapped(name);
}

bool Env::isDeclaredVar(char const *name)
{
  if (isLocalDeclaredVar(name)) {
    return true;
  }

  // if there's a local type, it shadows variables
  // in any enclosing scopes (TODO3: is this right?)
  if (lookupLocalType(name)) {
    return false;
  }

  // look in enclosing scopes
  return parent && parent->isDeclaredVar(name);
}


bool Env::isEnumValue(char const *name)
{
  Variable *var = getVariableIf(name);
  return var && var->isEnumValue();
}


Variable *Env::getVariableIf(char const *name)
{
  if (variables.isMapped(name)) {
    return variables.queryf(name);
  }

  if (parent) {
    return parent->getVariableIf(name);
  }
  else {
    return NULL;
  }
}

Variable *Env::getVariable(char const *name)
{
  Variable *var = getVariableIf(name);
  if (!var) {
    xfailure(stringc << "getVariable: undeclared variable `" << name << "'");
  }
  return var;
}


void Env::report(SemanticError const &err)
{
  errors.append(new SemanticError(err));
}


int Env::numErrors() const
{
  int ct = numLocalErrors();
  if (parent) {
    ct += parent->numErrors();
  }
  return ct;
}


void Env::printErrors(ostream &os) const
{
  if (parent) {
    parent->printErrors(os);
  }
  printLocalErrors(os);
}

void Env::printLocalErrors(ostream &os) const
{
  FOREACH_OBJLIST(SemanticError, errors, iter) {
    // indenting here doesn't quite work because sometimes
    // the error is passed to a parent in ~Env and then
    // we get the wrong indentation level...
    // so, screw it; the indentation looks ugly anyway
    os << "Error: " << iter.data()->whyStr() << endl;
  }
}


void Env::flushLocalErrors(ostream &os)
{
  printLocalErrors(os);
  forgetLocalErrors();
}

void Env::forgetLocalErrors()
{
  errors.deleteAll();
}


bool Env::isTrialBalloon() const
{
  // true if we, or any parent, is a trial
  return trialBalloon ||
         (parent && parent->isTrialBalloon());
}


Variable *Env::newTmpVar(Type const *type)
{                 
  string name = makeFreshName("tmpvar");
  Variable *var = addVariable(name, DF_NONE, type);
  return var;
}


string Env::toString() const
{
  stringBuilder sb;
  
  // for now, just the variables
  StringObjDict<Variable>::Iter iter(variables);
  for (; !iter.isDone(); iter.next()) {
    sb << iter.value()->toString() << " ";
  }
  
  return sb;
}


// --------------------- TypeEnv ------------------
TypeEnv::TypeEnv()
{}

TypeEnv::~TypeEnv()
{}


TypeId TypeEnv::grab(Type *type)
{ 
  xassert(type->id == NULL_TYPEID);
  TypeId ret = types.insert(type);
  type->id = ret;
  return ret;
}

AtomicTypeId TypeEnv::grabAtomic(AtomicType *type)
{
  xassert(type->id == NULL_ATOMICTYPEID);
  AtomicTypeId ret = atomicTypes.insert(type);
  type->id = ret;
  return ret;
}

