// list.c
// some linked-list code to verify..

#define NULL 0
int *object(int *ptr);
int offset(int *ptr);
int typeof(void *obj);

struct Node {
  int data;
  struct Node *next;
};

//enum Types { tNode };
int tNode = 1;

// is it better to make these syntactic macros or to
// make them function symbols with definitions?
#define validNode(p) (offset(p)==0 && typeof(object(p))==tNode)
#define validNodeOrNull(p) (p==NULL || validNode(p))

// global structure invariant; this isn't strong enough to
// guarantee termination (doesn't prevent loops) but is strong
// enough to prove memory safety
#define validNodeHeap                            \
  (thmprv_forall struct Node *q;                 \
    validNode(q) ==> validNodeOrNull(q->next))


int count(struct Node *p)
  thmprv_pre(
    validNodeOrNull(p) &&
    validNodeHeap
  )
{
  int ret = 0;
  while (p) {
    thmprv_invariant(
      validNode(p) && validNodeHeap
    );

    ret += p->data;
    p = p->next;
  }
  return ret;
}

