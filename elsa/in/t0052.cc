// cc.in52
// variant of 3.4.5.cc

struct A {
  int a;
};
struct B : /*virtual*/ A {};
struct C : B {};
struct D : B {};
struct E : public C, public D {};
struct F : public A{};

void f()
{
  E e;
  //ERROR(1): e.B::a = 0;     // ambiguous
  //e.C::B::a = 0;            // ok (err.. gcc doesn't like it.. should I?)
  // f it ...

  F f;
  f.A::a = 1;        // OK, A::a is a member of F
}
