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

enum AbsOwnerValue {
  AOV_TOP             = 0,
  
  // basis elements
  AOV_NULL            = 1,
  AOV_INIT            = 2,
  AOV_UNINIT_NOT_NULL = 4,

  AOV_INITQ           = 3,    // AOV_NULL | AOV_INIT
  AOV_UNINIT          = 5,    // uninit; can be NULL
  AOV_NOT_NULL        = 6,    // init or uninit-null
  AOV_BOTTOM          = 7,    // AOV_INIT | AOV_UNINIT
};                                     

// e.g., fv_name(AOV_TOP) = "AOV_TOP"
char const *fv_name(AbsOwnerValue v);

// is v1 >= v2?
// i.e., is there a path from v1 down to v2 in the lattice?
// e.g.:
//   top >= init
//   null >= init?
//   init >= init
// but NOT:
//   init >= uninit
bool fv_geq(AbsOwnerValue v1, AbsOwnerValue v2);

// combine info from two merging control branches
AbsOwnerValue fv_meet(AbsOwnerValue v1, AbsOwnerValue v2);

// intersect info, e.g. intersect the values some variable
// has now with the set of possible values for entering a
// branch of an 'if' statement
AbsOwnerValue fv_join(AbsOwnerValue v1, AbsOwnerValue v2);


// dataflow info about a variable, at some program point
class DataflowVar {
private:   // data
  Variable const *var;      // associated declaration

public:    // data
  AbsOwnerValue value;

public:    // funcs
  DataflowVar(Variable const *v)
    : var(v), value(AOV_UNINIT) {}
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

  // add a new variable to track; must not collide with an existing value
  void addVariable(Variable *var);

  // use the dataflow 'meet' operator to combine this dataflow
  // mapping with another (e.g. at the end of an if-then-else)
  void mergeWith(DataflowEnv const &obj);

  // get the structure that records info about a given name;
  // return NULL if we don't have any info for this name
  DataflowVar *getVariable(char const *name);

  // true if we have any information about the named variable
  bool haveInfoFor(char const *name) const;

  // throw away all info
  void reset();
};


#endif // DATAFLOW_H
