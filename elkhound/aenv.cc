// aenv.cc
// code for aenv.h

#include "aenv.h"               // this module
#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // IntValue & children
#include "prover.h"             // runProver
#include "predicate.ast.gen.h"  // Predicate, P_and
#include "trace.h"              // tracingSys

AEnv::AEnv(StringTable &table)
  : ints(),
    facts(new P_and(NULL)),
    counter(1),
    stringTable(table)
{}

AEnv::~AEnv()
{
  delete facts;
}


void AEnv::clear()
{
  ints.empty();
  facts->conjuncts.deleteAll();
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


void AEnv::addFact(IntValue *expr)
{
  facts->conjuncts.append(exprToPred(expr));
}


void AEnv::prove(IntValue const *expr)
{
  // map the goal into a predicate
  Predicate *goal = exprToPred(expr);

  // build an implication with all our known facts on the left
  // (temporarily let this object think it owns 'facts')
  P_impl implication(facts, goal);
  facts = NULL;

  // map this into an sexp
  string implSexp = implication.toSexpString();

  // restore proper ownership of 'facts'
  facts = implication.premise->asP_and();
  implication.premise = NULL;

  bool printPredicate = tracingSys("predicates");

  // run the prover on that predicate
  if (runProver(implSexp)) {
    if (printPredicate) {
      cout << "predicate proved\n";
    }
  }
  else {
    cout << "predicate NOT proved:\n";
    printPredicate = true;      // always print for unprovens
  }
  
  if (printPredicate) {
    FOREACH_ASTLIST(Predicate, facts->conjuncts, iter) {
      cout << "  fact: " << iter.data()->toSexpString() << "\n";
    }
    cout << "  goal: " << goal->toSexpString() << "\n";
  }

  // if I did it right, 'implication' contains properly
  // recursively owned substructures..
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




