// cc_env.cc            see license.txt for copyright and terms of use
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys
#include "ckheap.h"      // heapCheck
#include "strtable.h"    // StringTable
#include "cc_lang.h"     // CCLang


// ----------------- ErrorMsg -----------------
ErrorMsg::~ErrorMsg()
{}


string ErrorMsg::toString() const
{
  return stringc << loc.toString() << " " << msg;
}


// --------------------- Env -----------------
Env::Env(StringTable &s, CCLang &L)
  : scopes(),
    disambiguateOnly(false),
    errors(),
    str(s),
    lang(L),                      
    
    // filled in below; initialized for definiteness
    type_info_const_ref(NULL),
    conversionOperatorName(NULL)
{
  // create first scope
  scopes.prepend(new Scope(0 /*changeCount*/, SourceLocation()));
  
  // create the typeid type
  CompoundType *ct = new CompoundType(CompoundType::K_CLASS, str("type_info"));
  // TODO: put this into the 'std' namespace
  // TODO: fill in the proper fields and methods
  type_info_const_ref = makeRefType(makeCVType(ct, CV_CONST));
  
  // cache this because I compare with it frequently
  conversionOperatorName = str("conversion-operator");
}

Env::~Env()
{ 
  // delete the scopes one by one, so we can skip any
  // which are in fact not owned
  while (scopes.isNotEmpty()) {
    Scope *s = scopes.removeFirst();
    if (s->curCompound) {
      // this isn't one we own
    }
    else {
      // we do own this one
      delete s;
    }
  }

  errors.deleteAll();
}


Scope *Env::enterScope()
{
  trace("env") << "entered scope\n";

  // propagate the 'curFunction' field
  Function *f = scopes.first()->curFunction;
  Scope *newScope = new Scope(getChangeCount(), loc());
  scopes.prepend(newScope);
  newScope->curFunction = f;
  
  return newScope;
}

void Env::exitScope(Scope *s)
{
  trace("env") << "exited scope\n";
  Scope *f = scopes.removeFirst();
  xassert(s == f);
  delete f;
}


void Env::extendScope(Scope *s)
{
  if (s->curCompound) {
    trace("env") << "extending scope "
                 << s->curCompound->keywordAndName() << "\n";
  }
  else {
    trace("env") << "extending scope at " << (void*)s << "\n";
  }
  scopes.prepend(s);
  s->curLoc = loc();
}

void Env::retractScope(Scope *s)
{
  if (s->curCompound) {
    trace("env") << "retracting scope "
                 << s->curCompound->keywordAndName() << "\n";
  }
  else {
    trace("env") << "retracting scope at " << (void*)s << "\n";
  }
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


void Env::setLoc(SourceLocation const &loc)
{
  trace("loc") << "setLoc: " << loc.toString() << endl;
  
  // only set it if it's a valid location; I get invalid locs
  // from abstract declarators..
  if (loc.validLoc()) {
    Scope *s = scope();
    s->curLoc = loc;
  }
}

SourceLocation const &Env::loc() const
{
  return scopeC()->curLoc;
}


// -------- insertion --------
Scope *Env::acceptingScope()
{
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


bool Env::addVariable(Variable *v)
{
  Scope *s = acceptingScope();
  registerVariable(v);
  return s->addVariable(v);
}

void Env::registerVariable(Variable *v)
{
  Scope *s = acceptingScope();
  if (s->curCompound && s->curCompound->name) {
    // since the scope has a name, let the variable point at it
    v->scope = s;
  }
}


bool Env::addCompound(CompoundType *ct)
{
  return acceptingScope()->addCompound(ct);
}


bool Env::addEnum(EnumType *et)
{
  return acceptingScope()->addEnum(et);
}


// -------- lookup --------
Scope *Env::lookupQualifiedScope(PQName const *name)
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
    }

    else {
      // look for a class called 'qual' in scope-so-far
      CompoundType *ct =
        scope==NULL? lookupCompound(qual, false /*innerOnly*/) :
                     scope->lookupCompound(qual, false /*innerOnly*/);
      if (!ct) {
        // I'd like to include some information about which scope
        // we were looking in, but I don't want to be computing
        // intermediate scope names for successful lookups; also,
        // I am still considering adding some kind of scope->name()
        // functionality, which would make this trivial.
        //
        // alternatively, I could just re-traverse the original name;
        // I'm lazy for now
        error(stringc
          << "cannot find class `" << qual << "' for `" << *name << "'");
        return NULL;
      }

      // now that we've found it, that's our active scope
      scope = ct;
    }

    // advance to the next name in the sequence
    name = qualifier->rest;
  } while (name->hasQualifiers());
  
  return scope;
}


