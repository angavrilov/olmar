// 7.3.3j.cc

// turn on overloading
int dummy();             // line 4
void ddummy() { __testOverload(dummy(), 4); }

asm("collectLookupResults __testOverload 1:1 dummy 4:5 f 18:8 p 27:11 f 11:16 p 27:11 g 12:8 p 27:11 g 21:8 p 27:11");

struct B {
  virtual void f(int);
  virtual void f(char);  // 11:16
  void g(int);           // 12:8
  void h(int);
};

struct D : B {
  using B::f;
  void f(int);           // OK: D::f(int) overrides B::f(int); (18:8)

  using B::g;
  void g(char);          // OK (21:8)

  using B::h;
  void h(int);           // OK: D::h(int) hides B::h(int)
};

void k(D* p /*27:11*/)
{
  p->f(1);               // calls D::f(int)
  p->f('a');             // calls B::f(char)
  p->g(1);               // calls B::g(int)
  p->g('a');             // calls D::g(char)
}
