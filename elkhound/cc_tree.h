// cc_tree.h
// C++ parse tree stuff

#ifndef CC_TREE_H
#define CC_TREE_H

#include "glrtree.h"    // NonterminalNode
#include "exc.h"        // xBase


class CCTreeNode : public NonterminalNode {
public:
  CCTreeNode(Reduction *red) : NonterminalNode(red) {}
  
  void error(char const *msg) const NORETURN;
};


// exception thrown when a problem is detected while executing
// semantic functions associated with tree nodes
class XTreeError : public xBase {
public:     // data
  // node where the error was detected; useful for printing
  // location in input file
  CCTreeNode const *node;

  // what is wrong
  string message;

private:    // funcs
  static string makeMsg(CCTreeNode const *n, char const *m);

public:     // funcs
  XTreeError(CCTreeNode const *n, char const *m);
  XTreeError(XTreeError const &obj);
  ~XTreeError();
};


#endif // CC_TREE_H
