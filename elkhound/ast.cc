// ast.cc
// code for ast.h

#include "ast.h"       // this module
#include "exc.h"       // xfailure


// --------------------- ASTNode -------------------
ASTNode::TypeToStringFn ASTNode::typeToString = NULL;
int ASTNode::nodeCount = 0;


ASTNode::ASTNode(int t)
  : type(t)
{
  nodeCount++;
}

ASTNode::~ASTNode()
{
  nodeCount--;
}


ASTInternal const &ASTNode::asInternalC() const
{
  if (!isInternal()) {
    xfailure("asInternal called on non-ASTInternal object");
  }
  return reinterpret_cast<ASTInternal const &>(*this);
}


ASTLeaf const &ASTNode::asLeafC() const
{
  if (!isLeaf()) {
    xfailure("asLeaf called on non-ASTLeaf object");
  }
  return reinterpret_cast<ASTLeaf const &>(*this);
}


bool ASTNode::leftmostLoc(SourceLocation &loc) const
{
  return false;
}


bool ASTNode::hasLeftmostLoc() const
{
  SourceLocation dummy;
  return leftmostLoc(dummy);
}

SourceLocation ASTNode::getLeftmostLoc() const
{
  SourceLocation ret;
  if (!leftmostLoc(ret)) {
    xfailure("this node has not leftmost location");
  }
  return ret;
}


static void doIndent(ostream &os, int indent)
{
  loopi(indent) {
    os << ' ';
  }
}

void ASTNode::debugPrint(ostream &os, int indent) const
{
  doIndent(os, indent);
  os << toString() << endl;
}

string ASTNode::toString() const
{
  return stringc << "ASTNode(" << typeDesc() << ")";
}


string ASTNode::typeDesc() const
{
  if (typeToString != NULL) {
    return typeToString(type);
  }
  else {
    return stringc << type;
  }
}


// ----------------- ASTInternal ------------------
ASTInternal::ASTInternal(int t)
  : ASTNode(t),
    children()
{}

ASTInternal::~ASTInternal()
{}


STATICDEF ASTInternal *ASTInternal::cons0(int type)
{
  return new ASTInternal(type);
}

STATICDEF ASTInternal *ASTInternal::cons1(int type, ASTNode *child1)
{
  ASTInternal *ret = new ASTInternal(type);
  ret->append(child1);
  return ret;
}

STATICDEF ASTInternal *ASTInternal::
  cons2(int type, ASTNode *child1, ASTNode *child2)
{
  ASTInternal *ret = new ASTInternal(type);
  ret->append(child1);
  ret->append(child2);
  return ret;
}

STATICDEF ASTInternal *ASTInternal::
  cons3(int type, ASTNode *child1, ASTNode *child2, ASTNode *child3)
{
  ASTInternal *ret = new ASTInternal(type);
  ret->append(child1);
  ret->append(child2);
  ret->append(child3);
  return ret;
}

STATICDEF ASTInternal *ASTInternal::
  cons4(int type, ASTNode *child1, ASTNode *child2,
                  ASTNode *child3, ASTNode *child4)
{
  ASTInternal *ret = new ASTInternal(type);
  ret->append(child1);
  ret->append(child2);
  ret->append(child3);
  ret->append(child4);
  return ret;
}


ASTInternal *ASTInternal::append(ASTNode *newChild)
{
  children.append(newChild);
  return this;       // returning 'this' is for syntactic convenience elsewhere
}


ASTNode *ASTInternal::walkTree(ASTWalkTree func, void *extra)
{
  // pre-order visit
  if (func(this, true /*firstVisit*/, extra)) {
    return this;
  }

  MUTATE_EACH_OBJLIST(ASTNode, children, iter) {
    if (iter.data()->isLeaf()) {
      // call func on the leaf
      if (func(iter.data(), true /*firstVisit*/, extra)) {
        return iter.data();
      }
    }
    else {
      // recursively walk the subtree
      ASTNode *ret = iter.data()->asInternal().walkTree(func, extra);
      if (ret) {
        // callback stopped the search
        return ret;
      }
    }
  }

  // post-order visit
  if (func(this, false /*firstVisit*/, extra)) {
    return this;
  }

  return NULL;
}


void ASTInternal::debugPrint(ostream &os, int indent) const
{
  doIndent(os, indent);
  os << "type=" << typeDesc() << " ("
     << children.count() << " children)\n";

  FOREACH_OBJLIST(ASTNode, children, iter) {
    iter.data()->debugPrint(os, indent+2);
  }
}


string ASTInternal::toString() const
{
  stringBuilder sb;

  sb << "{ type=" << typeDesc() << " ";

  FOREACH_OBJLIST(ASTNode, children, iter) {
    sb << iter.data()->toString() << " ";
  }

  sb << "}";

  return sb;
}


ASTInternal *iappend(ASTNode *presumedInternal, ASTNode *newChild)
{
  return presumedInternal->asInternal().append(newChild);
}


bool ASTInternal::leftmostLoc(SourceLocation &L) const
{
  // check with each child in order until one has a location
  FOREACH_OBJLIST(ASTNode, children, iter) {
    if (iter.data()->leftmostLoc(L)) {
      return true;
    }
  }

  // and if none do then bail
  return false;
}


// ------------------- ASTLeaf --------------------
bool ASTLeaf::leftmostLoc(SourceLocation &L) const
{
  L = loc;
  return true;
}


string ASTLeaf::toString() const
{
  return stringc << "Leaf(" << typeDesc() << ")";
}
