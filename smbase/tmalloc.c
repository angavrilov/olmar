// tmalloc.c
// test malloc implementation in DEBUG_HEAP configuration

#include <stdlib.h>     // malloc
#include <stdio.h>      // printf

#include "mysig.h"      // setHandler
                

typedef void (*Fn)(void);


void expectFail(Fn f)
{
  if (setjmp(sane_state) == 0) {
    setHandler(SIGABRT, jmpHandler);
    setHandler(SIGSEGV, jmpHandler);
    f();
    printf("should have caught that\n");
    exit(2);
  }
  else {
    printf("caught error ok\n");
  }
}


void offEnd()
{
  char *p = malloc(10);
  p[10] = 7;    // oops
  free(p);
}

void offBeg()
{
  char *p = malloc(15);
  p[-1] = 8;
  free(p);
}

void ok()
{
  char *p = malloc(20);
  p[0] = 5;
  p[19] = 9;
  free(p);
}

void dangle()
{
  char **p = malloc(20);
  *p = (char*)(p+4);    // *p is a pointer to 4 bytes later
  **p = 8;              // should work ok
  free(p);
  **p = 7;              // should hit the 0xBB trap
}


int main()
{
  expectFail(offEnd);
  expectFail(offBeg);
  ok();
  expectFail(dangle);
  
  printf("tmalloc works\n");
  return 0;
}
