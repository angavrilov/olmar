// node2.c
// some simple code that manipulates list nodes
// this version uses inDegree instead of quantification

// the subject of our study
struct Node {
  struct Node *next;
};                         

// for now, I need to see an explicit cast of 0 to a pointer
// type so my abstract interpreter can insert the appropriate
// pointer(...) constructor
#define NULLNODE ((Node*)0)

// declare a predicate which means has-type-node
int hasTypeNode(Node *n);

// function which yields the in-degree of 'n'
int inDegree(int *mem, int offset, Node *n);

#define nodeInDegree(n) \
  inDegree(mem, Node::next, n)

// predicate which defines what it is to be a node
//  thmprv_defpred int isNode(Node *n)
//  {
//    // leaving out the stuff about type since I don't yet
//    // have type-of, etc.
//    return
//      // at most one predecessor
//      inDegree(mem, next, n)==0 || inDegree(mem, next, n)==1
//      thmprv_forall(Node *x, *y;
//                    x->next==n && y->next==n ==> x==y);
//  }

#define isNode(n)                                              \
  hasTypeNode(n) &&                                            \
  (nodeInDegree(n)==0 || nodeInDegree(n)==1) &&                \
  (n->next!=NULLNODE ==> hasTypeNode(n->next))




void append(struct Node *head, struct Node *toAdd)
  thmprv_pre(
    // 'head' and 'toAdd' have accurate types
    head!=NULLNODE && hasTypeNode(head) &&
    toAdd!=NULLNODE && hasTypeNode(toAdd) &&

    // everything with an accurate type is actually a node
    // (in terms of its structural relationships)
    thmprv_forall(Node *n; hasTypeNode(n) ==> isNode(n)) &&

    // nothing points to 'toAdd'
    nodeInDegree(toAdd)==0
  )
  thmprv_post(
    // everything with an accurate type is actually a node
    // (in terms of its structural relationships)
    thmprv_forall(Node *n; hasTypeNode(n) ==> isNode(n)) &&

    // exactly one thing points to 'toAdd'
    nodeInDegree(toAdd)==1
  )
{
  struct Node *p = head;

  while (p->next != NULLNODE) {
    thmprv_invariant(1);
    p = p->next;
  }

  p->next = toAdd;
  //toAdd->next = head;   // Simplify loops..
  
  thmprv_assert(isNode(toAdd));
}



void foo()
{
  struct Node *a = new Node;
  thmprv_assume(hasTypeNode(a));
  thmprv_assert(nodeInDegree(a)==0);

  a->next = 0;
  thmprv_assert(isNode(a));

  thmprv_assert(nodeInDegree(a)==0);
  //thmprv_assert(isNode(a));

  struct Node *b = new Node;
  thmprv_assume(hasTypeNode(b));
  thmprv_assert(nodeInDegree(b)==0);

  b->next = a;
  thmprv_assert(nodeInDegree(b)==0);
  thmprv_assert(nodeInDegree(a)==1);
  thmprv_assert(b->next == a);

  thmprv_assert(isNode(a));
  thmprv_assert(isNode(b));

}