Variable *Env::lookupPQVariable(PQName const *name)
{
  if (name->hasQualifiers()) {
    // look up the scope named by the qualifiers
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) {
      // error has already been reported
      return NULL;
    }

    // look inside the final scope for the final name
    Variable *field =
      scope->lookupVariable(name->getName(), false /*innerOnly*/, *this);
    if (!field) {
      error(stringc
        << name->qualifierString() << " has no member called `"
        << name->getName() << "'");
      return NULL;
    }

    return field;
  }

  return lookupVariable(name->getName(), false /*innerOnly*/);
}

Variable *Env::lookupVariable(StringRef name, bool innerOnly)
{
  if (innerOnly) {
    return scopes.first()->lookupVariable(name, innerOnly, *this);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Variable *v = iter.data()->lookupVariable(name, innerOnly, *this);
    if (v) {
      return v;
    }
  }
  return NULL;    // not found
}

CompoundType *Env::lookupPQCompound(PQName const *name)
{
  if (name->hasQualifiers()) {
    cout << "warning: ignoring qualifiers\n";
  }

  return lookupCompound(name->getName(), false /*innerOnly*/);
}

CompoundType *Env::lookupCompound(StringRef name, bool innerOnly)
{
  if (innerOnly) {
    return scopes.first()->lookupCompound(name, innerOnly);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    CompoundType *ct = iter.data()->lookupCompound(name, innerOnly);
    if (ct) {
      return ct;
    }
  }
  return NULL;    // not found
}

EnumType *Env::lookupPQEnum(PQName const *name)
{
  if (name->hasQualifiers()) {
    cout << "warning: ignoring qualifiers\n";
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    EnumType *et = iter.data()->lookupEnum(name->getName(), false /*innerOnly*/);
    if (et) {
      return et;
    }
  }
  return NULL;    // not found
}


// -------- diagnostics --------
Type const *Env::error(char const *msg, bool disambiguates)
{
  trace("error") << (disambiguates? "[d] " : "") << "error: " << msg << endl;
  if (!disambiguateOnly || disambiguates) {
    errors.prepend(new ErrorMsg(stringc << "error: " << msg, loc(), disambiguates));
  }
  return getSimpleType(ST_ERROR);
}


Type const *Env::warning(char const *msg)
{
  trace("error") << "warning: " << msg << endl;
  if (!disambiguateOnly) {
    errors.prepend(new ErrorMsg(stringc << "warning: " << msg, loc()));
  }
  return getSimpleType(ST_ERROR);
}


Type const *Env::unimp(char const *msg)
{ 
  // always print this immediately, because in some cases I will
  // segfault (deref'ing NULL) right after printing this
  cout << "unimplemented: " << msg << endl;

  errors.prepend(new ErrorMsg(stringc << "unimplemented: " << msg, loc()));
  return getSimpleType(ST_ERROR);
}


bool Env::setDisambiguateOnly(bool newVal)
{
  bool ret = disambiguateOnly;
  disambiguateOnly = newVal;
  return ret;
}















#if 0

// --------------------- ScopedEnv ---------------------
ScopedEnv::ScopedEnv()
{}

