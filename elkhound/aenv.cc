// aenv.cc
// code for aenv.h

#include "aenv.h"               // this module
#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // IntValue & children
#include "prover.h"             // runProver
#include "predicate.ast.gen.h"  // Predicate

AEnv::AEnv(StringTable &table)
  : ints(),
    stringTable(table),
    counter(1)
{}

AEnv::~AEnv()
{}


void AEnv::clear()
{
  ints.empty();
}


void AEnv::set(StringRef name, IntValue *value)
{
  if (ints.isMapped(name)) {
    ints.remove(name);
  }
  ints.add(name, value);
}


IntValue *AEnv::get(StringRef name)
{
  IntValue *ret;
  if (ints.query(name, ret)) {
    return ret;
  }
  else {
    return NULL;
  }
}


IntValue *AEnv::freshIntVariable(char const *why)
{
  StringRef name = stringTable.add(stringc << "v" << counter++);
  return new IVvar(name, why);
}

                     
// essentially, express 'expr != 0', but try to map as much
// of expr into the predicate domain as possible, so Simplify
// will interpret them as predicates (otherwise I'll have to
// add lots of inference rules about e.g. '<' in the expression
// domain); returns owner pointer
Predicate *exprToPred(IntValue const *expr)
{                      
  // make one instance of this, so I can circumvent
  // allocation issues for this one
  static IVint const zero(0);

  ASTSWITCHC(IntValue, expr) {
    ASTCASEC(IVint, i) {
      // when a literal integer is interpreted as a predicate,
      // I can map it immediately to true/false
      if (i->i == 0) {
        return new P_lit(false);
      }
      else {
        return new P_lit(true);
      }
    }

    ASTNEXTC(IVunary, u) {
      if (u->op == UNY_NOT) {
        return new P_not(exprToPred(u->val));
      }
    }

    ASTNEXTC(IVbinary, b) {
      if (b->op >= BIN_EQUAL && b->op <= BIN_GREATEREQ) {
        return new P_relation(b->v1, binOpToRelation(b->op), b->v2);
      }

      if (b->op == BIN_AND) {
        return P_and2(exprToPred(b->v1), exprToPred(b->v2));
      }

      if (b->op == BIN_OR) {
        return P_or2(exprToPred(b->v1), exprToPred(b->v2));
      }
      
      if (b->op == BIN_IMPLIES) {
        return new P_impl(exprToPred(b->v1), exprToPred(b->v2));
      }
    }

    ASTNEXTC(IVcond, c) {
      // map ?: as a pair of implications
      return P_and2(new P_impl(exprToPred(c->cond),
                               exprToPred(c->th)),
                    new P_impl(new P_not(exprToPred(c->cond)),
                               exprToPred(c->el)));
    }
    
    ASTENDCASEC
  }

  // if we get here, then the expression's toplevel construct isn't
  // amenable to translation into a predicate, so we just construct
  // a predicate to express the usual C true/false convention
  return new P_relation(expr, RE_NOTEQUAL, &zero);
}


void AEnv::prove(IntValue const *expr)
{
  // map the expression into a predicate
  Predicate *pred = exprToPred(expr);

  // map that into an sexp
  stringBuilder sb;
  pred->toSexp(sb);
  
  // run the prover
  if (runProver(sb)) {
    cout << "proved:\n";
  }
  else {
    cout << "predicate NOT proved:\n";
  }
  cout << "  " << sb << endl;

  // if I did it right, 'pred' is an owner pointer to properly
  // recursively owned substructures..
  delete pred;
}


IntValue *AEnv::grab(IntValue *v) { return v; }
void AEnv::discard(IntValue *)    {}
IntValue *AEnv::dup(IntValue *v)  { return v; }

void AEnv::print()
{
  for (StringSObjDict<IntValue>::Iter iter(ints);
       !iter.isDone(); iter.next()) {
    string const &name = iter.key();
    IntValue const *value = iter.value();

    cout << "  " << name << ":\n";
    if (value) {
      value->debugPrint(cout, 4);
    }
    else {
      cout << "    null\n";
    }
  }
}




