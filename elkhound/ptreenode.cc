// ptreenode.cc
// code for ptreenode.h

#include "ptreenode.h"      // this module

TreeCount PTreeNode::countTrees()
{
  // memoize to avoid exponential blowup
  if (count != 0) {
    return count;
  }
  
  if (type == PTREENODE_MERGE) {                       
    // a single tree can take from any one of its children,
    // but not more than one simultaneously
    count = 0;     // redundant, actually
    for (int i=0; i<numChildren; i++) {
      count += children[i]->countTrees();
    }
  }

  else {
    // a single tree can have any possibility for each of
    // its children, so the result is their product
    count = 1;
    for (int i=0; i<numChildren; i++) {
      count *= children[i]->countTrees();
    }
  }

  return count;
}


