// aenv.cc
// code for aenv.h

#include "aenv.h"               // this module
#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // AbsValue & children
#include "prover.h"             // runProver
#include "predicate.ast.gen.h"  // Predicate, P_and
#include "trace.h"              // tracingSys
#include "exc.h"                // xBase
#include "owner.h"              // Owner

#include <assert.h>             // assert


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


// ------------------- AbsVariable ----------------
STATICDEF void const* AbsVariable::getAbsVariableKey(AbsVariable *av)
{
  // the need to return this one of the main reasons we even store
  // the 'decl' in AbsVariable: the hash table does not independently
  // store keys, instead relying on the user to be able to map from
  // data to key
  return av->decl;
}


// ------------------- AEnv -----------------
AEnv::AEnv(StringTable &table, Variable const *m)
  : bindings(&AbsVariable::getAbsVariableKey,
             &HashTable::lcprngHashFn,
             &HashTable::pointerEqualKeyFn),
    facts(new P_and(NULL)),
    distinct(),
    counter(1),
    typeFacts(new P_and(NULL)),
    mem(m),
    result(NULL),
    currentFunc(NULL),
    seenStructs(),
    inPredicate(false),
    stringTable(table),
    failedProofs(0)
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
  distinct.removeAll();

  // initialize the environment with a fresh variable for memory
  set(mem, freshVariable("mem", "initial contents of memory"));
  
  // part of the deal with type facts is I don't clear them..
}


void AEnv::set(Variable const *var, AbsValue *value)
{
  AbsVariable *avar = bindings.get(var);
  if (!avar) {
    avar = new AbsVariable(var, value, false /*memvar*/);
    bindings.add(var, avar);
  }

  avar->value = value;
}


AbsValue *AEnv::get(Variable const *var)
{
  AbsVariable *avar = bindings.get(var);
  if (!avar) {
    THROW(xBase(stringc << "failed to lookup " << var->name));
  }
  return avar->value;
}


AbsValue *AEnv::freshVariable(char const *prefix, char const *why)
{
  StringRef name = stringTable.add(stringc << prefix << counter++);
  return new AVvar(name, why);
}


AbsValue *AEnv::addMemVar(Variable const *var)
{
  StringRef addrName = str(stringc << "addr_" << var->name);
  AbsValue *addr = new AVvar(addrName, stringc << "address of " << var->name);

  AbsVariable *avar = new AbsVariable(var, addr, true /*memvar*/);
  bindings.add(var, avar);

  return addr;
}


AbsValue *AEnv::getMemVarAddr(Variable const *var)
{
  return get(var);
}

bool AEnv::isMemVar(Variable const *var) const
{
  AbsVariable *avar = bindings.get(var);
  return avar && avar->memvar;
}

AbsValue *AEnv::updateVar(Variable const *var, AbsValue *newValue)
{
  if (!isMemVar(var)) {
    // ordinary variable: replace the mapping
    set(var, newValue);
  }
  else {
    // memory variable: memory changes
    setMem(avUpdate(getMem(), getMemVarAddr(var),
                    new AVint(0) /*offset*/, newValue));
  }
  
  return newValue;
}


