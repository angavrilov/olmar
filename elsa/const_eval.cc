// const_eval.cc
// code for const_eval.h

#include "const_eval.h"     // this module
#include "cc_ast.h"         // C++ AST


ConstEval::ConstEval()
  : msg(),
    dependent(false)
{}

ConstEval::~ConstEval()
{}

bool ConstEval::setDependent(int &result)
{
  dependent = true;
  return true;
}


// external interface
bool Expression::constEval(ConstEval &env, int &result) const
{                    
  if (iconstEval(env, result)) {
    if (env.dependent) {
      result = 1;        // nominal value safe for most contexts
    }
    return true;
  }
  return false;
}


bool Expression::iconstEval(ConstEval &env, int &result) const
{
  xassert(!ambiguity);

  if (type->isError()) {
    // don't try to const-eval an expression that failed
    // to typecheck
    return false;
  }

  ASTSWITCHC(Expression, this) {
    // Handle this idiom for finding a member offset:
    // &((struct scsi_cmnd *)0)->b.q
    ASTCASEC(E_addrOf, eaddr)
      return eaddr->expr->constEvalAddr(env, result);

    ASTNEXTC(E_boolLit, b)
      result = b->b? 1 : 0;
      return true;

    ASTNEXTC(E_intLit, i)
      result = i->i;
      return true;

    ASTNEXTC(E_charLit, c)
      result = c->c;
      return true;

    ASTNEXTC(E_variable, v)
      if (v->var->isEnumerator()) {
        result = v->var->getEnumeratorValue();
        return true;
      }

      if (v->var->type->isCVAtomicType() &&
          (v->var->type->asCVAtomicTypeC()->cv & CV_CONST) &&
          v->var->value) {
        // const variable
        return v->var->value->iconstEval(env, result);
      }

      if (v->var->type->isGeneralizedDependent() &&
          v->var->value) {
        return env.setDependent(result);
      }

      if (v->var->isTemplateParam()) {
        return env.setDependent(result);
      }

      env.msg = stringc
        << "can't const-eval non-const variable `" << v->var->name << "'";
      return false;

    ASTNEXTC(E_constructor, c)
      if (type->isIntegerType()) {
        // allow it; should only be 1 arg, and that will be value
        return c->args->first()->iconstEval(env, result);
      }
      else {
        env.msg = "can only const-eval E_constructors for integer types";
        return false;
      }

    ASTNEXTC(E_sizeof, s)
      result = s->size;
      return true;

    ASTNEXTC(E_unary, u)
      if (!u->expr->iconstEval(env, result)) return false;
      switch (u->op) {
        default: xfailure("bad code");
        case UNY_PLUS:   result = +result;  return true;
        case UNY_MINUS:  result = -result;  return true;
        case UNY_NOT:    result = !result;  return true;
        case UNY_BITNOT: result = ~result;  return true;
      }

    ASTNEXTC(E_binary, b)
      if (b->op == BIN_COMMA) {
        // avoid trying to eval the LHS
        return b->e2->iconstEval(env, result);
      }

      int v1, v2;
      if (!b->e1->iconstEval(env, v1) ||
          !b->e2->iconstEval(env, v2)) return false;

      if (v2==0 && (b->op == BIN_DIV || b->op == BIN_MOD)) {
        env.msg = "division by zero in constant expression";
        return false;
      }

      switch (b->op) {
        case BIN_EQUAL:     result = (v1 == v2);  return true;
        case BIN_NOTEQUAL:  result = (v1 != v2);  return true;
        case BIN_LESS:      result = (v1 < v2);  return true;
        case BIN_GREATER:   result = (v1 > v2);  return true;
        case BIN_LESSEQ:    result = (v1 <= v2);  return true;
        case BIN_GREATEREQ: result = (v1 >= v2);  return true;

        case BIN_MULT:      result = (v1 * v2);  return true;
        case BIN_DIV:       result = (v1 / v2);  return true;
        case BIN_MOD:       result = (v1 % v2);  return true;
        case BIN_PLUS:      result = (v1 + v2);  return true;
        case BIN_MINUS:     result = (v1 - v2);  return true;
        case BIN_LSHIFT:    result = (v1 << v2);  return true;
        case BIN_RSHIFT:    result = (v1 >> v2);  return true;
        case BIN_BITAND:    result = (v1 & v2);  return true;
        case BIN_BITXOR:    result = (v1 ^ v2);  return true;
        case BIN_BITOR:     result = (v1 | v2);  return true;
        case BIN_AND:       result = (v1 && v2);  return true;
        case BIN_OR:        result = (v1 || v2);  return true;
        // BIN_COMMA handled above

        default:         // BIN_BRACKETS, etc.
          return false;
      }

    ASTNEXTC(E_cast, c)
      if (!c->expr->iconstEval(env, result)) return false;

      Type *t = c->ctype->getType();
      if (t->isIntegerType() ||
          t->isPointer()) {             // for Linux kernel
        return true;       // ok
      }
      else {
        // TODO: this is probably not the right rule..
        env.msg = stringc
          << "in constant expression, can only cast to integer or pointer types, not `"
          << t->toString() << "'";
        return false;
      }

    ASTNEXTC(E_cond, c)
      if (!c->cond->iconstEval(env, result)) return false;

      if (result) {
        return c->th->iconstEval(env, result);
      }
      else {
        return c->el->iconstEval(env, result);
      }

    ASTNEXTC(E_sizeofType, s)
      if (s->atype->getType()->isGeneralizedDependent()) {
        return env.setDependent(result);
      }
      result = s->size;
      return true;

    ASTNEXTC(E_grouping, e)
      return e->expr->iconstEval(env, result);

    ASTDEFAULTC
      return extConstEval(env, result);

    ASTENDCASEC
  }
}

// The intent of this function is to provide a hook where extensions
// can handle the 'constEval' message for their own AST syntactic
// forms, by overriding this function.  The non-extension nodes use
// the switch statement above, which is more compact.
bool Expression::extConstEval(ConstEval &env, int &result) const
{
  env.msg = stringc << kindName() << " is not constEval'able";
  return false;
}

bool Expression::constEvalAddr(ConstEval &env, int &result) const
{
  result = 0;                   // FIX: this is wrong
  int result0;                  // dummy for use below
  // FIX: I'm sure there are cases missing from this
  ASTSWITCHC(Expression, this) {
    // These two are dereferences, so they recurse back to constEval().
    ASTCASEC(E_deref, e)
      return e->ptr->iconstEval(env, result0);
      break;

    ASTNEXTC(E_arrow, e)
      return e->obj->iconstEval(env, result0);
      break;

    // These just recurse on constEvalAddr().
    ASTNEXTC(E_fieldAcc, e)
      return e->obj->constEvalAddr(env, result0);
      break;

    ASTNEXTC(E_cast, e)
      return e->expr->constEvalAddr(env, result0);
      break;

    ASTNEXTC(E_keywordCast, e)
      // FIX: haven't really thought about these carefully
      switch (e->key) {
      default:
        xfailure("bad CastKeyword");
      case CK_DYNAMIC:
        return false;
        break;
      case CK_STATIC: case CK_REINTERPRET: case CK_CONST:
        return e->expr->constEvalAddr(env, result0);
        break;
      }
      break;

    ASTNEXTC(E_grouping, e)
      return e->expr->constEvalAddr(env, result);
      break;

    ASTDEFAULTC
      return false;
      break;

    ASTENDCASEC
  }
}


// EOF
