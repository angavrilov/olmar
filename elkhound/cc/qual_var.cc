// qual_var.cc
// code for qual_var.h

#include "qual_var.h"      // this module
#include "variable.h"      // Variable
#include "qual_type.h"     // Type, Type_Q

#include <iostream.h>      // cout


SObjList<Variable_Q> Variable_Q::instances;


Variable_Q::Variable_Q(SourceLocation const &L, StringRef n,
                       Type_Q *t, DeclFlags f,
                       bool put_into_instances_list)
  : Variable(L, n, t->type(), f)
{
  // sm: dsw's modifications to variable.cc didn't actually do
  // this check, but I'm assuming he intended to
  if (put_into_instances_list) {
    instances.prepend(this);
  }
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

  int index = instances.indexOf(this);
  if (index >= 0) {
    instances.removeAt(index);
  }
  else {
    // put_into_instances_list must have been false on creation
  }
}


Type_Q const *Variable_Q::typeC() const
{
  return asTypeC_Q(Variable::type);
}
