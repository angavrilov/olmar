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

  checkHomomorphism();

  // no!  See checkHomomorphism, below.
  //xassert(!v->q);
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

                                                      
// this is defined in cc_tcheck.cc.. bit of a hack
// but there's not a great place to declare it, and I'm
// not sure whether it's precisely what I want
bool almostEqualTypes(Type const *t1, Type const *t2);

void Variable_Q::checkHomomorphism() const
{
  // annotation relation is 1-1
  //xassert(var->q == this);
  //
  // update: No it's not!  The whole point of having Variable_Q
  // be a separate object is to allow for the possibility of
  // having more than one Variable_Q per Variable.  And in fact,
  // the deepClone of function types causes exactly this, when
  // it clones the parameters.
  //
  // That calls into question the wisdom of having a pointer in
  // Variable to Variable_Q, of course.  But that's the way it
  // worked before: the AST nodes had a single Variable pointer,
  // even when they were being cloned.  So there may well be a
  // design error here, but I didn't introduce it by separating
  // Variable into Variable and Variable_Q.

  // our notion of the variable's type should correspond with
  // the C++ tcheck's notion of its type (except we allow
  // some variance in array size matching)
  xassert(almostEqualTypes(qtype->type(), var->type));

  // and that type should be self-consistent
  qtype->checkHomomorphism();
}