void AEnv::addDistinct(AbsValue *obj)
{
  distinct.append(obj);
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
        Predicate *p1 = exprToPred(b->v1);
        Predicate *p2 = exprToPred(b->v2);
        if (p1->isP_and()) {
          p1->asP_and()->conjuncts.append(p2);
          return p1;
        }
        else if (p2->isP_and()) {
          p2->asP_and()->conjuncts.prepend(p1);
          return p2;
        }
        else {
          return P_and2(p1, p2);
        }
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

    ASTENDCASECD
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

void AEnv::addBoolFact(AbsValue *expr, bool istrue)
{
  if (istrue) {
    addFact(expr);
  }
  else {
    addFact(grab(avNot(expr)));
  }
}


void AEnv::prove(AbsValue const *expr, char const *context)
{
  char const *proved =
    tracingSys("predicates")? "predicate proved" : NULL;
  char const *notProved = "*** predicate NOT proved ***";

  // map the goal into a predicate
  Owner<Predicate> goal(exprToPred(expr));

  // add the fact that all known local variable addresses are distinct
  P_distinct *addrs = new P_distinct(NULL);
  for (OwnerHashTableIter<AbsVariable> iter(bindings);
       !iter.isDone(); iter.adv()) {
    AbsVariable const *avar = iter.data();
    if (avar->memvar) {
      addrs->terms.append(avar->value);
    }
  }
  SMUTATE_EACH_OBJLIST(AbsValue, distinct, iter2) {
    addrs->terms.append(iter2.data());
  }
  facts->conjuncts.prepend(addrs);


  // first, we'll try to prove false, to check the consistency
  // of the assumptions
  P_lit falsePred(false);
  if (innerProve(&falsePred,
                 NULL,                          // printFalse
                 "inconsistent assumptions",    // printTrue
                 context)) {
    // this counts as a proved predicate (we can prove anything if
    // we can prove false)
  }

  else {
    if (goal->isP_and()) {
      // if the goal is a conjunction, prove each part separately,
      // so if part fails we can report that
      FOREACH_ASTLIST_NC(Predicate, goal->asP_and()->conjuncts, iter) {
        if (!innerProve(iter.data(),
                        notProved,   // printFalse
                        proved,      // printTrue
                        context)) {
          failedProofs++;
        }
      }
    }

    else {
      // prove the whole thing at once
      if (!innerProve(goal,
                      notProved,   // printFalse
                      proved,      // printTrue
                      context)) {
        failedProofs++;
      }
    }
  }

  // pull the distinction fact back out
  facts->conjuncts.deleteFirst();
}


// try to prove 'pred', and return true or false
bool AEnv::innerProve(Predicate * /*serf*/ goal,
                      char const *printFalse,
                      char const *printTrue,
                      char const *context)
{
  // build an implication
  string implSexp;
  {
    // temporarily let 'facts' own 'typeFacts'
    facts->conjuncts.prepend(typeFacts);
    P_impl implication(facts, goal);

    try {
      implSexp = implication.toSexpString();
    }
    catch (...) {
      assert(!"not prepared to handle this");
    }

    // restore proper ownership
    facts->conjuncts.removeFirst();
    implication.premise = NULL;
    implication.conclusion = NULL;
  }

  bool ret = false;
  char const *print = printFalse;
  if (runProver(implSexp)) {
    ret = true;
    print = printTrue;
  }

  if (print) {
    VariablePrinter vp;
    cout << print << ": " << context << endl;
    printFact(vp, facts);
    cout << "  goal: " << goal->toSexpString() << "\n";
    walkValuePredicate(vp, goal);

    // print out variable map
    vp.dump();
  }

  return ret;
}


// print a fact, breaking apart conjunctions
void AEnv::printFact(VariablePrinter &vp, Predicate const *fact)
{
  if (fact->isP_and()) {
    FOREACH_ASTLIST(Predicate, fact->asP_andC()->conjuncts, iter) {
      printFact(vp, iter.data());
    }
  }
  else {
    cout << "  fact: " << fact->toSexpString() << "\n";
    walkValuePredicate(vp, fact);
  }
}
      

AbsValue *AEnv::grab(AbsValue *v) { return v; }
void AEnv::discard(AbsValue *)    {}
AbsValue *AEnv::dup(AbsValue *v)  { return v; }


AbsValue *AEnv::avFunc5(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3, AbsValue *v4, AbsValue *v5)
{
  return ::avFunc5(str(func), v1, v2, v3, v4, v5);
}

AbsValue *AEnv::avFunc4(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3, AbsValue *v4)
{
  return ::avFunc4(str(func), v1, v2, v3, v4);
}

AbsValue *AEnv::avFunc3(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3)
{
  return ::avFunc3(str(func), v1, v2, v3);
}

AbsValue *AEnv::avFunc2(char const *func, AbsValue *v1, AbsValue *v2)
{
  return ::avFunc2(str(func), v1, v2);
}

AbsValue *AEnv::avFunc1(char const *func, AbsValue *v1)
{
  return ::avFunc1(str(func), v1);
}


void AEnv::print()
{
  for (OwnerHashTableIter<AbsVariable> iter(bindings);
       !iter.isDone(); iter.adv()) {
    AbsVariable const *avar = iter.data();

    cout << "  " << avar->decl->name << ":\n";
    if (avar->value) {
      avar->value->debugPrint(cout, 4);
    }
    else {
      cout << "    null\n";
    }
  }
}
