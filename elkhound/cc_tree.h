// cc_tree.h
// C++ parse tree stuff

#ifndef CC_TREE_H
#define CC_TREE_H

#include "glrtree.h"    // NonterminalNode
#include "exc.h"        // xBase
#include "cc_env.h"     // Env


class CCTreeNode : public NonterminalNode {
public:
  CCTreeNode(Reduction *red) : NonterminalNode(red) {}

  // do the declaration, and report an error if it happens
  void declareVariable(Env &env, char const *name,
                       DeclFlags flags, Type const *type) const;
                                                   
  // attempt #1 at a general disambiguator...
  typedef void (CCTreeNode::*DisambFn)(Env &env) const;
  void disambiguate(Env &env, DisambFn func) const;

  // construct a general error, and throw it
  void throwError(char const *msg) const NORETURN;

  // make a general error but simply report it
  void reportError(Env &env, char const *msg) const;

};


#endif // CC_TREE_H
