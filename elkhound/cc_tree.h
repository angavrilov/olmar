// cc_tree.h
// C++ parse tree stuff

#ifndef CC_TREE_H
#define CC_TREE_H

#include "glrtree.h"    // NonterminalNode
#include "exc.h"        // xBase
#include "cc_env.h"     // Env

class Condition_Node;
class DataflowVar;

class CCTreeNode : public NonterminalNode {
public:      // data
  // keep a pointer to the environment for interpreting
  // names that appear in this node or subtrees
  Env *env;

private:     // funcs
  // analysis helpers
  DataflowVar *getOwnerPtr(char const *name);

public:      // funcs
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

  // analysis entry points
  void ana_free(string varName);
  void ana_malloc(string varName);
  void ana_endScope(Env *localEnv);
  void ana_applyConstraint(bool negated);
  void ana_checkDeref();
};


// more analysis helpers
bool isOwnerPointer(Type const *t);


#endif // CC_TREE_H
