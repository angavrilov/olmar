// ast.h
// representations for Abstract Syntax Trees
// (though they're not all that abstract)

#ifndef __AST_H
#define __AST_H


#include <iostream.h>       // ostream
#include "str.h"            // string
#include "objlist.h"        // ObjList


// fwd decls
class ASTInternal;
class ASTLeaf;


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

  // global type-to-string mapper.. since there could be several
  // kinds of ASTs running around, this isn't entirely ideal, but
  // it solves my present problem
  typedef string (*TypeToStringFn)(int type);
  static TypeToStringFn typeToString;       // initially NULL

public:     // funcs
  ASTNode(int t) : type(t) {}
  virtual ~ASTNode();

  // true when this can safely be cast (with asInternal())
  // to an ASTInternal
  virtual bool isInternal() const = 0;
  bool isLeaf() const { return !isInternal(); }

  // casts; checked at runtime (and exception thrown if invalid)
  ASTInternal const &asInternalC() const;
  ASTInternal &asInternal()
    { return const_cast<ASTInternal&>(asInternalC()); }

  ASTLeaf const &asLeafC() const;
  ASTLeaf &asLeaf()
    { return const_cast<ASTLeaf&>(asLeafC()); }

  // debugging: print as indented tree
  virtual void debugPrint(ostream &os, int indent) const;

  // debugging: return {}-nested string
  virtual string toString() const;

  // debugging: map 'type' into a descriptive string; the default
  // calls 'typeToString' if non-NULL, otherwise does itoa
  virtual string typeDesc() const;
};


// tree-walking callback function; called for each node in the
// tree with app-supplied extra info, until returns true ("done");
// called with internal nodes twice, once on the way down and
// again on the way up; 'virstVisit' distinguishes
typedef bool (*ASTWalkTree)(ASTNode *node, bool firstVisit, void *extra);


// generic internal node
class ASTInternal : public ASTNode {
public:         // data
  // list of child nodes
  ObjList<ASTNode> children;

public:         // funcs
  ASTInternal(int type);
  virtual ~ASTInternal();


  // tree constructor functions
  static ASTInternal *cons0(int type);
  static ASTInternal *cons1(int type, ASTNode *child1);
  static ASTInternal *cons2(int type, ASTNode *child1, ASTNode *child2);
  static ASTInternal *cons3(int type, ASTNode *child1, ASTNode *child2,
                                      ASTNode *child3);
  static ASTInternal *cons4(int type, ASTNode *child1, ASTNode *child2,
                                      ASTNode *child3, ASTNode *child4);

  // append a child to 'children' and return 'this'
  ASTInternal *append(ASTNode *newChild);


  // do a treewalk; returns the node where 'func' returned true,
  // or NULL if it never did
  ASTNode *walkTree(ASTWalkTree func, void *extra);

  // helper for debugPrint
  static bool printWalker(ASTNode *node, bool firstVisit, void *extra);

  // ASTNode funcs
  virtual bool isInternal() const { return true; }
  virtual void debugPrint(ostream &os, int indent) const;
  virtual string toString() const;
};


// macros for bison rules
#define AST0(t) ASTInternal::cons0(t)
#define AST1(t,c1) ASTInternal::cons1((t),(c1))
#define AST2(t,c1,c2) ASTInternal::cons2((t),(c1),(c2))
#define AST3(t,c1,c2,c3) ASTInternal::cons3((t),(c1),(c2),(c3))
#define AST4(t,c1,c2,c3,c4) ASTInternal::cons4((t),(c1),(c2),(c3),(c4))


// leaf superclass; real ASTs need a variety of kinds of leaf,
// and so this class should be subclassed as necessary
class ASTLeaf : public ASTNode {
public:
  ASTLeaf(int type) : ASTNode(type) {}

  // ASTNode funcs
  virtual bool isInternal() const { return false; }
  virtual string toString() const;
};


// many leaves will just carry one piece of data that doesn't
// need to be 'delete'd and knows how to do string<<
template <class Data, int typeCode>
class ASTSimpleLeaf : public ASTLeaf {
public:
  Data data;

  ASTSimpleLeaf(Data d)
    : ASTLeaf(typeCode), data(d) {}
  virtual ~ASTSimpleLeaf() {}

  virtual string toString() const
    { return stringc << typeDesc() << ":" << data; }
};


// may as well do one for owner pointers
template <class Data, int typeCode>
class ASTOwnerLeaf : public ASTLeaf {
public:
  Data *data;

  ASTOwnerLeaf(Data *d)
    : ASTLeaf(typeCode), data(d) {}

  virtual ~ASTOwnerLeaf()
  {
    // cripes.. right here I'm using:
    //   - templates
    //   - templates with non-"class" arguments
    //   - virtual functions
    //   - inline function
    //   - 'delete', and a dtor might be called
    // that's 5 language features.. getting this by g++ is
    // going to be interesting....
    delete data;
  }
};


// example usage:
//   typedef ASTSimpleLeaf<int, AST_INTEGER> ASTIntLeaf;
//   typedef ASTSimpleLeaf<string, AST_STRING> ASTStringLeaf;


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
