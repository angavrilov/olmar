// t0004.c
// non-inner struct

struct Outer {
  struct Inner {     // not really!
    int x;
  };
  enum InnerEnum { InnerEnumerator };
  int y;
  typedef int InnerTypedef;
};

int foo(struct Inner *i)
{
  enum InnerEnum gotcha;
  InnerTypedef z;
  return i->x + InnerEnumerator;
}