ScopedEnv::~ScopedEnv()
{}


// --------------------------- Env ----------------------------
Env::Env(StringTable &table, CCLang &alang)
  : scopes(),
    typedefs(),
    compounds(),
    enums(),
    enumerators(),
    errors(0),
    warnings(0),
    compoundStack(),
    currentFunction(NULL),
    locationStack(),
    inPredicate(false),
    strTable(table),
    lang(alang)
{
  enterScope();

  #if 0
  declareVariable("__builtin_constant_p", DF_BUILTIN,
    makeFunctionType_1arg(
      getSimpleType(ST_INT),                          // return type
      CV_NONE,
      getSimpleType(ST_INT), "potentialConstant"),    // arg type
      true /*initialized*/);
  #endif // 0
}


Env::~Env()
{
  // explicitly free things, for easier debugging of dtor sequence
  scopes.deleteAll();
  typedefs.empty();
  compounds.empty();
  enums.empty();
  enumerators.empty();
}


void Env::enterScope()
{
  scopes.prepend(new ScopedEnv());
}

void Env::leaveScope()
{
  scopes.deleteAt(0);
}


// NOTE: the name lookup rules in this code have not been
// carefully checked against what the standard requires,
// so they are likely wrong; I intend to go through and
// make them correct at some point


// ---------------------------- variables ---------------------------
void Env::addVariable(StringRef name, Variable *decl)
{
  // convenience
  Type const *type = decl->type;
  DeclFlags flags = decl->flags;

  // shouldn't be using this interface to add typedefs
  xassert(!(flags & DF_TYPEDEF));

  // special case for adding compound type members
  if (compoundStack.isNotEmpty()) {
    addCompoundField(compoundStack.first(), decl);
    return;
  }

  Variable *prev = getVariable(name, true /*inner*/);
  if (prev) {
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
      errThrow(stringc
        << "conflicting declaration for `" << name
        << "'; previous type was `" << prev->type->toString()
        << "', this type is `" << type->toString() << "'");
    }

    // TODO: this is all wrong.. I didn't want more than one Variable
    // to exist for a given name, but this function allows more than
    // one....

    // but it's ok if:
    //   - both were functions, or
    //   - one is extern, or
    //   - both were static
    // (TODO: what are the real rules??)
    // and, there can be at most one initializer (why did I comment that out?)
    if ( ( type->isFunctionType() ||
           ((flags & DF_EXTERN) || (prev->flags & DF_EXTERN)) ||
           ((flags & DF_STATIC) && (prev->flags & DF_STATIC))
         )
         //&& (!prev->isInitialized() || !initialized)
       ) {
      // ok
      // at some point I'm going to have to deal with merging the
      // information, but for now.. skip it

      // merging for functions: pre/post
      // disabled: there's a problem because the parameter names don't
      // get symbolically evaluated right
      if (type->isFunctionType()) {
        FunctionType const *prevFn = &( prev->type->asFunctionTypeC() );
        FunctionType const *curFn = &( type->asFunctionTypeC() );

        // if a function has a prior declaration or definition, then
        // I want all later occurrences to *not* have pre/post, but
        // rather to inherit that of the prior definition.  this way
        // nobody gets to use the function before a precondition is
        // attached.  (may change this later, somehow)
        // update: now just checking..
        if (!curFn->precondition != !prevFn->precondition  || 
            !curFn->postcondition != !prevFn->postcondition) {
          warn("pre/post-condition different after first introduction");
        }

        // transfer the prior pre/post to the current one
        // NOTE: this is a problem if the names of parameters are different
        // in the previous declaration
        //decl->type = prev->type;     // since 'curFn' is const, just point 'decl' elsewhere..
      }
    }
    else {
      err(stringc << "duplicate variable decl: " << name);
    }
  }

  else /*not already mapped*/ {
    if (isGlobalEnv()) {
      decl->flags = (DeclFlags)(decl->flags | DF_GLOBAL);
    }

    scopes.first()->variables.add(name, decl);
  }
}


