// gramast.h
// declarations for the grammar AST

#ifndef __GRAMAST_H
#define __GRAMAST_H

#include "ast.h"       // basic ast stuff
#include "str.h"       // string


// node type codes
enum ASTTypeCode {
  // ---- leaves ----
  // tokens
  AST_INTEGER=1,         // ASTIntLeaf
  AST_STRING,            // ASTStringLeaf

  // ---- internal nodes ----
  // attribute-expression components
  EXP_ATTRREF,           // reference to an attribute
  EXP_FNCALL,            // function call
  EXP_LIST,              // expression list

  EXP_NEGATE,            // unary operators
  EXP_NOT,

  EXP_MULT,              // binary operators
  EXP_DIV,
  EXP_MOD,
  EXP_PLUS,
  EXP_MINUS,
  EXP_LT,
  EXP_GT,
  EXP_LTE,
  EXP_GTE,
  EXP_EQ,
  EXP_NEQ,
  EXP_AND,
  EXP_OR,

  EXP_COND,              // ternary operator
};

                                 
// map type to descriptive string
string astTypeToString(int type);


typedef ASTSimpleLeaf<int, AST_INTEGER> ASTIntLeaf;
typedef ASTSimpleLeaf<string, AST_STRING> ASTStringLeaf;


#endif // __GRAMAST_H
