// qual_var.cc
// code for qual_var.h

#include "qual_var.h"      // this module
#include "variable.h"      // Variable
#include "qual_type.h"     // Type, Type_Q

#include <iostream.h>      // cout


SObjList<Variable_Q> Variable_Q::instances;


Variable_Q::Variable_Q(Variable *v, Type_Q *t)
  : var(v), qtype(t)
{
  instances.prepend(this);
}

Variable_Q::~Variable_Q()
{
  // linear time removal so quadratic time removal of all elements..
  // since for now we never delete Variable_Qs it won't matter, but
  // if at some point we do then a doubly-linked list should be used
  // instead of SObjList

  static bool first = true;
  if (first) {
    // so we notice if these things start getting deleted
    cout << "hey!  there's a performance problem in " << __FILE__ << endl;
    first = false;
  }

  instances.removeAt(instances.indexOf(this));
}


Variable_Q *Variable_Q::deepClone() const
{
  return new Variable_Q(var, qtype->deepClone());
}


void Variable_Q::checkHomomorphism() const
{
  // annotation relation is 1-1
  xassert(var->q == this);
  
  // our notion of the variable's type should correspond with
  // the C++ tcheck's notion of its type
  xassert(qtype->type() == var->type);

  // and that type should be self-consistent
  qtype->checkHomomorphism();
}
