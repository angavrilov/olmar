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
#include "cc_type.h"            // Type

#include <assert.h>             // assert


#define FOREACH_BINDING(binding)                             \
  {                                                          \
    for (OwnerHashTableIter<AbsVariable> iter(bindings);     \
         !iter.isDone(); iter.adv()) {                       \
      AbsVariable *binding = iter.data();

#define END_FOREACH_BINDING                                  \
  }}


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
    pathFacts(new ObjList<Predicate>),
    pathFactsStack(),
    exprFacts(),
    distinct(),
    //typeFacts(new P_and(NULL)),
    mem(m),
    result(NULL),
    currentFunc(NULL),
    seenStructs(),
    inPredicate(false),
    disableProver(false),
    stringTable(table),
    failedProofs(0),
    inconsistent(0)
{
  newFunction(NULL);
}

AEnv::~AEnv()
{
  delete pathFacts;

  //delete typeFacts;
}

                 
void AEnv::newFunction(TF_func const *newFunc)
{
  currentFunc = newFunc;
  funcFacts.deleteAll();
  newPath();
}


void AEnv::newPath()
{
  bindings.empty();
  pathFacts->deleteAll();
  pathFactsStack.deleteAll();
  exprFacts.removeAll();
  distinct.removeAll();

  // initialize the environment with a fresh variable for memory
  makeFreshMemory("initial contents of memory");

  // null is an address distinct from any other we'll encounter
  addDistinct(avInt(0));

  inconsistent = 0;

  // do *not* clear failedProofs; 'clear' is used to refresh between
  // paths, but we want to accumulate failedProofs across all of them
}


void AEnv::makeFreshMemory(char const *why)
{
  set(mem, freshVariable("mem", why));
  addFact(P_named1(str("uncheckedAccess"), getMem()),
          "memory itself is unchecked");
}


void AEnv::set(Variable const *var, AbsValue *value)
{
  value = rval(value);
  trace("aenvSet") << "setting " << var->name 
                   << " to " << value->toString() << endl;
  AbsVariable *avar = bindings.get(var);
  if (!avar) {
    avar = new AbsVariable(var, value, false /*memvar*/);
    bindings.add(var, avar);
  }

  avar->value = value;
}


void AEnv::setLval(AVlval const *lval, AbsValue *value)
{
  setLval(lval->progVar, lval->offset, value);
}

void AEnv::setLval(Variable const *var, AbsValue *offset, AbsValue *value)
{          
  AbsValue *updated = avUpdOffset(get(var), offset, value);
  if (var == mem) {
    setMem(updated);    // makes fresh memory variabes for each new state
  }
  else {
    set(var, updated);
  }
}


