// fldinvar.c
// field invariants
  

int typeOf(int addr);
int sel(int mem, int addr);
int sub(int obj, int index);
thmprv_predicate int freshObj(int *obj, int *mem);
int firstIndexOf(int *ptr);
thmprv_predicate int okSelOffsetRange(int mem, int offset, int len);
#define whole 0
int *appendIndex(int *ptr, int index);
thmprv_predicate int okSel(int mem, int index);
int upd(int obj, int index, int newVal);       
thmprv_predicate int equalMemory(int mem1, int mem2);


#define oneLevelPtr(ptr) (                 \
  ptr == sub(firstIndexOf(ptr), whole) &&  \
  firstIndexOf(ptr) != 0 &&                \
  okSel(mem, firstIndexOf(ptr))            \
)



void *malloc(int size)
  thmprv_pre(int *pre_mem = mem; size >= 0)
  thmprv_post(
    thmprv_exists(
      int address, initVal;
      // the returned pointer is a toplevel address only
      oneLevelPtr(result) &&
      address == firstIndexOf(result) &&
      // pointer points to new object
      !okSel(pre_mem, address) &&
      // of at least 'size' bytes
      okSelOffsetRange(mem, appendIndex(result, 0), size) &&
      // and does not modify anything else
      mem == upd(pre_mem, address, initVal)
    )
  );


struct GT {
  int x;
  int y;

  // invariant: x > y
};
#define TYPE_GT 1
#define FIELD_X 0
#define FIELD_Y 1

struct LT {
  int x;
  int y;

  // invariant: x < y
};
#define TYPE_LT 2
// x and y field indexes from above are ok


// map types into claims about field relationships
#define INVARIANT                       \
  thmprv_forall(int addr;               \
    (typeOf(addr) == TYPE_GT ==>        \
      sel(sel(mem, addr), FIELD_X) >    \
      sel(sel(mem, addr), FIELD_Y)) &&  \
    (typeOf(addr) == TYPE_LT ==>        \
      sel(sel(mem, addr), FIELD_X) <    \
      sel(sel(mem, addr), FIELD_Y))     \
  )


struct GT *makeGT(int x)
  thmprv_pre(int pre_mem=mem; INVARIANT)
  thmprv_post(
    INVARIANT &&
    oneLevelPtr(result) &&
    !okSel(pre_mem, firstIndexOf(result)) &&
    okSelOffsetRange(mem, appendIndex(result, 0), 2) &&
    thmprv_exists(int initVal;
      equalMemory(mem, upd(pre_mem, firstIndexOf(result), initVal))) &&
    result->x == x &&
    typeOf(firstIndexOf(result)) == TYPE_GT
  )
{
  struct GT *ret = (struct GT*)malloc(sizeof(struct GT));
  thmprv_assume(typeOf(firstIndexOf(ret)) == TYPE_GT);
  ret->x = x;
  ret->y = x-1;   // satisfy invariant x>y

  thmprv_assert(equalMemory(mem, upd(pre_mem, firstIndexOf(ret), sel(mem, firstIndexOf(ret)))));
  thmprv_assume(equalMemory(mem, upd(pre_mem, firstIndexOf(ret), sel(mem, firstIndexOf(ret)))));


  return ret;
}


void foo()
  thmprv_pre(INVARIANT)
  thmprv_post(INVARIANT)
{
  struct GT *g1, *g2, *g3;

  thmprv_assert(INVARIANT);

  g1 = (struct GT*)malloc(sizeof(struct GT));
  thmprv_assert(g1 == sub(firstIndexOf(g1), whole));
  thmprv_assume(typeOf(firstIndexOf(g1)) == TYPE_GT);

  g1->x = 5;
  g1->y = 4;
  thmprv_assert(INVARIANT);

  g3 = makeGT(7);
  
  thmprv_assert(g1 != g3);

  g3->y = 3;
  thmprv_assert(typeOf(firstIndexOf(g3)) == TYPE_GT);
  thmprv_assert(INVARIANT);

  g2 = (struct GT*)malloc(sizeof(struct GT));
  thmprv_assume(typeOf(firstIndexOf(g2)) == TYPE_GT);

  g2->x = 5;
  g2->y = 1;

  thmprv_assert(g1 != g2);

//    thmprv_assert(INVARIANT);
  



}



