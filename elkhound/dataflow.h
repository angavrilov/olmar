// dataflow.h
// data structures, etc. for dataflow analysis

#ifndef DATAFLOW_H
#define DATAFLOW_H

#include "strobjdict.h"   // StringObjDict
#include "cc_env.h"       // Variable, Env

class Type;               // cc_type.h

// lattice of values
//
//          top: not seen any value = {}                     .
//              /   /     \                                  .
//             /   /       \                                 .
//            /   /         \                    ^           .
//           /  null        init                 |           .
//          /  /    \       /                    | more      .
//         /  /      \     /                     | precise   .
//        /  /        \   /                      | info      .
//       uninit        init? = init U null       |           .
//           \          /                                    .
//            \        /                                     .
//        bottom: could be anything                          .
//          uninit U init U null                             .

enum FlowValue {
  FV_TOP   =0,
  FV_NULL  =1,
  FV_INIT  =2,
  FV_UNINIT=5,          // uninit; can be NULL
  FV_INITQ =3,          // FV_NULL | FV_INIT
  FV_BOTTOM=7,          // FV_INIT | FV_UNINIT
};                                     

char const *fv_name(FlowValue v);
bool fv_geq(FlowValue v1, FlowValue v2);
FlowValue fv_meet(FlowValue v1, FlowValue v2);


// dataflow info about a variable, at some program point
class DataflowVar {
private:   // data
  Variable const *var;      // associated declaration

public:    // data
  FlowValue value;

public:    // funcs
  DataflowVar(Variable const *v)
    : var(v), value(FV_UNINIT) {}
  DataflowVar(DataflowVar const &obj)
    : DMEMB(var), DMEMB(value) {}

  char const *getName() const { return var->name; }
  Type const *getType() const { return var->type; }
};


// mapping from names to dataflow values
class DataflowEnv {
private:
  StringObjDict<DataflowVar> vars;

public:
  DataflowEnv();                           // empty env
  DataflowEnv(DataflowEnv const &obj);     // copy ctor
  ~DataflowEnv();

  DataflowEnv& operator= (DataflowEnv const &obj);

  // use the dataflow 'meet' operator to combine this dataflow
  // mapping with another (e.g. at the end of an if-then-else)
  void mergeWith(DataflowEnv const &obj);

  // get the structure that records info about a given name;
  // if we don't have any info, assume it's a new name, and make
  // up new info based on what's in 'env' (where the name must
  // be declared)
  DataflowVar *getVariable(Env *env, char const *name);
  
  // throw away all info
  void reset();
};


#endif // DATAFLOW_H
