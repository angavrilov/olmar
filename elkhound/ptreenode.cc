// ptreenode.cc
// code for ptreenode.h

#include "ptreenode.h"      // this module

int PTreeNode::allocCount = 0;


void PTreeNode::init()
{ 
  merged = NULL;
  allocCount++;
}


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
    
    // are there alternatives?
    if (merged) {
      // add them too (recurse down the list of alts)
      count += merged->countTrees();
    }
  }

  return count;
}


void PTreeNode::addAlternative(PTreeNode *alt)
{
  // insert as 2nd element
  alt->merged = this->merged;
  this->merged = alt;
}
