// kandr.cc            see license.txt for copyright and terms of use
// support routines for the K&R extension

#include "cc_ast.h"         // AST
#include "generic_aux.h"    // genericSetNext


D_func *D_name::getD_func() {return NULL;}
D_func *D_pointer::getD_func() {return base->getD_func();}
D_func *D_reference::getD_func() {return base->getD_func();}
D_func *D_func::getD_func() {
  // you have to be careful that there may be another one nested down
  // inside if the user used this funky syntax:
  // int (*oink2(x))(int)
  D_func *df = base->getD_func();
  if (df) return df;
  else return this;
}
D_func *D_array::getD_func() {return base->getD_func();}
D_func *D_bitfield::getD_func() {return NULL;}
D_func *D_ptrToMember::getD_func() {return base->getD_func();}
D_func *D_grouping::getD_func() {return base->getD_func();}


void PQ_name::setNext(PQ_name *newNext)
{
  next = newNext;
}


// EOF
