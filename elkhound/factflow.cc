// factflow.cc
// dataflow for facts: infer what is obvious

#include "factflow.h"        // this module

#include "c.ast.gen.h"       // C AST stuff
#include "ohashtbl.h"        // OwnerHashTable
#include "trace.h"           // trace
#include "exprvisit.h"       // ExpressionVisitor
#include "exprequal.h"       // equalExpressions
#include "aenv.h"            // AEnv
#include "nonport.h"         // getMilliseconds()

#include <stdio.h>           // sprintf

// info from ccgrmain.cc
extern StringTable *globalStringTable;
extern Variable const *globalMemVariable;


// utility timer
class Timer {
  long &acc;      // accumulates spent time
  long start;     // start time for this section

public:
  Timer(long &a) : acc(a), start(getMilliseconds()) {}
  ~Timer() { acc += getMilliseconds() - start; }
};

// sections of interest
class Section {
public:
  char const *name;       // section description
  long acc;               // total time spent
  int attempts;           // prover attempts
  int proven;             // # of successful proofs

public:
  Section(char const *n)
    : name(n), acc(0), attempts(0), proven(0) {}

  string toString() const;
};

string Section::toString() const
{
  char buf[80];
  sprintf(buf, "  %15s: %6ld ms   %d/%d proven\n",
               name, acc, proven, attempts);
  return string(buf);
}

string ltoaPad(long n, int pad)
{
  char buf[20];
  sprintf(buf, "%*ld", pad, n);
  return string(buf);
}

Section intersection("intersection");
Section invalidation("invalidation");
Section weakening("weakening");
Section consistency("consistency");
Section *sections[4] = { &intersection, &invalidation, 
                         &weakening, &consistency };


// -------------------- NodeFacts ------------------
// for each <stmt,cont> node, the set of known facts
struct NodeFacts {
public:    // data
  // the node this refers to; needed because the hashtable interface
  // needs to be able to compute the key from the data
  NextPtr stmtPtr;

  // set of facts; these are pointers to the AST nodes of the
  // expressions from which the facts arise
  SObjList<Expression> facts;
  
  // when true, 'facts' is ignored, and this node is interpreted
  // to have all facts true
  bool allTrue;

public:    // funcs
  NodeFacts(NextPtr s, bool a=false) : stmtPtr(s), allTrue(a) {}
  ~NodeFacts();
};

NodeFacts::~NodeFacts()
{}

void const *nodeFactsKey(NodeFacts *nf)
{
  return nf->stmtPtr;
}

string factListString(SObjList<Expression> const &facts,
                      char const *separator = ", ")
{
  stringBuilder sb;
  int ct=0;
  SFOREACH_OBJLIST(Expression, facts, fact) {
    if (ct++ > 0) {
      sb << separator;
    }
    sb << fact.data()->toString();
  }       
  return sb;
}


// ----------------- dataflow value manipulation ---------------
// return true if 'src' (or anything equalExpressions to it) is
// in 'dest'
bool hasFact(SObjList<Expression /*const*/> const &dest, Expression const *src)
{
  SFOREACH_OBJLIST(Expression, dest, iter) {
    if (equalExpressions(iter.data(), src)) {
      return true;
    }
  }
  return false;
}


// 'src' is an expression naming some facts that we will add to
// 'dest', but we need to break 'src' down into a set of conjoined
// facts, and we want to avoid adding the same fact twice; return true
// if 'dest' changes
// (I think the return value isn't used by anything..)
bool addFacts(SObjList<Expression /*const*/> &dest, Expression const *src)
{
  // break down src conjunctions
  if (src->isE_binary() &&
      src->asE_binaryC()->op == BIN_AND) {
    E_binary const *bin = src->asE_binaryC();
    bool ret = addFacts(dest, bin->e1);
    ret = addFacts(dest, bin->e2) || ret;
    return ret;
  }

  // is 'src' already somewhere in 'dest'?
  if (!hasFact(dest, src)) {
    // it's not already in, so add it
    dest.append(const_cast<Expression*>(src));
    return true;
  }
  else {
    return false;   // no changes
  }
}


