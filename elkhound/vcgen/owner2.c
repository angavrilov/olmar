// owner2.c
// arrays of owners

#include "owner1.h"     // OwnerPtrMeta, etc.

int main()
{
  int * owner arr[5];     // array of 5 owner pointers
  int i;
  int *tmp;
  int sum;

  #define ARRAYPREDJ(start, end, pred) \
    thmprv_forall(int j; start<=j && j<end ==> (pred))

  #define ARRAYSTATE(start, end, stateConst) \
    ARRAYPREDJ(start, end, arr[j].state==stateConst)

  thmprv_invariant(ARRAYSTATE(0, 5, DEAD));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(i>=0 && i<5 &&
                     ARRAYSTATE(0, i, NULLOWNER) &&
                     ARRAYSTATE(i, 5, DEAD));
    arr[i] = (int * owner)0;
  }

  thmprv_invariant(ARRAYSTATE(0, 5, NULLOWNER));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(i>=0 && i<5 &&
                     ARRAYSTATE(0, i, OWNING) &&
                     ARRAYPREDJ(0, i, VALID_INTPTR(arr[j].ptr)) &&
                     ARRAYSTATE(i, 5, NULLOWNER));
    arr[i] = allocFunc();
  }

  thmprv_invariant(ARRAYSTATE(0, 5, OWNING) &&
                   ARRAYPREDJ(0, 5, VALID_INTPTR(arr[j].ptr)));

  for (i=0; i<5; i=i+1) {
    thmprv_invariant(i>=0 && i<5 &&
                     ARRAYPREDJ(0, 5, VALID_INTPTR(arr[j].ptr)) &&
                     ARRAYSTATE(0, 5, OWNING));
    tmp = (int*)(arr[i]);      // cast owner to nonowner
    *tmp = i;
  }

  thmprv_invariant(ARRAYSTATE(0, 5, OWNING) &&
                   ARRAYPREDJ(0, 5, VALID_INTPTR(arr[j].ptr)));

  sum = 0;
  for (i=0; i<5; i=i+1) {
    thmprv_invariant(i>=0 && i<5 &&
                     ARRAYPREDJ(0, 5, VALID_INTPTR(arr[j].ptr)) &&
                     ARRAYSTATE(0, 5, OWNING));
    tmp = (int*)arr[i];        // cast owner to nonowner
    sum = sum + *tmp;
  }

  thmprv_invariant(ARRAYSTATE(0, 5, OWNING) &&
                   ARRAYPREDJ(0, 5, VALID_INTPTR(arr[j].ptr)));

  int * owner tmpOwner;
  for (i=0; i<5; i=i+1) {
    thmprv_invariant(i>=0 && i<5 &&
                     ARRAYSTATE(0, i, DEAD) &&
                     ARRAYSTATE(i, 5, OWNING) &&
                     ARRAYPREDJ(i, 5, VALID_INTPTR(arr[j].ptr)) &&
                     tmpOwner.state == DEAD);
    tmpOwner = arr[i];
    deallocFunc(tmpOwner);
  }

  thmprv_invariant(ARRAYSTATE(0, 5, DEAD) &&
                   tmpOwner.state == DEAD);

  return 0;
}
