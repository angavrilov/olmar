// ptreenode.h
// parse tree node for experimental grammars (this isn't somthing
// Elkhound as a whole knows about--it doesn't make trees unless
// the user actions do)

#ifndef PTREENODE_H
#define PTREENODE_H

#include <stddef.h>     // NULL

// for storing counts of parse trees; I try to make the code work for
// either 'int' or 'double' in this spot (e.g. I assign 0 to it
// instead of 0.0), even though 'int' overflows quickly for the highly
// ambiguous grammars
typedef double TreeCount;

// max # of children
enum { MAXCHILDREN = 4 };

class PTreeNode {
public:
  // textual repr. of the production applied; possibly useful for
  // printing the tree, or during debugging; (hack) if it is NULL,
  // then this node is an ambiguity merge point; not otherwise
  // interpreted by code which manipulates these trees
  char const *type;
  #define PTREENODE_MERGE NULL

  // alternative merge scheme: instead of making explicit merge nodes
  // (which runs afoul of the yield-then-merge problem), just link
  // alternatives together using this link; this is NULL when there
  // are no alternatives, or for the last node in a list of alts
  PTreeNode *merged;

  // array of children; these aren't owner pointers because
  // we might have arbitrary sharing for some grammars
  int numChildren;
  PTreeNode *children[MAXCHILDREN];

  // # of parse trees of which this is the root; effectively this
  // memoizes the result to avoid an exponential blowup counting
  // the trees; when this value is 0, it means the count has not
  // yet been computed (any count must be positive)
  TreeCount count;

  // count of # of allocated nodes; useful for identifying when
  // we're making too many
  static int allocCount;
  
private:     // funcs
  // init fields which don't depend on ctor args
  void init();

public:      // funcs
  // now lots of constructors so we have one for each possible
  // number of children; the calls are automatically inserted
  // by a perl script ('make-trivparser.pl')
  PTreeNode(char const *t)
    : type(t), numChildren(0), count(0) { init(); }
  PTreeNode(char const *t, PTreeNode *ch0)
    : type(t), numChildren(1), count(0) { init(); children[0] = ch0; }
  PTreeNode(char const *t, PTreeNode *ch0, PTreeNode *ch1)
    : type(t), numChildren(2), count(0) { init(); children[0] = ch0; children[1] = ch1; }
  PTreeNode(char const *t, PTreeNode *ch0, PTreeNode *ch1, PTreeNode *ch2)
    : type(t), numChildren(3), count(0) { init(); children[0] = ch0; children[1] = ch1; children[2] = ch2; }
  PTreeNode(char const *t, PTreeNode *ch0, PTreeNode *ch1, PTreeNode *ch2, PTreeNode *ch3)
    : type(t), numChildren(4), count(0) { init(); children[0] = ch0; children[1] = ch1; children[2] = ch2; children[3] = ch3; }

  ~PTreeNode() { allocCount--; }

  // count the number of trees encoded (taking merge nodes into
  // account) in the tree rooted at 'this'
  TreeCount countTrees();
  
  // add an alternative to the current 'merged' list
  void addAlternative(PTreeNode *alt);
};

#endif // PTREENODE_H
