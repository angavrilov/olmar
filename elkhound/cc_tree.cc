// cc_tree.cc
// code for cc_tree.h

#include "cc_tree.h"     // this module


// --------------------- CCTreeNode ----------------------
void CCTreeNode::declareVariable(Env &env, char const *name,
                                 DeclFlags flags, Type const *type) const
{
  if (!env.declareVariable(name, flags, type)) {
    SemanticError err(this, SE_DUPLICATE_VAR_DECL);
    err.varName = name;
    env.report(err);
  }
}


void CCTreeNode::throwError(char const *msg) const
{
  SemanticError err(this, SE_GENERAL);
  err.msg = msg;
  THROW(XSemanticError(err));
}


void CCTreeNode::reportError(Env &env, char const *msg) const
{
  SemanticError err(this, SE_GENERAL);
  err.msg = msg;
  env.report(err);
}
