// 7.3.3g.cc

// turn on overloading
int dummy();             // line 4
void ddummy() { __testOverload(dummy(), 4); }

asm("collectLookupResults __testOverload 1:1 dummy 4:5 f 10:8 f 16:8");

namespace A {
  void f(int);          // 10:8
}

using A::f;             // f is a synonym for A::f;
                        // that is, for A::f(int).
namespace A {
  void f(char);         // 16:8
}

void foo()
{
  f('a');               // calls f(int),
}                       // even though f(char) exists.

void bar()
{
  using A::f;           // f is a synonym for A::f;
                        // that is, for A::f(int) and A::f(char).
  f('a');               // calls f(char)
}
