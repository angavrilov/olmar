// owner2.c
// arrays of owners

#include "owner1.h"     // OwnerPtrMeta, etc.

int main()
{
  int * owner arr[5];     // array of 5 owner pointers
  int i;
  int *tmp;
  int sum;
  int * owner tmpOwner;

  #define ARRAYSTATE(start, end, stateConst) \
    thmprv_forall(int j; start<=j && j<end ==> arr[j].state==stateConst)

  thmprv_invariant(ARRAYSTATE(0, 5, DEAD));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(ARRAYSTATE(0, i, NULLOWNER) && ARRAYSTATE(i, 5, DEAD));
    arr[i] = (int * owner)0;
  }

  thmprv_invariant(ARRAYSTATE(0, 5, NULLOWNER));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(ARRAYSTATE(0, i, OWNING) && ARRAYSTATE(i, 5, NULLOWNER));
    arr[i] = allocFunc();
  }

  thmprv_invariant(ARRAYSTATE(0, 5, OWNING));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(ARRAYSTATE(0, 5, OWNING));
    tmp = arr[i];
    *tmp = i;
  }

  thmprv_invariant(ARRAYSTATE(0, 5, OWNING));

  sum = 0;
  for (i=0; i<5; i=i+1) {
    thmprv_invariant(ARRAYSTATE(0, 5, OWNING));
    tmp = arr[i];
    sum = sum + *tmp;
  }

  thmprv_invariant(ARRAYSTATE(0, 5, OWNING));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(ARRAYSTATE(0, i, DEAD) && 
                     ARRAYSTATE(i, 5, OWNING) &&
                     tmpOwner.state == DEAD);
    tmpOwner = arr[i];
    deallocFunc(tmpOwner);
  }

  thmprv_invariant(ARRAYSTATE(0, 5, DEAD));

  return 0;
}
