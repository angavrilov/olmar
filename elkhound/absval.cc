// absval.cc
// pretty-printing, etc., for abstract domain values

#include "absval.ast.gen.h"    // this module
#include "str.h"          // stringBuilder


string AbsValue::toString() const
{
  stringBuilder sb;
  toSexp(sb);
  return sb;
}


void AVvar::toSexp(stringBuilder &sb) const
{
  sb << (char const*)name;
}

int AVvar::toSexpLen() const
{
  return strlen(name);
}


void AVint::toSexp(stringBuilder &sb) const
{
  sb << i;
}

int AVint::toSexpLen()
{ 
  int ret = 0;
  if (i < 0) {
    ret++;
    i = -i;
  }

  while (i >= 10) {
    ret++;
    i = i/10;
  }
  ret++;     // final digit, less than 10

  return ret;
}


void AVunary::toSexp(stringBuilder &sb) const
{
  static char const * const names[] = {
    "uPlus",
    "uMinus",
    "uNot",
    "uBitNot"
  };
  STATIC_ASSERT(TABLESIZE(names) == NUM_UNARYOPS);

  if (op < UNY_PLUS) {
    cout << "warning: emitting sexp for " << unaryOpNames[op] << endl;
  }

  sb << "(" << names[op] << " ";
  val->toSexp(sb);
  sb << ")";
}

int AVunary::toSexpLen() const
{
  return 3 + strlen(names[op]) + val->toSexpLen();
}


void AVbinary::toSexp(stringBuilder &sb) const
{
  static char const * const names[] = {
    "bEqual",
    "bNotEqual",
    "bLess",
    "bGreater",
    "bLessEq",
    "bGreaterEq",

    "*",       // the three interpreted symbols are special
    "bDiv",
    "bMod",
    "+",
    "-",
    "bLShift",
    "bRShift",
    "bBitAnd",
    "bBitXor",
    "bBitOr",
    "bAnd",
    "bOr",

    "!!!BIN_ASSIGN!!!",    // shouldn't be sent to Simplify

    "bImplies"
  };
  STATIC_ASSERT(TABLESIZE(names) == NUM_BINARYOPS);
  xassert(op != BIN_ASSIGN);

  sb << "(" << names[op] << " ";
  v1->toSexp(sb);
  sb << " ";
  v2->toSexp(sb);
  sb << ")";
}

int AVbinary::toSexpLen() const
{
  return 4 + strlen(names[op]) + v1->toSexpLen() + v2->toSexpLen();
}


void AVcond::toSexp(stringBuilder &sb) const
{
  sb << "(ifThenElse ";
  cond->toSexp(sb);
  sb << " ";
  th->toSexp(sb);
  sb << " ";
  el->toSexp(sb);
  sb << ")";
}

int AVcond::toSexpLen() const
{
  return 15 + cond->toSexpLen() + th->toSexpLen() + el->toSexpLen();
}


void AVfunc::toSexp(stringBuilder &sb) const
{
  sb << "(" << func;
  FOREACH_ASTLIST(AbsValue, args, iter) {
    sb << " ";
    iter.data()->toSexp(sb);
  }
  sb << ")";
}

int AVfunc::toSexpLen() const
{
  int ret = 2 + strlen(func);
  FOREACH_ASTLIST(AbsValue, args, iter) {
    ret += 1 + iter.data()->toSexpLen();
  }
  return ret;
}


void AVlval::toSexp(stringBuilder &sb) const
{
  // this should not be passed to simplify; but if it does, I want
  // to see the information ("!" is to make Simplify choke)
  sb << "(!AVlval " << progVar->name << " ";
  offset->toSexp(sb);
  sb << ")";
}

int AVlval::toSexpLen() const
{
  return 11 + strlen(progVar->name) + offset->toSexpLen();
}


void AVsub::toSexp(stringBuilder &sb) const
{
  sb << "(sub ";
  index->toSexp(sb);
  sb << " ";
  offset->toSexp(sb);
  sb << ")";
}

int AVsub::toSexpLen() const
{
  return 7 + index->toSexpLen() + offset->toSexpLen();
}


void AVwhole::toSexp(stringBuilder &sb) const
{
  sb << "whole";
}

int AVwhole::toSexpLen() const
{
  return 5;
}


// ------------ syntactic sugar ----------------
AVfunc *avFunc1(StringRef func, AbsValue *a1)
{
  AVfunc *f = new AVfunc(func, NULL);
  f->args.prepend(a1);
  return f;
}

AVfunc *avFunc2(StringRef func, AbsValue *a1, AbsValue *a2)
{
  xassert(a1 && a2);
  AVfunc *f = new AVfunc(func, NULL);
  f->args.prepend(a2);
  f->args.prepend(a1);
  return f;
}

AVfunc *avFunc3(StringRef func, AbsValue *a1, AbsValue *a2, AbsValue *a3)
{
  xassert(a1 && a2 && a3);
  AVfunc *f = new AVfunc(func, NULL);
  f->args.prepend(a3);
  f->args.prepend(a2);
  f->args.prepend(a1);
  return f;
}

AVfunc *avFunc4(StringRef func, AbsValue *a1, AbsValue *a2, AbsValue *a3, AbsValue *a4)
{
  AVfunc *f = new AVfunc(func, NULL);
  f->args.prepend(a4);
  f->args.prepend(a3);
  f->args.prepend(a2);
  f->args.prepend(a1);
  return f;
}

AVfunc *avFunc5(StringRef func, AbsValue *a1, AbsValue *a2, AbsValue *a3, AbsValue *a4, AbsValue *a5)
{
  xassert(a1 && a2 && a3);
  AVfunc *f = new AVfunc(func, NULL);
  f->args.prepend(a5);
  f->args.prepend(a4);
  f->args.prepend(a3);
  f->args.prepend(a2);
  f->args.prepend(a1);
  return f;
}

AVunary *avNot(AbsValue *v)
{
  return new AVunary(UNY_NOT, v);
}

// ------------- visitor ---------------
bool AbsValueVisitor::visitAbsValue(AbsValue const *value)
{
  return true;
}

void walkAbsValue(AbsValueVisitor &vis, AbsValue const *value)
{
  if (!vis.visitAbsValue(value)) {
    return;
  }

  ASTSWITCHC(AbsValue, value) {
    ASTCASEC(AVunary, u) {
      walkAbsValue(vis, u->val);
    }

    ASTNEXTC(AVbinary, b) {
      walkAbsValue(vis, b->v1);
      walkAbsValue(vis, b->v2);
    }

    ASTNEXTC(AVcond, c) {
      walkAbsValue(vis, c->cond);
      walkAbsValue(vis, c->th);
      walkAbsValue(vis, c->el);
    }

    ASTNEXTC(AVfunc, f) {
      FOREACH_ASTLIST(AbsValue, f->args, iter) {
        walkAbsValue(vis, iter.data());
      }
    }

    ASTENDCASECD
  }
}