// lhs = lhs intersect rhs; return true if lhs changes     
// doesn't use the prover; here for reference only (not used)
bool intersectRawFacts(SObjList<Expression /*const*/> &lhs,
                       SObjList<Expression /*const*/> const &rhs)
{
  bool ret = false;

  // for each thing in 'lhs'
  SObjListMutator<Expression /*const*/> mut(lhs);
  while (!mut.isDone()) {
    // if it is not in 'rhs'
    if (!hasFact(rhs, mut.data())) {
      // remove it from 'lhs'
      mut.remove();     // also advances 'mut'
      ret = true;       // something changed
    }
    else {
      mut.adv();
    }
  }

  return ret;
}


#define IN_PREDICATE(env) Restorer<bool> restorer(env.inPredicate, true)
#define DISABLE_PROVER(env) Restorer<bool> restorer(env.disableProver, true)

bool miniProve(AEnv &aenv, Expression const *pred, char const *context,
               Section &section)
{
  Timer timer(section.acc);
  bool ret = aenv.prove(pred->vcgenPred(aenv, 0 /*path*/),
                        context, !tracingSys("factflowPredicates"));
  section.attempts++;
  if (ret) {
    section.proven++;
  }
  return ret;                      
}

// return true if 'facts' imply 'candidate'
bool factsImply(SObjList<Expression /*const*/> const &facts,
                Expression const *candidate)
{
  // shortcut: if 'candidate' is in 'facts', it is clearly implied
  // no -- do this at call site to prevent spurious 'changed' reports
  //if (hasFact(facts, candidate)) {
  //  return true;
  //}

  AEnv aenv(*globalStringTable, globalMemVariable);
  IN_PREDICATE(aenv);

  SFOREACH_OBJLIST(Expression, facts, iter) {
    aenv.addFact(iter.data()->vcgenPred(aenv, 0 /*path*/),
                 "element of fact-set");
  }

  if (!miniProve(aenv, candidate, "testing for fact-set intersection",
                 intersection)) {
    trace("factflw2")
      << "    fact " << candidate->toString()
      << " invalidated by intersection; premises: "
      << factListString(facts) << endl;
    return false;
  }
  else {
    return true;
  }
}


// lhs = lhs intersect rhs; return true if lhs changes
bool intersectFacts(SObjList<Expression /*const*/> &lhs,
                    SObjList<Expression /*const*/> const &rhs)
{
  bool ret = false;

  // do additions before subtractions to support weakened forms

  // additions: for each thing in 'rhs'
  SFOREACH_OBJLIST(Expression, rhs, iter) {
    // if it is not already in 'lhs' but is implied by 'lhs'
    if (!hasFact(lhs, iter.data()) &&
        factsImply(lhs, iter.data())) {
      // add it to 'lhs'
      lhs.append(const_cast<Expression*>(iter.data()));
      ret = true;
    }
  }

  // subtractions: for each thing in 'lhs'
  SObjListMutator<Expression /*const*/> mut(lhs);
  while (!mut.isDone()) {
    // if it is not in 'rhs' and not implied by 'rhs'
    if (!hasFact(rhs, mut.data()) &&
        !factsImply(rhs, mut.data())) {
      // remove it from 'lhs'
      mut.remove();     // also advances 'mut'
      ret = true;       // something changed
    }
    else {
      mut.adv();
    }
  }

  return ret;
}


