// exprvisit.cc
// code for exprvisit.h

#include "exprvisit.h"       // this module

#include "c.ast.gen.h"       // C AST definitions

void walkExpression(ExpressionVisitor &vis, Expression const *root)
{     
  // visit the current node
  vis.visitExpr(root);

  ASTSWITCHC(Expression, root) {
    ASTCASEC(E_funCall, e)
      walkExpression(vis, e->func);
      FOREACH_ASTLIST(Expression, args, iter) {
        walkExpression(vis, iter.data());
      }

    ASTNEXTC(E_fieldAcc, e)
      walkExpression(e->obj);

    ASTNEXTC(E_sizeof, e)     
      // this is potentially bad since, e.g. if I'm searching for
      // modifications to variables, it doesn't hurt inside sizeof..
      // need walk cancellation semantics, but whatever
      walkExpression(e->expr);

    ASTNEXTC(E_unary, e)
      walkExpression(e->expr);

    ASTNEXTC(E_effect, e)     
      walkExpression(e->expr);

    ASTNEXTC(E_binary, e)
      walkExpression(e->e1);
      walkExpression(e->e2);

    ASTNEXTC(E_addrOf, e)
      walkExpression(e->expr);

    ASTNEXTC(E_deref, e)
      walkExpression(e->ptr);

    ASTNEXTC(E_cast, e)
      walkExpression(e->expr);

    ASTNEXTC(E_cond, e)
      walkExpression(e->cond);
      walkExpression(e->th);
      walkExpression(e->el);

    ASTNEXTC(E_comma, e)
      walkExpression(e->e1);
      walkExpression(e->e2);

    ASTNEXTC(E_assign, e)
      walkExpression(e->target);
      walkExpression(e->src);

    ASTNEXTC(E_forall, e)
      walkExpression(e->pred);

    ASTENDCASECD
  }
}