AbsValue *AEnv::get(Variable const *var)
{
  AbsVariable *avar = bindings.get(var);
  if (avar) {
    return avar->value;
  }
  
  // we're asking for a variable's value, but we don't know anything
  // about that variable.. so we'll make up a new logic variable
  // to talk about it

  // UPDATE: the circumstances where we get here are somewhat confusing,
  // and currently in flux..

  // convenience
  StringRef name = var->name;
  xassert(name);    // otherwise how did it get referred-to?
  Type const *type = var->type;
                        
  // the way the conditionals below work, this actually always gets
  // set to something else, but gcc doesn't know that
  AbsValue *value = NULL;

  // first reference to a quantified variable
  if (var->hasFlag(DF_LOGIC)) {
    if (var->hasFlag(DF_UNIVERSAL)) {
      value = freshVariable(var->name, "universally quantified");
    }
    else if (var->hasFlag(DF_EXISTENTIAL)) {
      value = freshVariable(var->name, "existentially quantified");
    }
    else {                                               
      // happens when the programmer declares a variable in a precondition,
      // but doesn't assign it a value; in a certain sense it's existentially
      // quantified...
      // update: if you do that, you can't satisfy the precondition, because
      // when *proving* it it's universally quantified!  so this really should
      // be explicitly quantified
      cout << "WARNING: not explicitly quantified: " << var->name << endl;
      value = freshVariable(var->name, "logic variable");
    }

    set(var, value);
    return value;
  }

  // reference to a structure field (field as a *whole*--not the field
  // of some particular object, but the "array" which represents
  // *every* object's value for this field)
  if (var->hasFlag(DF_FIELD)) {
    value = freshVariable(var->name,
      stringc << "models object offset of field " << var->name);
    set(var, value);
    return value;
  }

  // give a name to its current value (unless it's a big thing in memory)
  if (!type->isArrayType()) {
    value = freshVariable(name,
      stringc << "starting value of "
              << (var->hasFlag(DF_PARAMETER)? "parameter " : "variable ")
              << name );
  }

  if (var->hasAddrTaken() || type->isArrayType()) {
    // model this variable as a location in memory; make up a name
    // for its address
    AbsValue *addr = addMemVar(var);
    AbsValue *initialObject = avSel(getMem(), addr);

    if (!type->isArrayType()) {
      // state that, right now, that memory location contains 'value'
      addFact(P_equal(value, initialObject),
              "initial value of in-memory variable");
    }
    else {
      // will model array contents as elements in memory, rather
      // than as symbolic whole-array values... the value we yield
      // will be the address of the array (I guess, relying on the
      // implicit coercion between arrays and pointers..)
    }

    int size = 1;         // default for non-arrays
    if (type->isArrayType() && type->asArrayTypeC().hasSize) {
      // state that the length is whatever the array length is
      // (if the size isn't specified I have to get it from the
      // initializer, but that will be TODO for now)
      size = type->asArrayTypeC().size;
    }

    #if 0     // moved down to addDeclarationFacts
    // remember the length, as a predicate in the set of known facts
    addFact(P_equal(avLength(initialObject), new AVint(size)),
            "memory object size");                
    #endif // 0
                         
    // caller knows that we're yielding the address, not the value,
    // since this is a memvar
    set(var, addr);       // need to associate this with the name too
    addDeclarationFacts(var, initialObject);
    return addr;
  }

  else {
    // model the variable as a simple, named, unaliasable variable
    set(var, value);

    // introduce additional facts based on the variable's type
    addDeclarationFacts(var, value);

    return value;
  }
}


// facts which arise from a variable's type
void AEnv::addDeclarationFacts(Variable const *var, AbsValue *value)
{
  StringRef name = var->name;
  Type const *type = var->type;

  #if 0   // this is wrong.. we only want this for uninitialized..
  if (type->isOwnerPtr()) {
    // OWNER: initialize 'state' to DEAD (1)
    trace("owner") << "initializing state of " << name << " to dead\n";
    addFact(P_equal(avSel(value, avOwnerField_state()),
                    avOwnerState_dead()),
            stringc << "initial state of owner pointer " << name);
  }
  #endif // 0

  if (type->isPointerType()) {
    // Simplify would like to know that every pointer variable's value
    // is of the form (sub index rest)
    AbsValue *pointerVal = rval(value);
    
    if (type->isOwnerPtr()) {
      // OWNER: this decomposition needs to be adjust to be in a field
      pointerVal = avSel(pointerVal, avOwnerField_ptr());
    }

    addFact(P_equal(pointerVal,
                    avSub(freshVariable(stringc << name << "_index",
                                        stringc << "supposed first index of " << name),
                          freshVariable(stringc << name << "_rest",
                                        stringc << "supposed rest of " << name))),
            "feasibility of pointer decomposition");
  }

  if (type->isArrayType()) {
    ArrayType const &at = type->asArrayTypeC();
    AbsValue *addr = get(var);      // need the object's address; 'value' isn't it
    AbsValue *object = avSel(getMem(), addr);
    
    // length
    if (at.hasSize) {
      addFact(P_equal(avLength(object), avInt(at.size)),
              "known array length");
    }
      
    if (at.eltType->isOwnerPtr()) {
      // OWNER: arrays of owners get initialized to the dead state
      trace("owner") << "initializing array of owners to dead\n";

      // thmprv_forall(int j; 0<=j && j<length(*addr) ==> addr[j].state==DEAD)
      AVvar *j = freshVariable("j", "quantifier for array index");
      addFact(P_forall(new ASTList<AVvar>(j),
                       new P_impl(P_and2(new P_relation(avInt(0), RE_LESSEQ,j),
                                         new P_relation(j, RE_LESS, avLength(object))),
                                  P_equal(avSel(avSel(object, j),
                                                avOwnerField_state()),
                                          avOwnerState_dead())
                                 )
                      ),
              "initial dead state of array of owners");
    }
  }
}


