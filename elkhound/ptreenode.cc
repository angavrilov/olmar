// ptreenode.cc
// code for ptreenode.h

#include "ptreenode.h"      // this module
#include "typ.h"            // STATICDEF

int PTreeNode::allocCount = 0;
int PTreeNode::alternativeCount = 0;


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


void PTreeNode::printTree(ostream &out) const
{
  innerPrintTree(out, 0 /*indentation*/);
}


// amount to indent per level
enum { INDENT_INC = 2 };

void PTreeNode::innerPrintTree(ostream &out, int indentation) const
{
  if (merged) {
    // this is an ambiguity node
    indent(out, indentation);
    out << "--------- ambiguity node: " << countMergedList()
        << " parses ---------\n";
    indentation += INDENT_INC;
  }

  // iterate over interpretations
  for (PTreeNode const *n = this; n != NULL; n = n->merged) {
    indent(out, indentation);
    out << n->type << "\n";

    // iterate over children
    for (int c=0; c < n->numChildren; c++) {
      // recursively print children
      n->children[c]->innerPrintTree(out, indentation + INDENT_INC);
    }
  }

  if (merged) {
    // close up ambiguity display
    indentation -= INDENT_INC;
    indent(out, indentation);
    out << "--------- end of ambiguity ---------\n";
  }
}

STATICDEF void PTreeNode::indent(ostream &out, int n)
{
  for (int i=0; i<n; i++) {
    out << " ";
  }
}

// # of nodes on the 'merged' list; always at least 1 since
// 'this' is considered to be in that list
int PTreeNode::countMergedList() const
{
  int ct = 1;
  for (PTreeNode const *n = merged; n != NULL; n = n->merged) {
    ct++;
  }
  return ct;
}


void PTreeNode::addAlternative(PTreeNode *alt)
{
  // insert as 2nd element
  alt->merged = this->merged;
  this->merged = alt;
  
  alternativeCount++;
}
