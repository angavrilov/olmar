// cexp3mrg.cc
// code to merge alternatives for cexp3ast.ast

#include "cexp3ast.gen.h"     // Exp
#include "xassert.h"          // xfailure

// this code is used to select between two competing interpretations
// for the same sequence of ground terminals
STATICDEF Exp *Exp::mergeAlts(Exp *p1, Exp *p2)
{
  // look at the operators
  if (p1->op != p2->op) {
    // they are different; keep the one with '+' at the
    // top, as this is the lower-precedence operator
    if (p1->op == '+') {
      delete p2;
      return p1;
    }
    else {
      delete p1;
      return p2;
    }
  }

  // same operators; decide using associativity:
  // for left associativity, we want the left subtree to be larger
  int nodes1 = p1->left->numNodes();
  int nodes2 = p2->left->numNodes();
  if (nodes1 != nodes2) {
    if (nodes1 > nodes2) {
      delete p2;
      return p1;
    }
    else {
      delete p1;
      return p2;
    }
  }

  // it should never be possible for two competing interpretations
  // to have the same operators and the same tree sizes
  xfailure("competing trees are too similar");
  return NULL;    // silence warning
}
