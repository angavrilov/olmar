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

int offset(int *ptr);
int *object(int *ptr);
int length(int *obj);
thmprv_predicate int/*bool*/ freshObj(int *obj, int *mem);

#define VALID_INTPTR(ptr)        \
  (ptr != (int*)0 &&             \
   offset(ptr) == 0 &&           \
   length(object(ptr)) == 4)

int * owner allocFunc()
  thmprv_pre(int *pre_mem = mem; true)
  thmprv_post(
    // returns a valid pointer
    VALID_INTPTR(result.ptr) &&
    // returns an owner pointer
    result.state == OWNING &&
    // to a new object
    freshObj(object(result), pre_mem) &&
    // and does not modify anything reachable from pre_mem
    pre_mem == mem
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
