// factflow.cc
// dataflow for facts


// -------------------- NodeFacts ------------------
// for each <stmt,cont> node, the set of known facts
struct NodeFacts {
public:    // data
  // the node this refers to; needed because the hashtable interface
  // needs to be able to compute the key from the data
  Statement *stmt;
  bool cont;

  // set of facts; these are pointers to the AST nodes of the
  // expressions from which the facts arise
  SObjList<Expression> facts;

public:    // funcs
  NodeFacts(Statement *s, bool c) : stmt(s), cont(c) {}
  ~NodeFacts();
};

NodeFacts::~NodeFacts()
{}

void const *nodeFactsKey(NodeFacts *nf)
{
  return makeNextPtr(nf->stmt, nf->cont);
}


// ------------------- VariablePair ---------------
struct VariablePair {
public:    // data
  Variable *varL;      // hash key
  Variable *varR;      // variable equivalent to varL
  
public:
  VariablePair(Variable *vL, Variable *vR)
    : varL(vL), varR(vR) {}
};

void const *variablePairKey(VariablePair *vp)
{
  return vp->varL;
}


// ---------------- expression comparison --------------
bool equalExpressions(Expression const *left, Expression const *right)
{
  // map of variable equivalences, for determining equality of expressions
  // that locally declare variables (forall); initially empty
  OwnerHashTable<VariablePair> equiv;

  return equalExpr(equiv, left, right);
}


