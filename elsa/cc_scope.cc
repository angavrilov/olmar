// cc_scope.cc            see license.txt for copyright and terms of use
// code for cc_scope.h

#include "cc_scope.h"     // this module
#include "trace.h"        // trace
#include "variable.h"     // Variable
#include "cc_type.h"      // CompoundType

Scope::Scope(int cc, SourceLocation const &initLoc)
  : variables(),
    compounds(),
    enums(),
    changeCount(cc),
    curCompound(NULL),
    curFunction(NULL),
    curLoc(initLoc)
{}

Scope::~Scope()
{}


// -------- insertion --------
template <class T>
bool insertUnique(StringSObjDict<T> &table, char const *key, T *value,
                  int &changeCount)
{
  if (table.isMapped(key)) {
    return false;
  }
  else {
    table.add(key, value);
    changeCount++;
    return true;
  }
}


bool Scope::addVariable(Variable *v)
{
  if (!curCompound) {
    // variable outside a class
    trace("env") << "added variable `" << v->name
                 << "' of type `" << v->type->toString()
                 << "' at " << v->loc.toString() << endl;
  }
  else {
    // class member
    v->access = curAccess;
    trace("env") << "added " << toString(v->access)
                 << " field `" << v->name
                 << "' of type `" << v->type->toString()
                 << "' at " << v->loc.toString()
                 << " to " << curCompound->keywordAndName() << endl;
  }

  return insertUnique(variables, v->name, v, changeCount);
}


bool Scope::addCompound(CompoundType *ct)
{
  trace("env") << "added " << toString(ct->keyword) << " " << ct->name << endl;

  ct->access = curAccess;
  return insertUnique(compounds, ct->name, ct, changeCount);
}


bool Scope::addEnum(EnumType *et)
{
  trace("env") << "added enum " << et->name << endl;

  et->access = curAccess;
  return insertUnique(enums, et->name, et, changeCount);
}


// -------- lookup --------
Variable const *Scope::lookupVariableC(StringRef name) const
{
  return variables.queryif(name);
}

CompoundType const *Scope::lookupCompoundC(StringRef name) const
{
  return compounds.queryif(name);
}

EnumType const *Scope::lookupEnumC(StringRef name) const
{
  return enums.queryif(name);
}
