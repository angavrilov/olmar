// cc_cfg.h
// control flow graph for C++ parse tree

#ifndef CC_CFG_H
#define CC_CFG_H

// stuff to get cc.cc to compile, for now

class CFGEdge;

class CFGNode {
};

class CFGNullNode : public CFGNode {
public:
  CFGEdge *getOutgoingEdge() { return NULL; }
  void setTarget(CFGNode *dest) {}
};

class CFGIfNode : public CFGNode {
public:
  CFGEdge *getThenEdge() { return NULL; }
  CFGEdge *getElseEdge() { return NULL; }
};

class CFGSwitchNode : public CFGNode {};
class CFGReturnNode : public CFGNode {};      
class CFGExprNode : public CFGNode {};

class CFGEdge {
public:
  void setTarget(CFGNode *node) {}
  CFGNode *getTarget() { return NULL; }
};

class CFG {
public:
  CFGNullNode *makeNullNode(CFGNode *target) { return NULL; }
  CFGIfNode *makeIfNode(CCTreeNode *expr, CFGNode *thenNode, CFGNode *elseNode) { return NULL; }
  CFGSwitchNode *makeSwitchNode(CCTreeNode *expr, CFGNode *afterDest) { return NULL; }
  CFGReturnNode *makeReturnNode(CCTreeNode *expr, CFGNode *retDest) { return NULL; }
  CFGExprNode *makeExprNode(CCTreeNode *expr) { return NULL; }

  // N1 -> edge -> N2  becomes
  // N1 -> edge -> node -> edge2 -> N2,  and
  // edge2 is returned
  CFGEdge *insertNode(CFGNode *node, CFGEdge *edge) { return NULL; }

  void addLabel(string name, CFGEdge *edge) {}
  void addCaseLabel(CCTreeNode *expr, CFGEdge *edge) {}
  void addDefaultLabel(CFGEdge *edge) {}

  void pushSwitch(CFGSwitchNode *sw) {}
  void popSwitch() {}

  void pushContinue(CFGNode *contDest) {}
  CFGNode *getContinueTarget() { return NULL; }
  void popContinue() {}

  void pushBreak(CFGNode *breakDest) {}
  CFGNode *getBreakTarget() { return NULL; }
  void popBreak() {}

  CFGNode *getReturnTarget() { return NULL; }
  CFGNode *getLabelTarget(string name) { return NULL; }
};

#endif // CC_CFG_H
