// 7.3.3e.cc

asm("collectLookupResults f 5:6 g 8:8");

void f();            // 5:6

namespace A {
  void g();          // 8:8
}

namespace X {
  using ::f;         // global f
  using A::g;        // A's g
}

void h()
{
  X::f();            // calls ::f
  X::g();            // calls A::g
}
