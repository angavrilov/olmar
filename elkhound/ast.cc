// ast.cc
// code for ast.h

#include "ast.h"       // this module
#include "exc.h"       // xfailure


// --------------------- ASTNode -------------------
ASTNode::~ASTNode()
{}

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


struct PrintWalkData {
  ostream &os;
  int indent;
             
public:
  PrintWalkData(ostream &o, int i)
    : os(o), indent(i) {}
  ostream &ind();
};

ostream &PrintWalkData::ind()
{
  loopi(indent) {
    os << ' ';
  } 
  return os;
}


void ASTInternal::debugPrint(ostream &os, int indent) const
{
  PrintWalkData dat(os, indent);
  
  // walkTree allows modifications, but 'printWalker' does not in
  // fact change anything.. another instance of nonideal constness
  // (need qualifier polymorphism)
  const_cast<ASTInternal*>(this)->walkTree(printWalker, &dat);
}


STATICDEF bool ASTInternal::
  printWalker(ASTNode *node, bool firstVisit, void *extra)
{
  PrintWalkData *dat = (PrintWalkData*)extra;

  if (node == NULL) {
    dat->ind() << "NULL\n";
  }

  else if (node->isInternal()) {
    if (firstVisit) {
      // print node type, then children indented
      dat->ind() << node->type << " ("
                 << node->asInternal().children.count()
                 << " children)\n";

      // children will be printed when the general walker code
      // visits them next
      dat->indent += 2;
    }
    else {
      // unindent on way up
      dat->indent -= 2;
    }
  }

  else {
    // use leaf-specific repr
    dat->ind() << node->asLeaf().toString() << endl;
  }

  return false;    // walk entire tree
}


// ------------------- ASTLeaf --------------------
string ASTLeaf::toString() const
{
  return stringc << "Leaf type " << type;
}


