// testmalloc.cc
// test malloc, test heap walk, etc.
  
#include <stdio.h>     // printf
#include <unistd.h>    // write
#include <string.h>    // strlen
#include <stdlib.h>    // malloc

extern "C" {
  void walk_malloc_heap();
  void malloc_stats();
}

#define MSGOUT(str) write(1, str, strlen(str))

void heapWalk(char const *context)
{
  printf("%s: ", context);
  walk_malloc_heap();
}

#define TRACERET(expr)                      \
  printf("%s returned %p\n", #expr, expr);  \
  heapWalk(#expr);                          \
  malloc_stats()

#define TRACE(expr)                         \
  expr;                                     \
  heapWalk(#expr);                          \
  malloc_stats()

int main()
{
  void *p1, *p2, *p3, *p4, *p5, *p6;

  MSGOUT("Before anything:\n");
  walk_malloc_heap();

  walk_malloc_heap("simple printf");

  TRACERET(p1 = malloc(5));
  TRACERET(p2 = malloc(70));
  TRACERET(p3 = malloc(1000));
  TRACERET(p4 = malloc(10000));
  TRACERET(p5 = malloc(100000));
  TRACERET(p6 = malloc(1000000));

  TRACE(free(p1));
  TRACE(free(p5));
  TRACE(free(p3));
  TRACE(free(p6));
  TRACE(free(p2));
  TRACE(free(p4));

  MSGOUT("testmalloc finished\n");
  return 0;
}