// simultaneous deconstruction..
bool equalExpr(OwnerHashTable<Variable> &equiv,
               Expression const *left, Expression const *right)
{
  // unlike in ML, I can do toplevel tag comparisons easily here
  if (left->kind() != right->kind()) {
    return false;
  }

  // another convenience in C: macros!
  #define DOUBLECASEC(type)                       \
    ASTNEXTC(type, _L)                            \
    type const &L = *_L;                          \
    type const &R = *(right->as##type##C());

  // performance note: in general, I try to discover that the nodes are
  // not equal by examining data within the nodes, and only when that
  // data matches do I invoke the recursive call to equalExpr

  // since the tags are equal, a switch on the left's type also
  // analyzes right's type
  ASTSWITCHC(Expression, left) {
    ASTCASEC(E_intLit, _L)      // note this expands to an "{" among other things
      E_intLit const &L = *_L;
      E_intLit const &R = *(right->asE_intLitC());
      return L.i == R.i;

    DOUBLECASEC(E_floatLit)       return L.f == R.f;
    DOUBLECASEC(E_stringLit)      return L.s == R.s;
    DOUBLECASEC(E_charLit)        return L.c == R.c;

    DOUBLECASEC(E_variable)
      if (L.var == R.var) {
        return true;
      }

      // check the equivalence map
      VariablePair *vp = equiv.get(L.var);
      if (vp && vp->varR == R.var) {
        // this is the equivalent variable in the right expression,
        // so we say these variable references *are* equal
        return true;
      }
      else {
        // either L.var has no equivalence mapping, or else it does
        // but it's not equivalent to R.var
        return false;
      }

    DOUBLECASEC(E_funCall)
      if (L.args.count() != R.args.count() ||
          !equalExpr(equiv, L.func, R.func)) {
        return false;
      }
      ASTListIter<Expression> iterL(L.args);
      ASTListIter<Expression> iterR(R.args);
      for (; !iterL.isDone(); iterL.adv(), iterR.adv()) {
        if (!equalExpr(equiv, iterL.data(), iterR.data())) {
          return false;
        }
      }
      return true;

    DOUBLECASEC(E_fieldAcc)
      return L.fieldName == R.fieldName &&
             equalExpr(equiv, L.obj, R.obj);

    DOUBLECASEC(E_sizeof)
      return L.size == R.size;

    DOUBLECASEC(E_unary)
      return L.op == R.op &&
             equalExpr(equiv, L.expr, R.expr);

    DOUBLECASEC(E_effect)
      return L.op == R.op &&
             equalExpr(equiv, L.expr, R.expr);

    DOUBLECASEC(E_binary)
      // for now I don't consider associativity and commutativity;
      // I'll rethink this if I encounter code where it's helpful
      return L.op == R.op &&
             equalExpr(equiv, L.e1, R.e1) &&
             equalExpr(equiv, L.e2, R.e2);

    DOUBLECASEC(E_addrOf)
      return equalExpr(equiv, L.expr, R.expr);

    DOUBLECASEC(E_deref)
      return equalExpr(equiv, L.ptr, R.ptr);

    DOUBLECASEC(E_cast)
      return L.type->equals(R.type) &&
             equalExpr(equiv, L.expr, R.expr);

    DOUBLECASEC(E_cond)
      return equalExpr(equiv, L.cond, R.cond) &&
             equalExpr(equiv, L.th, R.th) &&
             equalExpr(equiv, L.el, R.el);

    DOUBLECASEC(E_comma)
      // I don't expect comma exprs in among those I'm comparing, but
      // may as well do the full comparison..
      return equalExpr(equiv, L.e1, R.e1) &&
             equalExpr(equiv, L.e2, R.e2);

    DOUBLECASEC(E_sizeofType)
      return L.size == R.size;

    DOUBLECASEC(E_assign)
      return L.op == R.op &&
             equalExpr(equiv, L.target, R.target) &&
             equalExpr(equiv, L.src, R.src);

    DOUBLECASEC(E_forall)
      if (L.decls.count() != R.decls.count()) {
        return false;
      }

      // verify the declarations are structurally identical
      // (i.e. "int x,y;" != "int x; int y;") and all declared
      // variables have the same type
      bool ret;
      SObjList<Variable> addedEquiv;
      ASTListIter<Declaration> outerL(L.args);
      ASTListIter<Declaration> outerR(R.args);
      for (; !outerL.isDone(); outerL.adv(), outerR.adv()) {
        if (outerL.data()->decllist->count() !=
            outerR.data()->decllist->count()) {
          ret = false;
          goto cleanup;
        }

        ASTListIter<Declarator> innerL(outerL.data()->decllist);
        ASTListIter<Declarator> innerR(outerR.data()->decllist);
        for (; !innerL.isDone(); innerL.adv(), innerR.adv()) {
          Variable const *varL = innerL.data()->var;
          Variable const *varR = innerR.data()->var;

          if (!varL->type->equal(varR->type)) {
            ret = false;     // different types
            goto cleanup;
          }

          // in expectation of all variables being of same type,
          // add these variables to the equivalence map
          equiv.add(varL, new VariablePair(varL, varR));
          addedEquiv.prepend(varL);     // keep track of what gets added
        }
      }

      // when we get here, we know all the variables have the
      // same types, and the equiv map has been extended with the
      // equivalent pairs; so check equality of the bodies
      ret = equalExpr(equiv, L.pred, R.pred);

    cleanup:
      // remove the equivalences we added
      while (addedEquiv.isNotEmpty()) {
        delete equiv.remove(addedEquiv.removeFirst());
      }

      return ret;
             
    ASTDEFAULTC
      xfailure("bad expr tag");
      return false;   // silence warning

    ENDCASEC
  }
  
  #undef DOUBLECASEC
}


// ----------------- dataflow value manipulation ---------------
// 'src' is an expression naming some facts that we will add to 'dest',
// but we need to break 'src' down into a set of conjoined facts, and
// we want to avoid adding the same fact twice
void addFacts(SObjList<Expression> &dest, Expression const *src)
{
  // break down src conjunctions
  if (src->isE_binary() &&
      src->asE_binaryC()->op == BIN_AND) {
    E_binary const *bin = src->asE_binaryC();
    addFacts(dest, bin->e1);
    addFacts(dest, bin->e2);
    return;
  }

  // is 'src' already somewhere in 'dest'?
  SFOREACH_OBJLIST(Expression, dest, iter) {
    if (equalExpressions(iter.data(), src)) {
      // it's in there; bail
      return;
    }
  }

  // it's not already in, so add it
  dest.append(src);
}


// ---------------- dataflow algorithm driver ---------------
// annotate all invariants with facts flowed from above
void factflow(TF_func &func)
{
  // initialize the worklist with a reverse postorder enumeration
  NextPtrList worklist;
  reversePostorder(worklist, func);

  // associate with each <stmt,cont> node a set of facts known
  // to be true at the start of that node; initially all mappings
  // are missing, meaning the set of facts is taken to be the
  // set of all possible facts
  OwnerHashTable<NodeFacts> factMap(nodeFactsKey,
    HashTable::lcprngHashFn, HashTable::pointerEqualKeyFn);

  // initialize the start node with the function precondition
  NodeFacts *startFacts = new NodeFacts(func.body, false /*isContinue*/);
  addFacts(startFacts->facts, func.ftype()->precondition->expr);

  while (worklist.isNotEmpty()) {
    // extract next element from worklist
    Statement const *stmt = nextPtrStmt(worklist.first());
    bool stmtCont = nextPtrContinue(worklist.first());
    worklist.removeFirst();

    