Variable *Env::getVariable(StringRef name, bool innerOnly)
{
  // TODO: add enums to what we search

  MUTATE_EACH_OBJLIST(ScopedEnv, scopes, iter) {
    Variable *v;
    if (iter.data()->variables.query(name, v)) {
      return v;
    }
    
    if (innerOnly) {
      return NULL;    // don't look beyond the first
    }
  }

  return NULL;
}


// ------------------- typedef -------------------------
void Env::addTypedef(StringRef name, Type const *type)
{       
  Type const *prev = getTypedef(name);
  if (prev) {
    // in C++, saying 'struct Foo { ... }' implicitly creates
    // "typedef struct Foo Foo;" -- but then in C programs
    // it's typical to do this explicitly as well.  apparently g++
    // does what I'm about to do: permit it when the typedef names
    // the same type
    if (lang.tagsAreTypes && prev->equals(type)) {
      // allow it, like g++ does
      return;
    }
    else {
      errThrow(stringc <<
        "conflicting typedef for `" << name <<
        "' as type `" << type->toCString() <<
        "'; previous type was `" << prev->toCString() <<
        "'");
    }
  }
  typedefs.add(name, const_cast<Type*>(type));
}


Type const *Env::getTypedef(StringRef name)
{
  Type *t;
  if (typedefs.query(name, t)) {
    return t;
  }
  else {
    return NULL;
  }
}


// ----------------------- compounds -------------------
CompoundType *Env::addCompound(StringRef name, CompoundType::Keyword keyword)
{
  if (name && compounds.isMapped(name)) {
    errThrow(stringc << "compound already declared: " << name);
  }

  CompoundType *ret = new CompoundType(keyword, name);
  //grabAtomic(ret);
  if (name) {
    compounds.add(name, ret);
  }

  return ret;
}


void Env::addCompoundField(CompoundType *ct, Variable *decl)
{
  if (ct->getNamedField(decl->name)) {
    errThrow(stringc << "field already declared: " << decl->name);
  }

  ct->addField(decl->name, decl->type, decl);
  decl->setFlag(DF_MEMBER);
}


CompoundType *Env::getCompound(StringRef name)
{
  CompoundType *e;
  if (name && compounds.query(name, e)) {
    return e;
  }
  else {
    return NULL;
  }
}


CompoundType *Env::getOrAddCompound(StringRef name, CompoundType::Keyword keyword)
{
  CompoundType *ret = getCompound(name);
  if (!ret) {
    return addCompound(name, keyword);
  }
  else {
    if (ret->keyword != keyword) {
      errThrow(stringc << "keyword mismatch for compound " << name);
    }
    return ret;
  }
}


// ---------------------- enums ---------------------
EnumType *Env::addEnum(StringRef name)
{
  if (name && enums.isMapped(name)) {
    errThrow(stringc << "enum already declared: " << name);
  }

  EnumType *ret = new EnumType(name);
  if (name) {
    enums.add(name, ret);
  }
  return ret;
}


EnumType *Env::getEnum(StringRef name)
{
  EnumType *ret;
  if (name && enums.query(name, ret)) {
    return ret;
  }
  else {
    return NULL;
  }
}


EnumType *Env::getOrAddEnum(StringRef name)
{
  EnumType *ret = getEnum(name);
  if (!ret) {
    return addEnum(name);
  }
  else {
    return ret;
  }
}


// ------------------ enumerators ---------------------
EnumType::Value *Env::addEnumerator(StringRef name, EnumType *et, int value,
                                    Variable *decl)
{
  if (enumerators.isMapped(name)) {
    errThrow(stringc << "duplicate enumerator: " << name);
  }

  EnumType::Value *ret = et->addValue(name, value, decl);
  enumerators.add(name, ret);
  return ret;
}


