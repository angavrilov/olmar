// dataflow.cc
// code for dataflow.h

#include "dataflow.h"     // this module
#include "trace.h"        // trace
#include "cc_tree.h"      // isOwnerPointer


// ----------------- AbsOwnerValue -----------------------
char const *aov_name(AbsOwnerValue v)
{
  switch (v) {
    default: xfailure("bad fv code");
    #define N(val) case val: return #val;
    N(AOV_TOP)
    N(AOV_NULL)
    N(AOV_INIT)
    N(AOV_UNINIT)
    N(AOV_INITQ)
    N(AOV_BOTTOM)
    #undef N
  }
}


bool aov_geq(AbsOwnerValue v1, AbsOwnerValue v2)
{
  return (v1 & v2) == v1;
}


AbsOwnerValue aov_meet(AbsOwnerValue v1, AbsOwnerValue v2)
{
  return (AbsOwnerValue)(v1 | v2);
}

AbsOwnerValue aov_join(AbsOwnerValue v1, AbsOwnerValue v2)
{
  return (AbsOwnerValue)(v1 & v2);
}


// ----------------------- AbsValue ---------------------
AbsValue::AbsValue()
  : single(true),
    singleValue(AOV_UNINIT)
{}


AbsValue::~AbsValue()
{}


AbsValue::AbsValue(AbsValue const &obj)
{
  dup(obj);
}


AbsValue::AbsValue(AbsOwnerValue v)
  : single(true),
    singleValue(v)
{}


AbsValue::AbsValue(char const *indexVarName, AbsOwnerValue left,
                   AbsOwnerValue mid, AbsOwnerValue right)
  : single(false),
    indexVar(indexVarName),
    leftValue(left),
    midValue(mid),
    rightValue(right)
{}


void AbsValue::dup(AbsValue const &obj)
{
  CMEMB(single);
  CMEMB(singleValue);
  CMEMB(indexVar);
  CMEMB(leftValue);
  CMEMB(midValue);
  CMEMB(rightValue);
}


AbsValue AbsValue::promote(char const *indexVarName) const
{
  xassert(single);

  return AbsValue(indexVarName,
                  singleValue, singleValue, singleValue);
}


AbsValue& AbsValue::operator= (AbsValue const &obj)
{
  if (this != &obj) {
    dup(obj);
  }
  return *this;
}


// promote one or the other object in a binary operator
// to array type, so they both are, and then re-apply
// the operator 'func'
#define PROMOTION_MERGE(func)                \
  if (single && !obj.single) {               \
    return promote(obj.indexVar).func(obj);  \
  }                                          \
  else { xassert(!single && obj.single);     \
    return func(obj.promote(indexVar));      \
  }


bool AbsValue::geq(AbsValue const &obj) const
{
  if (single && obj.single) {
    return aov_geq(singleValue, obj.singleValue);
  }

  else if(!single && !obj.single) {
    return indexVar == obj.indexVar &&    // otherwise incomparable
           aov_geq(leftValue, obj.leftValue) &&
           aov_geq(midValue, obj.midValue) &&
           aov_geq(rightValue, obj.rightValue);
  }

  else PROMOTION_MERGE(geq)
}


AbsValue AbsValue::meet(AbsValue const &obj) const
{
  if (single && obj.single) {
    return aov_meet(singleValue, obj.singleValue);
  }

  else if (!single && !obj.single) {
    if (indexVar != obj.indexVar) {
      cout << "array values' meet is bottom because `"
           << indexVar << "' != `" << obj.indexVar << "'\n";
      return AOV_BOTTOM;
    }
    else {
      return AbsValue(indexVar,
                      aov_meet(leftValue, obj.leftValue),
                      aov_meet(midValue, obj.midValue),
                      aov_meet(rightValue, obj.rightValue));
    }
  }

  else PROMOTION_MERGE(meet)
}


// we'll have to see how this is used before deciding
// if its structure really should be the same as 'meet' ..
AbsValue AbsValue::join(AbsValue const &obj) const
{
  if (single && obj.single) {
    return aov_join(singleValue, obj.singleValue);
  }

  else if (!single && !obj.single) {
    if (indexVar != obj.indexVar) {
      // this is *very* fishy...
      cout << "array values' join is bottom because `"
           << indexVar << "' != `" << obj.indexVar << "'\n";
      return AOV_BOTTOM;
    }
    else {
      return AbsValue(indexVar,
                      aov_join(leftValue, obj.leftValue),
                      aov_join(midValue, obj.midValue),
                      aov_join(rightValue, obj.rightValue));
    }
  }

  else PROMOTION_MERGE(join)
}


string AbsValue::toString() const
{
  if (single) {
    return aov_name(singleValue);
  }
  else {
    return stringc << "array(" << indexVar << ": "
                   << aov_name(leftValue) << ", "
                   << aov_name(midValue) << ", "
                   << aov_name(rightValue) << ")";
  }
}


// --------------------------- DataflowEnv --------------------
DataflowEnv::DataflowEnv()
{}

DataflowEnv::DataflowEnv(DataflowEnv const &obj)
{
  *this = obj;
}

DataflowEnv::~DataflowEnv()
{}


DataflowEnv& DataflowEnv::operator= (DataflowEnv const &obj)
{              
  trace("DataflowEnv") << "operator=" << endl;

  if (this != &obj) {
    vars.empty();

    StringObjDict<DataflowVar>::Iter iter(obj.vars);
    for (; !iter.isDone(); iter.next()) {
      vars.add(iter.key(), new DataflowVar(*iter.value()));
    }
  }

  return *this;
}


void DataflowEnv::addVariable(Variable *var)
{
  trace("DataflowEnv") << "addVariable(" << var->name << ")\n";
                                                
  if (isOwnerPointer(var->type)) {
    cout << "owner pointer: " << var->name << endl;
    vars.add(var->name, new DataflowVar(var));
  }
  else if (isArrayOfOwnerPointer(var->type)) {
    cout << "array of owner pointer: " << var->name << endl;
    vars.add(var->name, new DataflowVar(var));
  }
}


void DataflowEnv::mergeWith(DataflowEnv const &obj)
{
  trace("DataflowEnv") << "mergeWith" << endl;

  StringObjDict<DataflowVar>::Iter iter(obj.vars);
  for (; !iter.isDone(); iter.next()) {
    if (vars.isMapped(iter.key())) {
      // both environments have a value; merge it
      DataflowVar *myvar = vars.queryf(iter.key());
      AbsValue tmp = 
        myvar->value .meet( iter.value()->value );

      trace("DataflowEnv")
        << "meet of " << myvar->value.toString()
        << " and " << iter.value()->value.toString()
        << " is " << tmp.toString() << endl;
        
      myvar->value = tmp;
    }
    else {
      // the 'this' environment doesn't have it, so copy 'obj's
      vars.add(iter.key(), new DataflowVar(*iter.value()));
    }
  }
}


DataflowVar *DataflowEnv::getVariable(char const *name)
{
  trace("DataflowEnv") << "getVariable(" << name << ")\n";

  if (vars.isMapped(name)) {
    return vars.queryf(name);
  }
  else {
    return NULL;
  }
}


bool DataflowEnv::haveInfoFor(char const *name) const
{
  return vars.isMapped(name);
}


void DataflowEnv::reset()
{
  trace("DataflowEnv") << "reset" << endl;
  vars.empty();
}
