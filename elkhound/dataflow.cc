// dataflow.cc
// code for dataflow.h

#include "dataflow.h"     // this module
#include "trace.h"        // trace
#include "cc_tree.h"      // isOwnerPointer


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
      AbsOwnerValue tmp =
        aov_meet(myvar->value, iter.value()->value);
        
      trace("DataflowEnv") 
        << "meet of " << aov_name(myvar->value)
        << " and " << aov_name(iter.value()->value)
        << " is " << aov_name(tmp) << endl;
        
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