AVvar *AEnv::freshVariable(char const *prefix, char const *why)
{
  int suffix = 0;
                             
  // keep generating names until we hit one which has not been used
  // for *any* purpose; this avoids conflict with any user-defined
  // names (since they all went through the string table), but also
  // tries to append just "_0" as often as possible
  for (;;) {
    string name = stringc << prefix << "_" << suffix++;
    if (!stringTable.get(name)) {
      // found an unused name
      StringRef nameRef = stringTable.add(name);
      return new AVvar(nameRef, why);
    }

    // infinite loop paranoia
    xassert(suffix < 10000);
  }
}


AbsValue *AEnv::addMemVar(Variable const *var)
{
  StringRef addrName = str(stringc << "addr_" << var->name);
  AbsValue *addr =
    freshVariable(addrName, stringc << "address of " << var->name);

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
  // we could query this before adding 'var' to the abstract
  // environment, so I list the reasons a variable would be
  // modelled in memory
  return var->hasAddrTaken() || var->type->isArrayType();
}

AbsValue *AEnv::updateVar(Variable const *var, AbsValue *newValue)
{
  if (!isMemVar(var)) {
    // ordinary variable: replace the mapping
    set(var, newValue);
  }
  else {
    // memory variable: memory changes
    setMem(avUpd(getMem(), getMemVarAddr(var), newValue));
  }
  
  return newValue;
}


void AEnv::addDistinct(AbsValue *obj)
{
  distinct.append(obj);
}


void AEnv::assumeNoFieldPointsTo(AbsValue *v)
{
  // TODO: don't know how to say this in my new sel/upd system...
                                                                 
  #if 0
  // need a quantification variable
  AVvar *objVar = freshVariable("obj",
    "introduced for quantifying over objects in assumeNoFieldPointsTo()");
  AVvar *ofsVar = freshVariable("ofs",
    "introduced for quantifying over offsets in assumeNoFieldPointsTo()");

  // build them into a list
  ASTList<AVvar> *quantVars = new ASTList<AVvar>;
  quantVars->append(objVar);
  quantVars->append(ofsVar);

  // (FORALL (obj ofs)
  //   (NEQ
  //     <variable referred-to by 'v'>
  //     (select <current memory> obj ofs)
  //   ))

  // assert it's never the answer to a select query
  pathFacts->append(
    P_forall(quantVars,
      P_notEqual(
        v,
        avSelect(getMem(), objVar, ofsVar)
      )));

  // need one more due to Simplify name capture bug
  AVvar *ofsVar2 = freshVariable("ofs",
    "introduced for quantifying over inDegree offsets in assumeNoFieldPointsTo()");

  // state that the inDegree is 0
  // (FORALL (ofs)
  //   (EQ 0 (inDegree <current memory> ofs <v>)))
  pathFacts->append(
    P_forall(new ASTList<AVvar>(ofsVar2),
      P_equal(
        new AVint(0),
        avInDegree(getMem(), ofsVar2, v)
      )));
  #endif // 0
}


void AEnv::forgetAcrossCall(E_funCall const *call)
{
  string thisCallSyntax = call->toString();

  // make up new variable to represent final contents of memory
  makeFreshMemory(
    stringc << "contents of memory after call: " << thisCallSyntax);

  // any current values for global variables are forgotten
  FOREACH_BINDING(av) {
    if ((av->decl->isGlobal() || av->decl->hasAddrTaken()) &&
        av->decl != mem) {     // don't blow away 'mem' again!
      updateVar(av->decl, freshVariable(av->decl->name,
        stringc << "value of " << av->decl->name
                << " after modified by call: " << thisCallSyntax));
    }
  } END_FOREACH_BINDING
}


