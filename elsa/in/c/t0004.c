// t0004.c
// non-inner struct

struct Outer {
  struct Inner {     // not really!
    int x;
  };
  int y;
};

int foo(struct Inner *i)
{  
  return i->x;
}
