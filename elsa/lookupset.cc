// lookupset.cc
// code for lookupset.h

#include "lookupset.h"        // this module
#include "variable.h"         // Variable, sameEntity


// vfilter: variable filter
// implements variable-filtering aspect of the flags; the idea
// is you never query 'variables' without wrapping the call
// in a filter
Variable *vfilter(Variable *v, LookupFlags flags)
{
  if (!v) return v;

  if ((flags & LF_ONLY_TYPES) &&
      !v->hasFlag(DF_TYPEDEF)) {
    return NULL;
  }

  if ((flags & LF_ONLY_NAMESPACES) &&
      !v->hasFlag(DF_NAMESPACE)) {
    return NULL;
  }

  if ((flags & LF_TYPES_NAMESPACES) &&
      !v->hasFlag(DF_TYPEDEF) &&
      !v->hasFlag(DF_NAMESPACE)) {
    return NULL;
  }

  if (!(flags & LF_SELFNAME) &&
      v->hasFlag(DF_SELFNAME)) {
    // the selfname is not visible b/c LF_SELFNAME not specified
    return NULL;
  }
                                                        
  if ((flags & LF_TEMPL_PARAM) &&
      !v->isTemplateParam()) {
    return NULL;
  }

  return v;
}


// --------------------- LookupSet ----------------------
LookupSet::LookupSet()
{}

LookupSet::~LookupSet()
{}


LookupSet::LookupSet(LookupSet const &obj)
{
  copy(obj);
}

LookupSet& LookupSet::operator= (LookupSet const &obj)
{
  if (this != &obj) {
    copy(obj);
  }
  return *this;
}

void LookupSet::copy(LookupSet const &obj)
{
  removeAll();
  SObjList<Variable>::operator=(obj);
}


Variable *LookupSet::filter(Variable *v, LookupFlags flags)
{
  v = vfilter(v, flags);
  if (v) {
    addsIf(v, flags);
  }
  return v;
}


void LookupSet::adds(Variable *v)
{
  if (v->isOverloaded()) {
    SFOREACH_OBJLIST_NC(Variable, v->overload->set, iter) {
      add(iter.data());
    }
  }
  else {
    add(v);
  }
}

void LookupSet::add(Variable *v)
{
  // is 'v' already present?
  SFOREACH_OBJLIST(Variable, *this, iter) {
    if (sameEntity(v, iter.data())) {
      return;      // already present
    }
  }

  // not already present, add it
  prepend(v);
}


void LookupSet::addIf(Variable *v, LookupFlags flags)
{
  if (flags & LF_LOOKUP_SET) {
    add(v);
  }
}

void LookupSet::addsIf(Variable *v, LookupFlags flags)
{
  if (flags & LF_LOOKUP_SET) {
    adds(v);
  }
}


void LookupSet::removeAllButOne()
{
  while (count() > 1) {
    removeFirst();
  }
}


void LookupSet::removeNonTemplates()
{
  SObjListMutator<Variable> mut(*this);
  while (!mut.isDone()) {
    if (mut.data()->isTemplate()) {
      mut.adv();     // keep it
    }
    else {
      mut.remove();  // filter it out
    }
  }
}


string LookupSet::asString() const
{
  if (isEmpty()) {
    return "";
  }

  // are all the names in the same scope?
  Scope *scope = firstC()->scope;
  SFOREACH_OBJLIST(Variable, *this, iter1) {
    Variable const *v = iter1.data();

    if (v->scope != scope) {
      scope = NULL;
    }
  }

  stringBuilder sb;

  SFOREACH_OBJLIST(Variable, *this, iter2) {
    Variable const *v = iter2.data();

    sb << "  " << v->loc << ": ";
    if (scope) {
      sb << v->toString();      // all same scope, no need to prin it
    }
    else {
      sb << v->toQualifiedString();
    }
    sb << "\n";
  }

  return sb;
}


void LookupSet::gdb() const
{
  cout << asString();
}


// EOF