EnumType::Value *Env::getEnumerator(StringRef name)
{
  EnumType::Value *ret;
  if (enumerators.query(name, ret)) {
    return ret;
  }
  else {
    return NULL;
  }
}


// -------------------- type construction ------------------
CVAtomicType *Env::makeType(AtomicType const *atomic)
{
  return makeCVType(atomic, CV_NONE);
}


CVAtomicType *Env::makeCVType(AtomicType const *atomic, CVFlags cv)
{
  CVAtomicType *ret = new CVAtomicType(atomic, cv);
  grab(ret);
  return ret;
}


Type const *Env::applyCVToType(CVFlags cv, Type const *baseType)
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


Type const *Env::makePtrOperType(PtrOper op, CVFlags cv, Type const *type)
{
  if (type->isError()) {
    return type;
  }

  PointerType *ret = new PointerType(op, cv, type);
  grab(ret);
  return ret;
}


FunctionType *Env::makeFunctionType(Type const *retType/*, CVFlags cv*/)
{
  FunctionType *ret = new FunctionType(retType/*, cv*/);
  grab(ret);
  return ret;
}


#if 0
FunctionType *Env::makeFunctionType_1arg(
  Type const *retType, CVFlags cv,
  Type const *arg1Type, char const *arg1Name)
{
  FunctionType *ret = makeFunctionType(retType/*, cv*/);
  ret->addParam(new Parameter(arg1Type, arg1Name));
  return ret;
}    
#endif // 0


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


void Env::checkCoercible(Type const *src, Type const *dest)
{
  if (dest->asRval()->isOwnerPtr()) {
    // can only assign owners into owner owners (whereas it's
    // ok to assign an owner into a serf)
    if (!src->asRval()->isOwnerPtr()) {
      err(stringc
        << "cannot convert `" << src->toString()
        << "' to `" << dest->toString());
    }
  }

  // just say yes
}

Type const *Env::promoteTypes(BinaryOp op, Type const *t1, Type const *t2)
{
  // yes yes yes
  return t1;
}


// --------------------- error/warning reporting ------------------
Type const *Env::err(char const *str)
{
  cout << currentLoc().likeGccToString()
       << ": error: " << str << endl;
  errors++;
  return fixed(ST_ERROR);
}


void Env::warn(char const *str)
{
  cout << currentLoc().likeGccToString()
       << ": warning: " << str << endl;
  warnings++;
}


void Env::errLoc(SourceLocation const &loc, char const *str)
{
  pushLocation(&loc);
  err(str);
  popLocation();
}

void Env::warnLoc(SourceLocation const &loc, char const *str)
{
  pushLocation(&loc);
  warn(str);
  popLocation();
}


void Env::errThrow(char const *str)
{
  err(str);
  THROW(XError(str));
}


void Env::errIf(bool condition, char const *str)
{
  if (condition) {
    errThrow(str);
  }
}


// ------------------- translation context -------------------
Type const *Env::getCurrentRetType()
{
  return currentFunction->nameParams->var->type
           ->asFunctionTypeC().retType;
}


void Env::pushLocation(SourceLocation const *loc)
{ 
  locationStack.push(const_cast<SourceLocation*>(loc));
}


SourceLocation Env::currentLoc() const
{
  if (locationStack.isEmpty()) {
    return SourceLocation();      // no loc info
  }
  else {
    return *(locationStack.topC());
  }
}


// ---------------------- debugging ---------------------
string Env::toString() const
{
  stringBuilder sb;

  // for now, just the variables
  FOREACH_OBJLIST(ScopedEnv, scopes, sc) {
    StringSObjDict<Variable>::Iter iter(sc.data()->variables);
    for (; !iter.isDone(); iter.next()) {
      sb << iter.value()->toString() << " ";
    }
  }

  return sb;
}


void Env::selfCheck() const
{}


#endif // 0
