// aenv.cc
// code for aenv.h

#include "aenv.h"               // this module
#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // AbsValue & children
#include "prover.h"             // runProver
#include "predicate.ast.gen.h"  // Predicate, P_and
#include "trace.h"              // tracingSys


// ----------------- VariablePrinter ----------------
// used to print what variables stand for
class VariablePrinter : public ValuePredicateVisitor {
  StringSObjDict<AVvar /*const*/> map;

public:
  virtual ~VariablePrinter() {}  // silence stupid warning
  virtual bool visitAbsValue(AbsValue const *value);
  void dump();
};


// remember every variable we see, but ignore duplicates
bool VariablePrinter::visitAbsValue(AbsValue const *value)
{
  if (value->isAVvar()) {
    AVvar const *v = value->asAVvarC();
    if (!map.isMapped(v->name)) {
      map.add(v->name, const_cast<AVvar*>(v));
    }
  }

  return true;      // recurse into children
}


// print the variables we say, and why those variables were introduced
void VariablePrinter::dump()
{
  StringSObjDict<AVvar>::Iter iter(map);
  for (; !iter.isDone(); iter.next()) {
    cout << "  " << iter.key() << ": "
         << iter.value()->why << "\n";
  }
}


// ------------------- AEnv -----------------
AEnv::AEnv(StringTable &table)
  : bindings(),
    facts(new P_and(NULL)),
    memVars(),
    counter(1),
    inPredicate(false),
    stringTable(table)
{
  clear();
}

AEnv::~AEnv()
{
  delete facts;
}


void AEnv::clear()
{
  bindings.empty();
  facts->conjuncts.deleteAll();
  memVars.empty();
  
  // initialize the environment with a fresh variable for memory
  set(str("mem"), freshVariable("initial contents of memory"));
}


void AEnv::set(StringRef name, AbsValue *value)
{
  if (bindings.isMapped(name)) {
    bindings.remove(name);
  }
  bindings.add(name, value);
}


AbsValue *AEnv::get(StringRef name)
{
  return bindings.queryf(name);
}


AbsValue *AEnv::freshVariable(char const *why)
{
  StringRef name = stringTable.add(stringc << "v" << counter++);
  return new AVvar(name, why);
}


AbsValue *AEnv::addMemVar(StringRef name)
{
  StringRef addrName = str(stringc << "addr_" << name);
  AbsValue *addr = new AVvar(addrName, stringc << "address of " << name);

  memVars.add(name, addr);

  return addr;
}


AbsValue *AEnv::getMemVarAddr(StringRef name)
{
  return memVars.queryf(name);
}

bool AEnv::isMemVar(StringRef name) const
{
  return memVars.isMapped(name);
}


// essentially, express 'expr != 0', but try to map as much
// of expr into the predicate domain as possible, so Simplify
// will interpret them as predicates (otherwise I'll have to
// add lots of inference rules about e.g. '<' in the expression
// domain); returns owner pointer
Predicate *exprToPred(AbsValue const *expr)
{
  // make one instance of this, so I can circumvent
  // allocation issues for this one
  static AVint const zero(0);

  ASTSWITCHC(AbsValue, expr) {
    ASTCASEC(AVint, i) {
      // when a literal integer is interpreted as a predicate,
      // I can map it immediately to true/false
      if (i->i == 0) {
        return new P_lit(false);
      }
      else {
        return new P_lit(true);
      }
    }

    ASTNEXTC(AVunary, u) {
      if (u->op == UNY_NOT) {
        return new P_not(exprToPred(u->val));
      }
    }

    ASTNEXTC(AVbinary, b) {
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

    ASTNEXTC(AVcond, c) {
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


void AEnv::addFact(AbsValue *expr)
{
  facts->conjuncts.append(exprToPred(expr));
}

  
void AEnv::prove(AbsValue const *expr)
{
  // map the goal into a predicate
  Predicate *goal = exprToPred(expr);

  // add the fact that all known local variable addresses are distinct
  P_distinct *addrs = new P_distinct(NULL);
  for (StringSObjDict<AbsValue>::Iter iter(memVars);
       !iter.isDone(); iter.next()) {
    addrs->terms.append(iter.value());
  }
  facts->conjuncts.prepend(addrs);

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
    VariablePrinter vp;

    FOREACH_ASTLIST(Predicate, facts->conjuncts, iter) {
      cout << "  fact: " << iter.data()->toSexpString() << "\n";
      walkValuePredicate(vp, iter.data());
    }
    cout << "  goal: " << goal->toSexpString() << "\n";
    walkValuePredicate(vp, goal);

    // print out variable map
    vp.dump();
  }

  // pull the distinction fact back out
  facts->conjuncts.deleteFirst();

  // if I did it right, 'implication' contains properly
  // recursively owned substructures..
}


AbsValue *AEnv::grab(AbsValue *v) { return v; }
void AEnv::discard(AbsValue *)    {}
AbsValue *AEnv::dup(AbsValue *v)  { return v; }


AbsValue *AEnv::avSelect(AbsValue *mem, AbsValue *addr)
{
  return avFunc2(str("select"), mem, addr);
}

AbsValue *AEnv::avUpdate(AbsValue *mem, AbsValue *addr, AbsValue *newValue)
{
  return avFunc3(str("update"), mem, addr, newValue);
}


void AEnv::print()
{
  for (StringSObjDict<AbsValue>::Iter iter(bindings);
       !iter.isDone(); iter.next()) {
    string const &name = iter.key();
    AbsValue const *value = iter.value();

    cout << "  " << name << ":\n";
    if (value) {
      value->debugPrint(cout, 4);
    }
    else {
      cout << "    null\n";
    }
  }
}