// ---------------- dataflow algorithm driver ---------------
// annotate all invariants with facts flowed from above
void factFlow(TF_func &func)
{
  trace("factflow") << "processing function " << func.name() << endl;
  long startTime = getMilliseconds();

  // initialize the worklist with a reverse postorder enumeration
  NextPtrList worklist;
  reversePostorder(worklist, func);

  // grab a copy of this list now, because it will be useful for
  // walking through all nodes later on
  NextPtrList allNodes;
  allNodes = worklist;

  // associate with each <stmt,cont> node a set of facts known
  // to be true at the start of that node; initially all mappings
  // are missing, meaning the set of facts is taken to be the
  // set of all possible facts
  OwnerHashTable<NodeFacts> factMap(nodeFactsKey,
    HashTable::lcprngHashFn, HashTable::pointerEqualKeyFn);

  // initialize the start node with the function precondition
  {
    NextPtr startPtr = makeNextPtr(func.body, false /*isContinue*/);
    NodeFacts *startFacts = new NodeFacts(startPtr);
    if (func.ftype()->precondition) {
      addFacts(startFacts->facts, func.ftype()->precondition->expr);
    }
    factMap.add(startPtr, startFacts);
  }

  while (worklist.isNotEmpty()) {
    // extract next element from worklist
    NextPtr stmtPtr = worklist.removeFirst();
    Statement const *stmt = nextPtrStmt(stmtPtr);
    bool stmtCont = nextPtrContinue(stmtPtr);

    // retrieve associated dataflow info
    NodeFacts *nodeFacts = factMap.get(stmtPtr);

    // reverse postorder should guarantee that no node gets visited
    // before at least one of its predecessors is visited, which will
    // create a NodeFacts if necessary
    xassert(nodeFacts);

    trace("factflow") << "  working on " << nextPtrString(stmtPtr) << endl;
    char const *sep = "\n                    ";
    trace("factflw2") << "    nodeFacts:" << sep
                       << factListString(nodeFacts->facts, sep) << endl;

    // consider each successor
    NextPtrList successors;
    stmt->getSuccessors(successors, stmtCont);
    FOREACH_NEXTPTR(successors, iter) {
      NextPtr succPtr = iter.data();
      trace("factflow") << "    considering edge to "
                        << nextPtrString(succPtr) << endl;

      // compute Out(n) U { edge-predicate }:
      //   - remove any facts jeopardized by state changes in stmt
      //   - add facts asserted, assumed, or invariant'd in stmt
      //   - add facts implied by the path used to exit the stmt
      SObjList<Expression /*const*/> afterFacts;
      bool consistent = true;     // when false, path's effect nullified
      {
        // build a new abstract store with the known facts
        AEnv aenv(*globalStringTable, globalMemVariable);
        SFOREACH_OBJLIST(Expression, nodeFacts->facts, fact) {
          IN_PREDICATE(aenv);
          aenv.addFact(fact.data()->vcgenPred(aenv, 0 /*path*/),
                       "element of fact-set propagation");
        }

        // let the statement's side effects change the store
        {
          DISABLE_PROVER(aenv);     // don't prove assertions, array accesses, etc.
          stmt->vcgen(aenv, stmtCont, 0 /*path??*/, nextPtrStmt(succPtr));
        }

        // as an initial approximation, use simple syntactic checks
        // to decide that some facts are definitely valid; this also
        // adds an facts implied by the control flow edge
        afterFacts = nodeFacts->facts;         // list copy
        stmt->factFlow(afterFacts, stmtCont, succPtr);

        // any facts that were not deemed valid by the above factFlow
        // will now be considered more carefully, but seeing whether
        // Simplify can prove them
        IN_PREDICATE(aenv);
        SFOREACH_OBJLIST(Expression, nodeFacts->facts, fact2) {
          Expression const *pred = fact2.data();
          if (afterFacts.contains(pred)) {
            // no need to consider this one
            continue;
          }

          if (miniProve(aenv, pred, "testing for fact invalidation",
                        invalidation)) {
            // rescued by Simplfy!
            trace("factflw2")
              << "      validated fact " << pred->toString()
              << " by Simplify" << endl;
            afterFacts.append(const_cast<Expression*>(pred));
          }
          else {
            trace("factflw2")
              << "      invalidated fact " << pred->toString()
              << " by " << stmt->kindLocString() << endl;

            // try to weaken the predicate slightly
            if (pred->isE_binary()) {
              E_binary const *bin = pred->asE_binaryC();
              if (bin->op == BIN_LESS || bin->op == BIN_GREATER) {
                // make a new node (which will be leaked -- TODO)
                // with a weaker relational test
                E_binary *weaker = new E_binary(
                    bin->e1,
                    bin->op == BIN_LESS? BIN_LESSEQ : BIN_GREATEREQ,
                    bin->e2);

                if (miniProve(aenv, weaker, "weakened form", weakening)) {
                  trace("factflw2")
                    << "      weakened fact " << pred->toString()
                    << " to " << weaker->toString() << endl;
                  afterFacts.append(weaker);
                }
                else {
                  // may as well deallocate in this case
                  weaker->e1 = NULL;
                  weaker->e2 = NULL;
                  delete weaker;
                }
              }
            }

          }
        }

        // are these facts consistent?  I.e., is the path feasible?
        E_intLit zero(0);
        if (miniProve(aenv, &zero, "testing fact-set consistency",
                      consistency)) {
          // they are inconsistent; do not contribute anything to the successor
          trace("factflw2") << "      flowing allTrue because of infeasible path" << endl;
          consistent = false;
        }
      }

      // is anything known about this successor?
      bool changed = false;
      NodeFacts *succFacts = factMap.get(succPtr);
      if (!succFacts) {
        // no: create something to hold knowledge about it
        succFacts = new NodeFacts(succPtr, !consistent);
        if (consistent) {
          succFacts->facts = afterFacts;
        }
        factMap.add(succPtr, succFacts);
        changed = true;

        // the successor node has changed, but since it's the first
        // time anybody's seen it, it's already on the worklist somewhere
        xassert(worklist.contains(succPtr));
      }
      else if (consistent && succFacts->allTrue) {
        // simply replace succFacts' facts with mine
        succFacts->facts = afterFacts;
        succFacts->allTrue = false;
      }
      else if (consistent) {
        // yes: intersect my afterFacts with the node's existing facts
        trace("factflw2") 
          << "      intersect prior: " << factListString(succFacts->facts)
          << "\n                           with new: " << factListString(afterFacts) << endl;
        if (intersectFacts(succFacts->facts, afterFacts)) {
          // the intersection operation changed this node, so add it
          // to the worklist if it's not already there
          worklist.appendUnique(succPtr);
          changed = true;

          trace("factflw2") << "  reintroduced " << nextPtrString(succPtr)
                            << " to worklist" << endl;
        }
      }
    }
  }

  // at this point the information associated with nodes has settled into
  // a fixpoint, and therefore we have accurate information about which
  // facts are obviously true where; for all invariant points, we now push
  // this information into the invariant nodes
  FOREACH_NEXTPTR(allNodes, iter) {
    Statement *stmt = nextPtrStmtNC(iter.data());

    if (stmt->isS_invariant()) {
      NodeFacts *nf = factMap.get(iter.data());
      xassert(nf);
      S_invariant *inv = stmt->asS_invariant();

      inv->inferFacts = nf->facts;    // copy facts

      if (tracingSys("factflw2")) {
        trace("factflw2") 
          << "  added to " << stmt->kindLocString() << ": "
          << factListString(nf->facts) << endl;
      }
    }
  }
    
  long totalSimp = 0;
  for (int i=0; i < TABLESIZE(sections); i++) {
    totalSimp += sections[i]->acc;
  }

  trace("factflow")
    << "done with " << func.name() << ":\n"
    << intersection.toString()
    << invalidation.toString()
    << weakening.toString()
    << consistency.toString()
    << "     tot Simplify: " << ltoaPad(totalSimp, 6) << " ms\n"
    << "     tot factflow: " << ltoaPad(getMilliseconds() - startTime, 6) << " ms\n";
    ;

  // through the magic of destructors, the factMap (and all the
  // NodeFacts) and the allNodes list will all be deallocated
}


