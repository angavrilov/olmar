// dataflow.cc
// code for dataflow.h

#include "dataflow.h"     // this module
#include "trace.h"        // trace


char const *fv_name(FlowValue v)
{
  switch (v) {
    default: xfailure("bad fv code");
    #define N(val) case val: return #val;
    N(FV_TOP)
    N(FV_NULL)
    N(FV_INIT)
    N(FV_UNINIT)
    N(FV_INITQ)
    N(FV_BOTTOM)
    #undef N
  }
}


// is v1 >= v2?
// i.e., is there a path from v1 down to v2 in the lattice?
// e.g.:
//   top >= init
//   null >= init?
//   init >= init
// but NOT:
//   init >= uninit
bool fv_geq(FlowValue v1, FlowValue v2)
{
  return (v1 & v2) == v1;
}


// combine info from two merging control branches
FlowValue fv_meet(FlowValue v1, FlowValue v2)
{
  return (FlowValue)(v1 | v2);
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


void DataflowEnv::mergeWith(DataflowEnv const &obj)
{
  trace("DataflowEnv") << "mergeWith" << endl;

  StringObjDict<DataflowVar>::Iter iter(obj.vars);
  for (; !iter.isDone(); iter.next()) {
    if (vars.isMapped(iter.key())) {
      // both environments have a value; merge it
      DataflowVar *myvar = vars.queryf(iter.key());
      FlowValue tmp =
        fv_meet(myvar->value, iter.value()->value);
        
      trace("DataflowEnv") 
        << "meet of " << fv_name(myvar->value)
        << " and " << fv_name(iter.value()->value)
        << " is " << fv_name(tmp) << endl;
        
      myvar->value = tmp;
    }
    else {
      // the 'this' environment doesn't have it, so copy 'obj's
      vars.add(iter.key(), new DataflowVar(*iter.value()));
    }
  }
}


DataflowVar *DataflowEnv::getVariable(Env *env, char const *name)
{
  trace("DataflowEnv") << "getVariable(" << name << ")\n";

  if (vars.isMapped(name)) {
    return vars.queryf(name);
  }
  else {
    DataflowVar *ret = new DataflowVar(env->getVariable(name));
    vars.add(name, ret);
    return ret;
  }
}


void DataflowEnv::reset()
{
  trace("DataflowEnv") << "reset" << endl;
  vars.empty();
}
