// lookupset.cc
// code for lookupset.h

#include "lookupset.h"        // this module
#include "variable.h"         // Variable, sameEntity
#include "template.h"         // TemplateInfo


string toString_LF(LookupFlags flags) {
  stringBuilder sb;

  // check for undefined flags
  if (flags & ~LF_ALL_FLAGS) {
    sb << "ILLEGAL FLAG";
    return sb;
  }

#define CHECK_FLAG(FLAG) if (flags & FLAG) {sb << #FLAG; sb << " ";}

  CHECK_FLAG(LF_INNER_ONLY)
  CHECK_FLAG(LF_ONLY_TYPES)
  CHECK_FLAG(LF_TYPENAME)
  CHECK_FLAG(LF_SKIP_CLASSES)
  CHECK_FLAG(LF_ONLY_NAMESPACES)
  CHECK_FLAG(LF_TYPES_NAMESPACES)
  CHECK_FLAG(LF_QUALIFIED)
  CHECK_FLAG(LF_TEMPL_PRIMARY)
  CHECK_FLAG(LF_FUNCTION_NAME)
  CHECK_FLAG(LF_DECLARATOR)
  CHECK_FLAG(LF_SELFNAME)
  CHECK_FLAG(LF_DEPENDENT)
  CHECK_FLAG(LF_TEMPL_PARAM)
  CHECK_FLAG(LF_SUPPRESS_ERROR)
  CHECK_FLAG(LF_SUPPRESS_NONEXIST)
  CHECK_FLAG(LF_IGNORE_USING)
  CHECK_FLAG(LF_NO_IMPL_THIS)
  CHECK_FLAG(LF_LOOKUP_SET)
  CHECK_FLAG(LF_QUERY_TAGS)
  CHECK_FLAG(LF_NO_DENOTED_SCOPE)
  CHECK_FLAG(LF_EXPECTING_TYPE)
  CHECK_FLAG(LF_EXPLICIT_INST)

#undef CHECK_FLAG

  return sb;
}


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
    Variable *v = mut.data();

    if (v->isTemplate()) {
      mut.adv();     // keep it
      continue;
    }
    
    // 2005-04-17: in/t0055.cc:  The context is a lookup that has
    // template arguments.  It might be that we found a selfname in
    // the scope of a template class instantiation; but if template
    // args are used, then the user is intending to apply args to the
    // original template.  (The code here is an approximation of what
    // is specified in 14.6p2b.)
    if (v->hasFlag(DF_SELFNAME)) {
      // argh.. my selfnames do not have proper template info, so
      // I have to go through the type (TODO: fix this)
      CompoundType *ct = v->type->asCompoundType();
      TemplateInfo *tinfo = ct->templateInfo();
      if (tinfo != NULL &&
          tinfo->isCompleteSpecOrInstantiation()) {
        // replace 'v' with a pointer to the template primary
        mut.dataRef() = tinfo->getPrimary()->var;
        mut.adv();
        continue;
      }
    }

    mut.remove();    // filter it out
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
