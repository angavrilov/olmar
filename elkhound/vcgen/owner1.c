// owner1.c
// experiments with owner/serf
              
#include "owner1.h"     // OwnerPtrMeta, etc.

int main()
{
  int * owner p;
  //p.state = DEAD;    // now automatic
  thmprv_assert(p.state == DEAD);

  p = (int * owner)0;
  //p.ptr = (int *)0;      // now done by assignment as a whole
  //p.state = NULLOWNER;   // now done by assignment as a whole           
  thmprv_assert(p.ptr == (int*)0);
  thmprv_assert(p.state == NULLOWNER);
  thmprv_assert(p == (int * owner)0);

  thmprv_assert(p.state != OWNING);    // otherwise leak
  p = allocFunc();
  //p.ptr = allocFunc();
  //p.state = OWNING;

  // use
  thmprv_assert(p.state == OWNING);
  *p = 6;
  // *(p.ptr) = 6;          // now automatic to use 'ptr'

  thmprv_assert(p.state == OWNING);
  int x = *p;
  //int x = *(p.ptr);       // now automatic to use 'ptr'
  thmprv_assert(x == 6);
  thmprv_assert(p.state == OWNING);

  #if 1
  deallocFunc(p);
  //p.state = DEAD;         // now automatic whenever owner is passed
  thmprv_assert(p.state == DEAD);

  // function return
  thmprv_assert(p.state != OWNING);    // otherwise leak
  #endif // 1

  return 0;
}




