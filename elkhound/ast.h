// ast.h
// representations for Abstract Syntax Trees
// (though they're not all that abstract)

#ifndef __AST_H
#define __AST_H
  

// fwd decls
class ASTInternal;


// tree node superclass
class ASTNode {
public:     // data
  // app-defined code to identify the run-time type of this
  // object (see end of file re RTTI)
  //                   
  // for ASTLeaf derivatives, specifies which kind of leaf
  // this is
  //
  // for ASTInternal, specifies the interpretations of the
  // node's children; typical usage is to have one code per
  // nonterminal and put the code here for the nonterminal that
  // generated the node
  int type;

public:     // funcs
  ASTNode(int t) : type(t) {}
  virtual ~ASTNode();

  // true when this can safely be cast (with asInternal())
  // to an ASTInternal
  virtual bool isInternal() const = 0;
  bool isLeaf() const { return !isInternal(); }

  // cast; checked at runtime (and exception thrown if invalid)
  ASTInternal const &asInternalC() const;
  ASTInternal &asInternal()
    { return const_cast<ASTInternal&>(asInternalC()); }
};


// leaf superclass; real ASTs need a variety of kinds of leaf,
// and so this class should be subclassed as necessary
class ASTLeaf {
public:
  // main purpose of this class is to supply this implementation
  virtual bool isInternal() const { return false; }
};


// tree-walking callback function; called for each node in the
// tree with app-supplied extra info, until returns true ("done")
typedef bool (*ASTTreeWalk)(ASTNode *node, void *extra);


// generic internal node
class ASTInternal {
public:         // data
  // list of child nodes
  ObjList<ASTNode> children;

public:         // funcs
  ASTInternal(int type);
  virtual ~ASTInternal();

  

  // do a treewalk; returns the node where 'func' returned true,
  // or NULL if it never did
  ASTNode *treeWalk(ASTTreeWalk func, void *extra);

  // ASTNode funcs
  virtual bool isInternal() const { return true; }
};






/*
 * why not RTTI (C++ Run-Time Type Identification) instead of 'type'?
 *
 *  - more flexible: 'type' can be the same for two different C++
 *    types if their treatment is the same, or different for two
 *    instances of the same class if that is appropriate; the C++
 *    type conveys *structure* whereas 'type' conveys *purpose*
 *
 *  - not much more syntactic overhead in declarations, and generally
 *    *less* syntactic overhead in usage
 *
 *  - more portable: lots of compilers either don't support RTTI, or
 *    do it differently from others
 *
 *  - (more efficient too, but that's not a good reason usually)
 */

#endif // __AST_H
