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

int * owner allocFunc()
  thmprv_post(result.ptr != (int*)0 &&
              offset(result.ptr) == 0 &&
              length(object(result.ptr)) == 4 &&
              result.state == OWNING);

void deallocFunc(int * owner q)
  thmprv_pre(q.state == OWNING);

#endif // OWNER1_H
