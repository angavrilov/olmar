// cc_tree.h
// C++ parse tree stuff

#ifndef CC_TREE_H
#define CC_TREE_H

#include "glrtree.h"    // NonterminalNode
#include "exc.h"        // xBase
#include "cc_env.h"     // Env


class CCTreeNode : public NonterminalNode {
public:    // vars
  // keep a pointer to the environment for interpreting
  // names that appear in this node or subtrees
  Env *env;

public:
  CCTreeNode(Reduction *red) 
    : NonterminalNode(red), env(NULL) {}

  // do the declaration, and report an error if it happens
  void declareVariable(Env *env, char const *name,
                       DeclFlags flags, Type const *type) const;

  // attempt #1 at a general disambiguator...
  typedef void (CCTreeNode::*DisambFn)(Env *env);
  void disambiguate(Env *passedEnv, DisambFn func);

  // construct a general error, and throw it
  void throwError(char const *msg) const NORETURN;

  // make a general error but simply report it
  void reportError(Env *env, char const *msg) const;

  // throw an internal error
  void internalError(char const *msg) const NORETURN;
};


#endif // CC_TREE_H
