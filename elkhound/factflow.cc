// factflow.cc
// dataflow for facts: infer what is obvious

#include "factflow.h"        // this module

#include "c.ast.gen.h"       // C AST stuff
#include "ohashtbl.h"        // OwnerHashTable
#include "trace.h"           // trace
#include "exprvisit.h"       // ExpressionVisitor
#include "exprequal.h"       // equalExpressions


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

public:    // funcs
  NodeFacts(NextPtr s) : stmtPtr(s) {}
  ~NodeFacts();
};

NodeFacts::~NodeFacts()
{}

void const *nodeFactsKey(NodeFacts *nf)
{
  return nf->stmtPtr;
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


// lhs = lhs intersect rhs; return true if lhs changes;
// current implementation is O(n^2)
bool intersectFacts(SObjList<Expression /*const*/> &lhs,
                    SObjList<Expression /*const*/> const &rhs)
{
  bool ret = false;

  // for each thing in 'lhs'
  SObjListMutator<Expression /*const*/> mut(lhs);
  while (!mut.isDone()) {
    // if it does not appear in 'rhs' somewhere
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


// ---------------- dataflow algorithm driver ---------------
// annotate all invariants with facts flowed from above
void factFlow(TF_func &func)
{
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
    addFacts(startFacts->facts, func.ftype()->precondition->expr);
    factMap.add(startPtr, startFacts);
  }

  while (worklist.isNotEmpty()) {
    // extract next element from worklist
    NextPtr stmtPtr = worklist.removeFirst();
    Statement const *stmt = nextPtrStmt(stmtPtr);
    bool stmtCont = nextPtrContinue(stmtPtr);

    trace("factflow") << "working on " << nextPtrString(stmtPtr) << endl;

    // retrieve associated dataflow info
    NodeFacts *nodeFacts = factMap.get(stmtPtr);

    // reverse postorder should guarantee that no node gets visited
    // before at least one of its predecessors is visited, which will
    // create a NodeFacts if necessary
    xassert(nodeFacts);

    // consider each successor
    NextPtrList successors;
    stmt->getSuccessors(successors, stmtCont);
    FOREACH_NEXTPTR(successors, iter) {
      NextPtr succPtr = iter.data();

      // compute Out(n) U { edge-predicate }:
      //   - remove any facts jeopardized by state changes in stmt
      //   - add facts asserted, assumed, or invariant'd in stmt
      //   - add facts implied by the path used to exit the stmt
      SObjList<Expression /*const*/> afterFacts;
      afterFacts = nodeFacts->facts;                   // copy the list contents
      stmt->factFlow(afterFacts, stmtCont, succPtr);   // TODO

      // is anything known about this successor?
      bool changed = false;
      NodeFacts *succFacts = factMap.get(succPtr);
      if (!succFacts) {
        // no: create something to hold knowledge about it
        succFacts = new NodeFacts(succPtr);
        succFacts->facts = afterFacts;
        changed = true;

        // the successor node has changed, but since it's the first
        // time anybody's seen it, it's already on the worklist somewhere
        xassert(worklist.contains(succPtr));
      }
      else {
        // yes: intersect my afterFacts with the node's existing facts
        if (intersectFacts(succFacts->facts, afterFacts)) {
          // the intersection operation changed this node, so add it
          // to the worklist if it's not already there
          worklist.appendUnique(succPtr);
          changed = true;

          trace("factflow") << "reintroduced " << nextPtrString(succPtr)
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

      if (tracingSys("factflow")) {
        cout << "added to " << stmt->kindLocString() << ":" << endl;
        SFOREACH_OBJLIST(Expression, nf->facts, fact) {
          cout << "  " << fact.data()->toString() << endl;
        }
      }
    }
  }

  // through the magic of destructors, the factMap (and all the
  // NodeFacts) and the allNodes list will all be deallocated
}


// ------------- per-statement flow propagation ------------
void invalidate(SObjList<Expression /*const*/> &facts, Expression const *expr);
void invalidateLvalue(SObjList<Expression /*const*/> &facts, Expression const *lval);
void invalidateVar(SObjList<Expression /*const*/> &facts, Variable const *var);
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
  if (positive) {
    addFacts(facts, expr);
  }
  else {
    addFacts(facts, HACK_not(expr));
  }
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
// remove from any 'facts' that refer to things modified by
// any of the visited expressions
class InvalidateVisitor : public ExpressionVisitor {
public:
  SObjList<Expression /*const*/> &facts;

public:
  InvalidateVisitor(SObjList<Expression /*const*/> &f) : facts(f) {}
  virtual void visitExpr(Expression const *expr);
};

void InvalidateVisitor::visitExpr(Expression const *expr)
{
  ASTSWITCHC(Expression, expr) {
    ASTCASEC(E_funCall, e)
      // very crude: forget everything
      facts.removeAll();
      PRETEND_USED(e);

    ASTNEXTC(E_effect, e)
      invalidateLvalue(facts, e->expr);

    ASTNEXTC(E_assign, e)
      invalidateLvalue(facts, e->target);

    ASTENDCASECD
  }
}

// any fact in 'facts', which contains any variable modified by 'expr',
// must be removed from 'facts'
void invalidate(SObjList<Expression /*const*/> &facts, Expression const *expr)
{
  InvalidateVisitor vis(facts);
  walkExpression(vis, expr);
}

// given that 'lval' is modified, decide which 'facts' to remove
void invalidateLvalue(SObjList<Expression /*const*/> &facts, Expression const *lval)
{
  ASTSWITCHC(Expression, lval) {
    ASTCASEC(E_variable, v)
      if (v->var->hasAddrTaken()) {
        invalidateVar(facts, NULL /*mem*/);
      }
      else {
        invalidateVar(facts, v->var);
      }

    ASTNEXTC(E_deref, d)
      invalidateVar(facts, NULL /*mem*/);
      PRETEND_USED(d);

    ASTDEFAULTC
      // any other kind: crudely forget
      facts.removeAll();

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

// remove any 'facts' that refer to 'var'
void invalidateVar(SObjList<Expression /*const*/> &facts, Variable const *var)
{
  SObjListMutator<Expression /*const*/> mut(facts);
  while (!mut.isDone()) {
    VariableSearcher vis(var);
    walkExpression(vis, mut.data());
    if (vis.found) {
      // remove this fact because it referred to 'var'
      mut.remove();
    }
    else {
      mut.adv();
    }
  }
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
