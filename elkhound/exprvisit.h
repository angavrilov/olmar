// exprvisit.h
// Expression visitor

#ifndef EXPRVISIT_H
#define EXPRVISIT_H

class Expression;

// interface for clients to implement
class ExpressionVisitor {
public:
  virtual void visitExpr(Expression const *expr) = 0;
};

// outer driver to visit an expression tree
void walkExpression(ExpressionVisitor &vis, Expression const *root);

#endif // EXPRVISIT_H
