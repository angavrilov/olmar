// cc_tree.cc
// code for cc_tree.h

#include "cc_tree.h"     // this module


// --------------------- CCTreeNode ----------------------
void CCTreeNode::error(char const *msg) const
{
  THROW(XTreeError(this, msg));
}


// -------------------- XTreeError ----------------------
STATICDEF string XTreeError::makeMsg(CCTreeNode const *n, char const *m)
{                    
  return stringc << n->locString() << ": " << m;
}

XTreeError::XTreeError(CCTreeNode const *n, char const *m)
  : xBase(makeMsg(n, m)),
    node(n),
    message(m)
{}


XTreeError::XTreeError(XTreeError const &obj)
  : xBase(obj),
    DMEMB(node),
    DMEMB(message)
{}

XTreeError::~XTreeError()
{}

