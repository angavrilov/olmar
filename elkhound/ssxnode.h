// ssxnode.h
// parse tree node for SSx.tree.gr

#ifndef SSXNODE_H
#define SSXNODE_H

class Node {
public:
  enum Type { MERGE, SSX, X } type;
  Node *left, *right;

public:
  Node(Type t, Node *L, Node *R)
    : type(t), left(L), right(R) {}
};

#endif // SSXNODE_H
