// cc_tree.h
// C++ parse tree stuff

#ifndef CC_TREE_H
#define CC_TREE_H

#include "glrtree.h"    // NonterminalNode
#include "exc.h"        // xBase
#include "cc_env.h"     // Env

class Condition_Node;
class DataflowVar;
class CilExpr;
class CilInstructions;
class CilLval;

class CCTreeNode : public NonterminalNode {
public:      // data
  // keep a pointer to the environment for interpreting
  // names that appear in this node or subtrees
  Env *env;

  // yet another cute hack: if the subtree represented
  // by this node is effectively just a literal integer
  // (for example, "sizeof(int)"), we mark isJustInt
  // true and set theInt to its value
  bool isJustInt;
  int theInt;

private:     // funcs
  // analysis helpers
  DataflowVar *getOwnerPtr(char const *name);

public:      // funcs
  CCTreeNode(Reduction *red)
    : NonterminalNode(red), env(NULL),
      isJustInt(false), theInt(-1) {}
  virtual ~CCTreeNode();

  // do the declaration, and report an error if it happens
  void declareVariable(Env *env, char const *name,
                       DeclFlags flags, Type const *type) const;

  // attempt at a general disambiguator...
  typedef CilExpr * (CCTreeNode::*DisambFn)(Env *env, CilInstructions &inst);
  CilExpr *disambiguate(Env *passedEnv, CilInstructions &inst, DisambFn func);

  // construct a general error, and throw it
  void throwError(char const *msg) const NORETURN;
  void throwError(SemanticError const &x) const NORETURN;

  // make a general error but simply report it
  void reportError(Env *env, char const *msg) const;

  // throw an internal error
  void internalError(char const *msg) const NORETURN;

  // if the value is null, complain about void rvalue
  CilExpr * /*owner*/ asRval(CilExpr * /*owner*/ expr, 
                             CCTreeNode const &exprSyntax) const;

  // convert to lval if it is, otherwise complain
  CilLval * /*owner*/ asLval(CilExpr * /*owner*/ expr, 
                             CCTreeNode const &exprSyntax) const;

  // set the "just an int" value
  void setTheInt(int val) { isJustInt=true; theInt=val; }

  // analysis entry points
  void ana_free(string varName);
  void ana_malloc(string varName);
  void ana_endScope(Env *localEnv);
  void ana_applyConstraint(bool negated);
  void ana_checkDeref();
  
  // NonterminalNode funcs
  virtual bool getIsJustInt() const { return isJustInt; }
};


// more analysis helpers
bool isOwnerPointer(Type const *t);
bool isArrayOfOwnerPointer(Type const *t);


#endif // CC_TREE_H
