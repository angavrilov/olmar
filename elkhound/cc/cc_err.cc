// cc_err.cc            see license.txt for copyright and terms of use
// code for cc_err.h

#include "cc_err.h"      // this module
#include "cc_tree.h"     // CCTreeNode


// ------------------- SemanticError --------------------
SemanticError::SemanticError(CCTreeNode const *n, SemanticErrorCode c)
  : node(n),
    code(c),
    msg(),
    varName()
{}


SemanticError::SemanticError(SemanticError const &obj)
  : DMEMB(node),
    DMEMB(code),
    DMEMB(msg),
    DMEMB(varName)
{}


SemanticError::~SemanticError()
{}


SemanticError& SemanticError::operator= (SemanticError const &obj)
{
  if (this != &obj) {
    CMEMB(node);
    CMEMB(code);
    CMEMB(msg);
    CMEMB(varName);
  }
  return *this;
}


string SemanticError::whyStr() const
{
  stringBuilder sb;
  sb << node->locString() << ": ";
  
  switch (code) {
    default:
      xfailure("bad code");

    case SE_DUPLICATE_VAR_DECL:
      sb << "duplicate variable declaration for `" << varName << "'";
      break;

    case SE_UNDECLARED_VAR:
      sb << "undeclared variable `" << varName << "'";
      break;

    case SE_GENERAL:
      sb << msg;
      break;
      
    case SE_INTERNAL_ERROR:
      sb << "internal error: " << msg;
      break;
  }
  
  return sb;
}


// -------------------- XSemanticError ----------------------
XSemanticError::XSemanticError(SemanticError const &e)
  : xBase(e.whyStr()),
    err(e)
{}


XSemanticError::XSemanticError(XSemanticError const &obj)
  : xBase(obj),
    DMEMB(err)
{}

XSemanticError::~XSemanticError()
{}
