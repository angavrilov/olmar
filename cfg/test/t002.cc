
void g();

namespace N {
  // class M {
  //   int f();
  // };

  namespace M {
    int f() {
      g();
      return 0;
    }

    class A{
    public:
      void a() { g(); };
    };

    class B : public A {
    public:
      static void b() { g(); };
      void c();
    };

  }
    void M::B::c() { g(); };

}

void g()
{
  N::M::f();
  N::M::B::b();
  (new N::M::B)->a();
}
