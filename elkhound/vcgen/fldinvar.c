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


void *malloc(int size)
  thmprv_pre(int *pre_mem = mem; size >= 0)
  thmprv_post(
    thmprv_exists(
      int address, initVal;
      // the returned pointer is a toplevel address only
      address != 0 &&
      result == sub(address, 0 /*whole*/) &&
      // pointer points to new object
      !okSel(pre_mem, address) &&
      okSel(mem, address) &&
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

#define INVARIANT                                              \
  thmprv_forall(int addr; typeOf(addr) == TYPE_GT ==>          \
                            sel(sel(mem, addr), FIELD_X) >     \
                            sel(sel(mem, addr), FIELD_Y))


void foo()
  thmprv_pre(INVARIANT)
  thmprv_post(INVARIANT)
{
  struct GT *g1, *g2;

  thmprv_assert(INVARIANT);

  g1 = (struct GT*)malloc(sizeof(struct GT));
  thmprv_assert(g1 == sub(firstIndexOf(g1), whole));
  thmprv_assume(typeOf(firstIndexOf(g1)) == TYPE_GT);

  g1->x = 5;
  g1->y = 4;
  thmprv_assert(INVARIANT);

  g2 = (struct GT*)malloc(sizeof(struct GT));
  thmprv_assume(typeOf(firstIndexOf(g2)) == TYPE_GT);

  g2->x = 5;
  g2->y = 1;
  
  thmprv_assert(g1 != g2);

//    thmprv_assert(INVARIANT);
  



}



