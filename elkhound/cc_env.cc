// cc_env.cc
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys


SimpleType const *Env::simpleBuiltins = NULL;
CVAtomicType const *Env::builtins = NULL;


Env::Env()
  : parent(NULL),
    anonCounter(1),
    compounds(),
    enums(),
    intermediates()
{
  // init of global data (won't be freed)
  if (!builtins) {
    SimpleType *sb = new SimpleType[NUM_SIMPLE_TYPES];
    {loopi(NUM_SIMPLE_TYPES) {
      sb[i].type = (SimpleTypeId)i;
    }}
    simpleBuiltins = sb;

    CVAtomicType *b = new CVAtomicType[NUM_SIMPLE_TYPES];
    loopi(NUM_SIMPLE_TYPES) {
      b[i].atomic = &simpleBuiltins[i];
      b[i].cv = CV_NONE;
    }
    builtins = b;
  }
}


Env::Env(Env *p)
  : parent(p),
    anonCounter(-1000),    // so it will be obvious if I use it (I don't intend to)
    compounds(),
    enums(),
    intermediates()
{}


Env::~Env()
{}


void Env::grab(Type *t)
{
  intermediates.prepend(t);
}


CVAtomicType *Env::makeType(AtomicType const *atomic)
{
  CVAtomicType *ret = new CVAtomicType(atomic, CV_NONE);
  grab(ret);
  return ret;
}


Type const *Env::applyCVToType(CVFlags cv, Type const *baseType)
{
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
  compounds.add(newName, ret);

  return ret;
}


string Env::makeAnonName() 
{
  // make the parent get the name so it's essentially a global
  // counter
  if (parent) {
    return parent->makeAnonName();
  }
  else {
    return stringc << "__anon" << (anonCounter++);
  }
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


CVAtomicType const *Env::getSimpleType(SimpleTypeId st)
{                     
  xassert(isValid(st));
  return &builtins[st];
}


Type const *Env::lookupType(char const *name)
{
  // search the current environment
  if (typedefs.isMapped(name)) {
    return typedefs.queryf(name);
  }

  if (compounds.isMapped(name)) {
    return makeType(compounds.queryf(name));
  }
  
  if (enums.isMapped(name)) {
    return makeType(enums.queryf(name));
  }

  // now search parents
  if (parent) {
    return parent->lookupType(name);
  }
  else {
    return NULL;
  }
}


void Env::declareVariable(char const *name, DeclFlags flags, Type const *type)
{
  variables.add(name, new Variable(flags, type));
  
  // debugging: print it
  if (tracingSys("env-declare")) {
    // indent proportional to nesting level
    for (Env *p = parent; p != NULL; p = p->parent) {
      cout << "  ";
    }

    // print declaration
    cout << type->toString(name) << endl;
  }
}
