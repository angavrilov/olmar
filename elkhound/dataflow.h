// dataflow.h
// data structures, etc. for dataflow analysis

#ifndef DATAFLOW_H
#define DATAFLOW_H

#include "strobjdict.h"   // StringObjDict
#include "cc_env.h"       // Variable, Env

class Type;               // cc_type.h

// lattice of values
//
//          top: can't have any value because 
//               this path isn't possible
//                  i.e. {}                     .
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
  FV_TOP             = 0,
  
  // basis elements
  FV_NULL            = 1,
  FV_INIT            = 2,
  FV_UNINIT_NOT_NULL = 4,

  FV_INITQ           = 3,    // FV_NULL | FV_INIT
  FV_UNINIT          = 5,    // uninit; can be NULL
  FV_NOT_NULL        = 6,    // init or uninit-null
  FV_BOTTOM          = 7,    // FV_INIT | FV_UNINIT
};                                     

// e.g., fv_name(FV_TOP) = "FV_TOP"
char const *fv_name(FlowValue v);

// is v1 >= v2?
// i.e., is there a path from v1 down to v2 in the lattice?
// e.g.:
//   top >= init
//   null >= init?
//   init >= init
// but NOT:
//   init >= uninit
bool fv_geq(FlowValue v1, FlowValue v2);

// combine info from two merging control branches
FlowValue fv_meet(FlowValue v1, FlowValue v2);

// intersect info, e.g. intersect the values some variable
// has now with the set of possible values for entering a
// branch of an 'if' statement
FlowValue fv_join(FlowValue v1, FlowValue v2);


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

  // true if we have any information about the named variable
  bool haveInfoFor(char const *name) const;

  // throw away all info
  void reset();
};


#endif // DATAFLOW_H