// ------------- per-statement flow propagation ------------
void invalidate(SObjList<Expression /*const*/> &facts, Expression const *expr);
void invalidateInit(SObjList<Expression /*const*/> &facts, Initializer const *init);


void S_skip::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}
void S_label::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}
void S_case::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}
void S_caseRange::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}
void S_default::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}

void S_expr::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  invalidate(facts, expr);
}

void S_compound::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}

// I should change my facts to be two lists, one of positive facts and
// another of negative facts; for now I hack around this by synthesizing
// "not" nodes which will then be leaked
Expression *HACK_not(Expression *expr)
{
  return new E_unary(UNY_NOT, expr);
}
  
// add either the positive or negative form of 'expr', depending
// on 'positive'
void addFactsBool(SObjList<Expression /*const*/> &facts, Expression *expr, bool positive)
{
  Expression *toAdd = positive? expr : HACK_not(expr);
  trace("factflw2") << "      added fact: " << toAdd->toString() << endl;
  addFacts(facts, toAdd);
}


void S_if::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  invalidate(facts, cond);
  addFactsBool(facts, cond, nextPtrStmt(succPtr) == thenBranch);
}

void S_switch::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  invalidate(facts, expr);
  // TODO: learn something from which edge is followed
}

void S_while::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  invalidate(facts, cond);
  addFactsBool(facts, cond, nextPtrStmt(succPtr) == body);
}

void S_doWhile::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  if (!isContinue) {
    // flows directly into body, nothing is changed
  }
  else {
    // flows into test, then possibly back into the body
    invalidate(facts, cond);
    addFactsBool(facts, cond, nextPtrStmt(succPtr) == body);
  }
}

