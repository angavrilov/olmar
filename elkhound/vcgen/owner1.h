// owner1.h
// shared declarations for owner-using code; called "owner1.h"
// instead of "owner.h" to avoid conflict with another file I
// use in the implementation..

#ifndef OWNER1_H
#define OWNER1_H

#define NULLOWNER 0
#define DEAD 1
#define OWNING 2

struct OwnerPtrMeta {      // name is special
  // I think I want to model owner ptrs as tuples generally, so
  // here's the pointer part
  int *ptr;

  // when I see <obj>.<field>, I need to connect <field> with some
  // declared field; so this struct will provide the declaration
  // I need
  int state;
};

//  int offset(int *ptr);
//  int *object(int *ptr);
//  int length(int *obj);
thmprv_predicate int/*bool*/ freshObj(int *obj, int *mem);
thmprv_predicate int okSelOffset(int mem, int offset);
thmprv_predicate int okSel(int mem, int index);
int *sub(int index, int *rest);
int upd(int mem, int index, int value);

#define VALID_INTPTR(ptr)        \
  (ptr != (int*)0 &&             \
   okSelOffset(mem, ptr))

int * owner allocFunc()
  thmprv_pre(int *pre_mem = mem; true)
  thmprv_post(
    thmprv_exists(
      int address;
      // the returned pointer is a toplevel address only
      address != 0 &&
      result.ptr == sub(address, 0 /*whole*/) &&
      // returns an owner pointer
      result.state == OWNING &&
      // to a new object
      !okSel(pre_mem, address) &&
      okSel(mem, address) &&
      // and does not modify anything reachable from pre_mem
      thmprv_exists(int initVal; mem == upd(pre_mem, address, initVal))
    )
  );

void deallocFunc(int * owner q)
  thmprv_pre(int *pre_mem = mem; 
             //freshObj(object(q), pre_mem) &&
             q.state == OWNING)
  thmprv_post(// I treat dealloc as no-op, as if I had a garbage
              // collector; this would actually be *sound*, if I
              // uncommented the 'fresh' precondition (!)
              pre_mem == mem);

#endif // OWNER1_H
