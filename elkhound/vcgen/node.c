// node.c
// some simple code that manipulates list nodes

// the subject of our study
struct Node {
  struct Node *next;
};                         


// predicate which defines what it is to be a node
//  thmprv_defpred int isNode(Node *n)
//  {                                                    
//    // leaving out the stuff about type since I don't yet
//    // have type-of, etc.
//    return
//      // at most one predecessor
//      thmprv_forall(Node *x, *y;
//                    x->next==n && y->next==n ==> x==y);
//  }

// declare a predicate which means has-type-node
thmprv_predicate int hasTypeNode(Node *n);

#define isNode(n)                                      \
  hasTypeNode(n) &&                                    \
  thmprv_forall(Node *x, *y;                           \
                x->next==n && y->next==n ==> x==y) &&  \
  (n->next!=(Node*)0 ==> hasTypeNode(n->next))



void append(struct Node *head, struct Node *toAdd)
  thmprv_pre(
    // 'head' and 'toAdd' have accurate types
    head!=(Node*)0 && hasTypeNode(head) &&
    toAdd!=(Node*)0 && hasTypeNode(toAdd) &&

    // everything with an accurate type is actually a node
    // (in terms of its structural relationships)
    thmprv_forall(Node *n; hasTypeNode(n) ==> isNode(n)) &&

    // nothing points to 'toAdd'
    !thmprv_exists(Node *n; n->next == toAdd)
  )
  thmprv_post(
    // everything with an accurate type is actually a node
    // (in terms of its structural relationships)
    thmprv_forall(Node *n; hasTypeNode(n) ==> isNode(n)) &&

    // exactly one thing points to 'toAdd'
    thmprv_exists(Node *n;
      n->next == toAdd &&
      thmprv_forall(Node *m; m->next==toAdd ==> n==m))
  )
{
  struct Node *p = head;

  while (p->next != (Node*)0) {
    thmprv_invariant(1);
    p = p->next;
  }

  p->next = toAdd;
  //toAdd->next = head;   // Simplify loops..
}



void foo()
{
  struct Node *a = new Node;
  thmprv_assume(hasTypeNode(a));
  thmprv_assert(!thmprv_exists(Node *n; n->next == a));

  a->next = 0;
  thmprv_assert(isNode(a));

  thmprv_assert(!thmprv_exists(Node *n; n->next == a));
  //thmprv_assert(isNode(a));

  struct Node *b = new Node;
  thmprv_assume(hasTypeNode(b));
  thmprv_assert(!thmprv_exists(Node *n; n->next == a));

  b->next = a;
  thmprv_assert(!thmprv_exists(Node *n; n->next == b));
  thmprv_assert(thmprv_forall(Node *n; (n->next == a) ==> (n == b)));

  thmprv_assert(isNode(a));
  thmprv_assert(isNode(b));

}
