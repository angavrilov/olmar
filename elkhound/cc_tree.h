// cc_tree.h
// C++ parse tree stuff

#ifndef CC_TREE_H
#define CC_TREE_H

#include "glrtree.h"    // NonterminalNode
#include "exc.h"        // xBase
#include "cc_env.h"     // Env
#include "cil.h"        // Cil constructors

class Condition_Node;
class DataflowVar;

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
                       DeclFlags flags, Type const *type,
                       bool initialized) const;

  // attempt at a general disambiguator...
  typedef CilExpr * (CCTreeNode::*DisambFn)(Env *env, CilContext const &ctxt);
  CilExpr *disambiguate(Env *passedEnv, CilContext const &ctxt, DisambFn func);

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
  
  // Cil constructors where the tree node is implicit
  #define MKCILCTOR0(rettype,name) \
    rettype *name()                \
      { return ::name(this); }
  #define MKCILCTOR1(rettype,name,arg1type,arg1name) \
    rettype *name(arg1type arg1name)                 \
      { return ::name(this, arg1name); }
  #define MKCILCTOR2(rettype,name,arg1type,arg1name,arg2type,arg2name) \
    rettype *name(arg1type arg1name, arg2type arg2name)                \
      { return ::name(this, arg1name, arg2name); }
  #define MKCILCTOR3(rettype,name,arg1type,arg1name,arg2type,arg2name,arg3type,arg3name) \
    rettype *name(arg1type arg1name, arg2type arg2name, arg3type arg3name)               \
      { return ::name(this, arg1name, arg2name, arg3name); }

  MKCILCTOR1(CilExpr,  newIntLit,  int,val)
  MKCILCTOR2(CilExpr,  newUnaryExpr,  UnaryOp,op, CilExpr*,expr)
  MKCILCTOR3(CilExpr,  newBinExpr,  BinOp,op, CilExpr*,e1, CilExpr*,e2)
  MKCILCTOR2(CilExpr,  newCastExpr,  Type const*,type, CilExpr*,expr)
  MKCILCTOR1(CilExpr,  newAddrOfExpr,  CilLval*,lval)

  MKCILCTOR1(CilLval,  newVarRef,  Variable*,var)
  MKCILCTOR1(CilLval,  newDeref,  CilExpr*,ptr)
  MKCILCTOR2(CilLval,  newFieldRef,  CilLval*,record, Variable*,field)
  MKCILCTOR2(CilLval,  newCastLval,  Type const*,type, CilLval*,lval)
  MKCILCTOR2(CilLval,  newArrayAccess,  CilExpr*,array, CilExpr*,index)

  MKCILCTOR1(CilExpr,  newVarRefExpr,  Variable*,var)

  MKCILCTOR2(CilInst,  newAssignInst,  CilLval*,lval, CilExpr*,expr)
  MKCILCTOR2(CilFnCall,  newFnCall,  CilLval*,result, CilExpr*,fn)

  MKCILCTOR2(CilStmt,  newWhileLoop,  CilExpr*,expr, CilStmt*,body)
  MKCILCTOR3(CilStmt,  newIfThenElse,  CilExpr*,cond, CilStmt*,thenBranch, CilStmt*,elseBranch)
  MKCILCTOR1(CilStmt,  newLabel,  LabelName,label)
  MKCILCTOR1(CilStmt,  newGoto,  LabelName,label)
  MKCILCTOR1(CilStmt,  newReturn,  CilExpr*,expr)
  MKCILCTOR2(CilStmt,  newSwitch,  CilExpr*,expr, CilStmt*,body)
  MKCILCTOR1(CilStmt,  newCase,  int,val)
  MKCILCTOR0(CilStmt,  newDefault)
  MKCILCTOR1(CilStmt,  newInst,  CilInst*,inst)

  MKCILCTOR2(CilStmt,  newAssign,  CilLval*,lval, CilExpr*,expr)

  MKCILCTOR0(CilCompound, newCompound);

  #undef MKCILCTOR0
  #undef MKCILCTOR1
  #undef MKCILCTOR2
  #undef MKCILCTOR3
};


// more analysis helpers
bool isOwnerPointer(Type const *t);
bool isArrayOfOwnerPointer(Type const *t);


#endif // CC_TREE_H