void AEnv::setMem(AbsValue *newMem)
{
  // the following is preferable to simply
  //   set(mem, newMem)
  // because this way the various updates to memory are broken apart,
  // instead of collapsed into one mammoth upd(upd(upd(...))) expression

  AVvar *newMemVar;
  if (!newMem->isAVvar()) {
    // make up a new variable
    newMemVar = freshVariable("mem", "updated memory");

    // say that it's equal to the 'newMem' value
    addFact(P_equal(newMemVar, newMem),
            "fresh mem var equals update(..)");
  }
  else {
    // if the new value is itself a simple variable, just bind it;
    // this happens when the value is being set from a function
    // postcondition, where we already made up one variable to stand
    // for the value as it comes back from the function
    newMemVar = newMem->asAVvar();
  }

  // update our notion of memory to refer to the new variable
  set(mem, newMemVar);
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


AbsValue *AEnv::rval(AbsValue *possible)
{
  if (possible->isAVlval()) {
    AVlval *lval = possible->asAVlval();
    return avSelOffset(get(lval->progVar), lval->offset);
  }
  else {
    return possible;
  }
}


void AEnv::addFact(Predicate *pred, char const *why)
{
  pathFacts->append(pred);

  if (tracingSys("addFact")) {
    trace("addFact") << why << ": " << pred->toSexpString() << endl;
  }
}

void AEnv::addBoolFact(Predicate *pred, bool istrue, char const *why)
{
  if (istrue) {
    addFact(pred, why);
  }
  else {
    addFact(P_negate(pred), why);    // strip outer P_not (if exists)
  }
}


#if 0     // old stuff
int AEnv::factStackDepth() const
{
  return pathFacts->count();
}

// O(n^2) implementation
void AEnv::popRecentFacts(int prevDepth, P_and *dest)
{
  while (factStackDepth() > prevDepth) {
    dest->conjuncts.prepend(pathFacts->removeLast());
  }
  xassert(factStackDepth() == prevDepth);
}


// O(n^2) implementation
void AEnv::transferDependentFacts(ASTList<AVvar> const &variables,
                                  int prevDepth, P_and *newFacts)
{
  int ct = pathFacts->count();
  for (int i=ct-1; i>=prevDepth; i--) {
    Predicate *fact = pathFacts->nth(i);

    // does this fact refer to any of the variables in 'variables'?
    FOREACH_ASTLIST(AVvar, variables, iter) {
      AVvar const *var = iter.data();

      // does the fact refer to 'var'?
      if (predicateRefersToAV(fact, var)) {
        // yes; pull it from 'pathFacts' and put it into 'newFacts'
        Predicate *captured = pathFacts->removeAt(i);
        newFacts->conjuncts.append(captured);
        trace("capturedFacts") << captured->toSexpString() << endl;
        break;
      }
    }

    // either it did refer and we've already pulled it, or it didn't
    // refer to any of the variables of interest, so it remained
    // in pathFacts
  }
}
#endif // 0


void AEnv::pushPathFactsFrame()
{
  // kick the current frame onto the stack and make a new current
  pathFactsStack.prepend(pathFacts);
  pathFacts = new ObjList<Predicate>;
}


ObjList<Predicate> *AEnv::popPathFactsFrame()
{
  // give the current one to the caller, and pull up the top
  // of the stack to become the current once again
  ObjList<Predicate> *ret = pathFacts;
  pathFacts = pathFactsStack.removeFirst();
  return ret;
}


void AEnv::setFuncFacts(ObjList<Predicate> * /*owner*/ newFuncFacts)
{
  funcFacts.concat(*newFuncFacts);    // empties 'newFuncFacts'
  delete newFuncFacts;
}


void AEnv::pushExprFact(Predicate *pred)
{
  exprFacts.prepend(pred);
  if (inconsistent) {
    // still inconsistent; counteract decrement in popFact
    inconsistent++;
  }
}

void AEnv::popExprFact()
{
  exprFacts.removeFirst();
  if (inconsistent) {
    // it's possible the corresponding pushFact introduced
    // the inconsistency (if not, the above ++ counteracts this)
    inconsistent--;
  }
}


AEnv::ProofResult AEnv::prove(Predicate *_goal, char const *context, bool silent)
{
  if (disableProver) {
    return PR_SUCCESS;
  }

  if (inconsistent) {
    // anything is provable while our assumptions are inconsistent
    return PR_INCONSISTENT;
  }

  char const *proved =
    tracingSys("predicates")? "------------ predicate proved ------------" : NULL;
  char const *notProved = "--- !!! ------- predicate NOT proved ------- !!! ---";
  char const *inconsisMsg =
    tracingSys("predicates") || tracingSys("inconsistent")?
      "---------- inconsistent assumptions ----------" : NULL;

  if (silent) {
    proved = notProved = inconsisMsg = NULL;
  }

  // take ownership of the goal predicate
  Owner<Predicate> goal(_goal);

  // add the fact that all known local variable addresses are distinct
  P_distinct *addrs = new P_distinct(NULL);
  FOREACH_BINDING(avar) {
    if (avar->memvar) {
      addrs->terms.append(avar->value);
    }
  } END_FOREACH_BINDING
  SMUTATE_EACH_OBJLIST(AbsValue, distinct, iter2) {
    addrs->terms.append(iter2.data());
  }
  pathFacts->prepend(addrs);     // temporarily add this to pathFacts

  ProofResult ret = PR_SUCCESS;

  // first, we'll try to prove false, to check the consistency
  // of the assumptions
  P_lit falsePred(false);
  if (innerProve(&falsePred,
                 NULL,               // printFalse
                 inconsisMsg,        // printTrue
                 context)) {
    // this counts as a proved predicate (we can prove anything if
    // we can prove false)
    ret = PR_INCONSISTENT;
    inconsistent++;
  }

  else {
    if (goal->isP_and()) {
      // if the goal is a conjunction, prove each part separately,
      // so if a part fails we can report that
      FOREACH_ASTLIST_NC(Predicate, goal->asP_and()->conjuncts, iter) {
        if (!innerProve(iter.data(),
                        notProved,   // printFalse
                        proved,      // printTrue
                        context)) {
          failedProofs++;
          ret = PR_FAIL;
        }
      }
    }

    else {
      // prove the whole thing at once
      if (!innerProve(goal,
                      notProved,     // printFalse
                      proved,        // printTrue
                      context)) {
        failedProofs++;
        ret = PR_FAIL;
      }
    }
  }

  // pull the temporarily-added distinction fact back out
  delete pathFacts->removeFirst();

  return ret;
}


// I could write this as a template class but that would require
// adding that funky 'typename' stuff to my list classes so I could
// refer to the type of the iterator classes..
void addFactsToConjunction(ObjList<Predicate> const &source, P_and &dest)
{
  FOREACH_OBJLIST(Predicate, source, iter) {
    // the P_and isn't actually going to own, or modify, the
    // value I'm passing it (constness)
    dest.conjuncts.append( const_cast<Predicate*>(iter.data()) );
  }
}


// try to prove 'pred', and return true or false
bool AEnv::innerProve(Predicate * /*serf*/ goal,
                      char const *printFalse,
                      char const *printTrue,
                      char const *context)
{
  bool ret = false;

  // build a big, nonowning P_and for our facts
  P_and allFacts(NULL);

  // build an implication
  P_impl implication(&allFacts, goal);         // again, fake ownership

  try {
    // build allFacts by throwing everything interesting into it
    {
      // funcFacts:
      addFactsToConjunction(funcFacts, allFacts);

      // pathFacts:
      addFactsToConjunction(*pathFacts, allFacts);
      FOREACH_OBJLIST(ObjList<Predicate>, pathFactsStack, iter) {
        addFactsToConjunction(*(iter.data()), allFacts);
      }

      // exprFacts:
      addFactsToConjunction((ObjList<Predicate>&)exprFacts, allFacts);
    }

    // map this to a Simplify input string
    string implSexp = implication.toSexpString();

    char const *print = printFalse;
    if (runProver(implSexp)) {
      ret = true;
      print = printTrue;
    }

    if (print) {
      VariablePrinter vp;
      cout << print << ": " << context << endl;
      // print facts surrounded by Simplify syntax to facilitate
      // copy+paste from xterm into Simplify interactive session
      cout << "  bg_push (\n";
      printFact(vp, &allFacts);
      cout << "  )\n"
           << "  valid \"goal\" (\n"
           << "    " << goal->toSexpString() << "\n"
           << "  )\n";
      walkValuePredicate(vp, goal);

      // print out variable map
      vp.dump();
    }

    // take apart the implication so dtor won't do anything
    allFacts.conjuncts.removeAll_dontDelete();
    implication.premise = NULL;
    implication.conclusion = NULL;
  }

  catch (...) {
    breaker();      // automatic breakpoint when in debugger

    // take apart implication in this case too
    allFacts.conjuncts.removeAll_dontDelete();
    implication.premise = NULL;
    implication.conclusion = NULL;
    throw;
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
    cout << "    " << fact->toSexpString() << "\n";
    walkValuePredicate(vp, fact);
  }
}
      

AbsValue *AEnv::grab(AbsValue *v) { return v; }
void AEnv::discard(AbsValue *)    {}
AbsValue *AEnv::dup(AbsValue *v)  { return v; }


AbsValue *AEnv::avSelOffset(AbsValue *obj, AbsValue *offset)
{  
  // do just a little simplification here..
  if (offset->isAVwhole()) {
    return obj;
  }
  else {
    return avFunc2("selOffset", obj, offset);
  }
}


AbsValue *AEnv::avUpdOffset(AbsValue *obj, AbsValue *offset, AbsValue *value)
{
  if (offset->isAVwhole()) {
    return value;
  }
  else {
    return avFunc3("updOffset", obj, offset, value);
  }
}


AbsValue *AEnv::avAppendIndex(AbsValue *offset, AbsValue *index)
{          
  if (offset->isAVwhole()) {
    return avSub(index, avWhole());
  }
  else if (offset->isAVsub()) {
    AVsub *sub = offset->asAVsub();
    return avSub(sub->index, avAppendIndex(sub->offset, index));
  }
  else {
    return avFunc2("appendIndex", offset, index);
  }
}


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

AbsValue *AEnv::avInt(int i)
{
  return grab(new AVint(i));
}

AbsValue *AEnv::avSum(AbsValue *a, AbsValue *b)
{
  return new AVbinary(a, BIN_PLUS, b);
}
                  

StringRef AEnv::str(char const *s)
{
  return stringTable.add(s);
}

bool AEnv::isNullPointer(AbsValue const *val) const
{
  // here's a case where ML-style pattern matching would actually
  // make a difference..
  if (val->isAVfunc()) {
    AVfunc const *f = val->asAVfuncC();
    if (f->func == str("sub") &&
        f->args.count() == 2) {
      if (isZero(f->args.nthC(0)) &&
          f->args.nthC(1)->isAVwhole()) {
        return true;
      }
    }
  }
  return false;
}

void AEnv::proveIsNullPointer(AbsValue const *val, char const *why)
{
  if (!isNullPointer(val)) {
    prove(P_equal(val, nullPointer()), why);
  }
}


bool AEnv::isZero(AbsValue const *val) const
{
  if (val->isAVint()) {
    return val->asAVintC()->i == 0;
  }
  return false;
}

void AEnv::proveIsZero(AbsValue const *val, char const *why)
{
  if (!isZero(val)) {
    prove(P_equal(val, avInt(0)), why);
  }
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