void S_for::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  if (isContinue) {
    // after, cond, optional body
    invalidate(facts, after);
  }
  else {
    // cond, optional body (init was already handled via CFG edge)
  }
  invalidate(facts, cond);
  addFactsBool(facts, cond, nextPtrStmt(succPtr) == body);
}

void S_break::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}
void S_continue::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}

void S_return::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  // probably irrelevant since an invariant can't follow a return..
  // maybe sometime I'll be suggesting ways to strengthen
  // postconditions?
  invalidate(facts, expr);
}

void S_goto::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
  {}

void S_decl::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  FOREACH_ASTLIST(Declarator, decl->decllist, dcltr) {
    Declarator const *d = dcltr.data();

    if (d->init) {
      invalidateInit(facts, d->init);
    }
  }
}

void S_assert::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  addFacts(facts, expr);
}

void S_assume::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  addFacts(facts, expr);
}

void S_invariant::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  addFacts(facts, expr);
}

void S_thmprv::factFlow(SObjList<Expression /*const*/> &facts, bool isContinue, NextPtr succPtr) const
{
  // what does this do?  I've forgotten already..
}


// ------------------ expression invalidation -----------------
bool invalidateExpr(Expression const *fact, Expression const *expr);
bool isMonotonic(E_assign const *assign, int &dir, Variable *&var);
bool isRelational(Expression const *expr, Variable const *var, int &dir);
bool invalidateLvalue(Expression const *facts, Expression const *lval);
bool invalidateVar(Expression const *fact, Variable const *var);


// remove from any 'facts' that refer to things modified by
// any of the visited expressions
class InvalidateVisitor : public ExpressionVisitor {
public:
  Expression const *fact;       // fact whose validity is under consideration
  bool inval;                   // true once we conclude it is invalid

public:
  InvalidateVisitor(Expression const *f)
    : fact(f), inval(false) {}
  virtual void visitExpr(Expression const *expr);
};

void InvalidateVisitor::visitExpr(Expression const *expr)
{
  ASTSWITCHC(Expression, expr) {
    ASTCASEC(E_funCall, e)
      // hack: until I have some better notation for functions which
      // don't affect state, I'll recognize one by name (worst case
      // I infer some untrue facts and fail during vcgen)
      if (e->func->isE_variable() &&
          0==strcmp(e->func->asE_variableC()->var->name, "assert")) {
        return;   // no effect
      }

      // very crude: forget everything
      inval = true;

    ASTNEXTC(E_effect, e)
      inval = invalidateLvalue(fact, e->expr);

    ASTNEXTC(E_assign, e)
      #if 0
      // look for monotonicity; is the assignment monotonic?
      int assignDir;     // becomes: +1 for increasing, -1 for decreasing
      Variable *var;     // variable monotonically modified
      if (isMonotonic(e, assignDir, var)) {
        // is the fact a relational?
        int relDir;      // becomes: +2 for ">", +1 for ">=", -1 for "<=", -2 for "<"
        if (isRelational(fact, var, relDir)) {
          if (assignDir * relDir > 0) {     // same sign
            trace("factflw2")
              << "  found that variable " << var->name
              << " was modified monotonically by " << e->toString()
              << " which respects constraint " << fact->toString() << endl;
            return;    // i.e. let 'inval' remain false if it's false now
          }
        }
      }
      #endif // 0

      // no monotonicity, so any modification will invalidate
      inval = invalidateLvalue(fact, e->target);

    ASTENDCASECD
  }
}

// any fact in 'facts', which contains any variable modified by 'expr',
// must be removed from 'facts'
void invalidate(SObjList<Expression /*const*/> &facts, Expression const *expr)
{
  // consider each fact in turn
  SObjListMutator<Expression /*const*/> mut(facts);
  while (!mut.isDone()) {
    Expression const *fact = mut.data();

    // dig around in 'expr' to find things which might invalidate
    // the current 'fact'
    if (invalidateExpr(fact, expr)) {
      // remove this fact because 'expr' invalidated it
      trace("factflw2")
        << "    fact " << fact->toString()
        << " potentially invalidated by " << expr->toString() << endl;
      mut.remove();
    }
    else {
      mut.adv();
    }
  }
}

