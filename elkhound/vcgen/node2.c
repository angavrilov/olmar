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

#define bool int

// declare a predicate which means has-type-node
thmprv_predicate int hasTypeNode(Node *obj);

thmprv_predicate bool inDegree0(int *mem, int field, Node *obj);
thmprv_predicate bool inDegree1(int *mem, int field, Node *obj, Node *referrer);

Node *object(Node *ptr);
int offset(Node *ptr);
Node *pointer(Node *obj, int ofs);
Node *select(int *mem, Node *obj, int ofs);

#define nodeInDegree0(n)    inDegree0(mem, Node::next, n)
#define nodeInDegree1(n, r) inDegree1(mem, Node::next, n, r)
#define nodeInDegree1b(n) \
  thmprv_exists(Node *refObj; inDegree1(mem, Node::next, n, refObj))

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
                                           
#define nextField(n) \
  select(mem, n, Node::next)

#define isNode(n) (                                                   \
  n!=0 &&                                                             \
  hasTypeNode(n) &&                                                   \
  (nodeInDegree0(n) || nodeInDegree1b(n))  &&                         \
  (nextField(n)!=NULLNODE ==> hasTypeNode(object(nextField(n))))      \
)

#define isNodePtr(p)                                                    \
  offset(p)==0 &&                                                       \
  isNode(object(p))


void append(struct Node *head, struct Node *toAdd)
  thmprv_pre(
    // 'head' and 'toAdd' have accurate types
    isNodePtr(head) &&
    isNodePtr(toAdd) &&

    // everything with an accurate type is actually a node
    // (in terms of its structural relationships)
    thmprv_forall(Node *n; hasTypeNode(n) ==> isNode(n)) &&

    // nothing points to 'toAdd'
    nodeInDegree0(object(toAdd))
  )
  thmprv_post(
    // everything with an accurate type is actually a node
    // (in terms of its structural relationships)
    //thmprv_forall(Node *n; hasTypeNode(n) ==> isNode(n)) &&

    // exactly one thing points to 'toAdd'
    nodeInDegree1b(object(toAdd))
  )
{
  struct Node *p = head;

  while (p->next != NULLNODE) {
    thmprv_invariant(isNodePtr(p));
    p = p->next;
  }

  p->next = toAdd;
  //toAdd->next = head;   // Simplify loops..

  thmprv_assert(isNodePtr(toAdd));
}



void foo()
{
  struct Node *a = new Node;
  thmprv_assume(hasTypeNode(object(a)));
  thmprv_assume(nodeInDegree0(object(a)));

  a->next = 0;
  thmprv_assert(isNodePtr(a));

  thmprv_assert(nodeInDegree0(object(a)));
  //thmprv_assert(isNode(a));

  struct Node *b = new Node;
  thmprv_assume(hasTypeNode(object(b)));
  thmprv_assume(nodeInDegree0(object(b)));

  b->next = a;
  thmprv_assert(nodeInDegree0(object(b)));
  thmprv_assert(nodeInDegree1b(object(a)));
  thmprv_assert(b->next == a);

  thmprv_assert(isNodePtr(a));
  thmprv_assert(isNodePtr(b));

}
