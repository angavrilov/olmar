// qual_var.h
// Variable_Q, a C++Qual-aware parallel to Variable

#ifndef QUAL_VAR_H
#define QUAL_VAR_H

#include "sobjlist.h"     // SObjList

class Variable;           // variable.h
class Type_Q;             // qual_type.h

class Variable_Q {
public:               
  // the corresponding Variable that this annotates;
  // invariant: var->q == this
  Variable *var;            // (serf)

  // Q++Qual-aware type of this variable
  Type_Q *qtype;            // (serf)

  // global list of all instances
  static SObjList<Variable_Q> instances;

public:
  Variable_Q(Variable *v, Type_Q *t);
  ~Variable_Q();

  Variable_Q *deepClone() const;

  // check the homomorphism invariants, failing an assertion
  // if they don't hold
  void checkHomomorphism() const;
};

#endif // QUAL_VAR_H