// return true if the side effects in 'expr' invalidate 'fact'
bool invalidateExpr(Expression const *fact, Expression const *expr)
{
  InvalidateVisitor vis(fact);
  walkExpression(vis, expr);
  return vis.inval;
}

// return true if 'assign' modifies some variable monotonically;
// if so, yield which direction and variable
bool isMonotonic(E_assign const *assign, int &dir, Variable *&var)
{
  // extract modified variable
  if (!assign->target->isE_variable()) return false;
  var = assign->target->asE_variableC()->var;

  // all my existing syntax uses BIN_ASSIGN ..
  if (assign->op != BIN_ASSIGN) return false;

  // src must be binary expression
  if (!assign->src->isE_binary()) return false;
  E_binary const *bin = assign->src->asE_binaryC();

  // LHS must be var
  if (!bin->e1->isE_variable() ||
      bin->e1->asE_variableC()->var != var) return false;
                                 
  // op must be + or -
  if (bin->op == BIN_PLUS) {
    dir = +1;
  }
  else if (bin->op == BIN_MINUS) {
    dir = -1;
  }
  else {
    return false;
  }

  // RHS must be a literal
  if (!bin->e2->isE_intLit()) return false;
  
  // sign of literal determines direction
  if (bin->e2->asE_intLitC()->i < 0) {
    dir = dir * -1;
  }
  
  return true;
}
 

// return true if 'expr' is a binary relational, and if so, set 'dir'
// accordingly
bool isRelational(Expression const *expr, Variable const *var, int &dir)
{
  // must be a binary exp
  if (!expr->isE_binary()) return false;
  E_binary const *bin = expr->asE_binaryC();

  // nominal assignment of 'dir' assuming var is on left
  switch (bin->op) {
    case BIN_LESS:      dir = -2; break;
    case BIN_LESSEQ:    dir = -1; break;
    case BIN_GREATER:   dir = +1; break;
    case BIN_GREATEREQ: dir = +1; break;
    default:            return false;
  }

  // var on left?
  if (bin->e1->isE_variable() &&
      bin->e1->asE_variableC()->var == var) {
    return true;
  }
                 
  // var on right?
  if (bin->e2->isE_variable() &&
      bin->e2->asE_variableC()->var == var) {
    // reverse dir
    dir = -dir;
    return true;
  }

  // var not involved
  return false;
}

// given that 'lval' is modified, decide whether to remove 'fact'
bool invalidateLvalue(Expression const *fact, Expression const *lval)
{
  ASTSWITCHC(Expression, lval) {
    ASTCASEC(E_variable, v)
      if (v->var->hasAddrTaken()) {
        return invalidateVar(fact, NULL /*mem*/);
      }
      else {
        return invalidateVar(fact, v->var);
      }

    ASTNEXTC(E_deref, d)
      return invalidateVar(fact, NULL /*mem*/) ||
             invalidateExpr(fact, d->ptr);

    ASTDEFAULTC
      // any other kind: crudely forget
      return true;

    ASTENDCASEC
  }
}


// set 'found' to true if any visited expressions refer to 'var'
class VariableSearcher : public ExpressionVisitor {
public:
  Variable const *var;
  bool found;

public:
  VariableSearcher(Variable const *v)
    : var(v), found(false) {}
  virtual void visitExpr(Expression const *expr);
};

void VariableSearcher::visitExpr(Expression const *expr)
{
  if (var &&       // actual variable
      expr->isE_variable() &&
      expr->asE_variableC()->var == var) {
    found = true;
  }

  if (!var &&      // we're actually searching for references to memory
      expr->isE_deref()) {
    found = true;
  }
}

// if 'fact' refers to 'var', return true
bool invalidateVar(Expression const *fact, Variable const *var)
{
  VariableSearcher vis(var);
  walkExpression(vis, fact);
  return vis.found;
}


// remove any 'facts' modified by expressions in 'init'
void invalidateInit(SObjList<Expression /*const*/> &facts, Initializer const *init)
{
  ASTSWITCHC(Initializer, init) {
    ASTCASEC(IN_expr, e)
      invalidate(facts, e->e);

    ASTNEXTC(IN_compound, c)
      FOREACH_ASTLIST(Initializer, c->inits, iter) {
        invalidateInit(facts, iter.data());
      }

    ASTENDCASECD
  }
}
