// t0407.cc
// testing 5.2.5

// icc fails to reject tests: 8
// gcc fails to reject tests: 5 8


struct A {
  struct B {
    struct C {
      int c;
    };

    int b;
    
    void g(int);
    void g(int,int);
  };

  int a;
  ~A();

  typedef int INT;

  void f(int);
  void f(int,int);
};

typedef int INT2;

struct C {
  int d;
};

struct E : A {
};

int x;

void foo()
{
  A a;
  a.a;
  a.A::a;
  a.::A::a;
  //ERROR(1): a.nonexist;    // field does not exist
  //ERROR(2): a.A::B::b;     // wrong class
  //ERROR(3): a.::x;         // not a class member

  A::B b;
  b.b;
  b.B::b;
  b.A::B::b;
  b.::A::B::b;

  A::B::C c;
  c.c;
  //ERROR(4): c.d;           // wrong class
  //ERROR(5): c.C::c;        // 'C' looks up differently
  //ERROR(6): c.B::C::c;     // 'B' not in global, nor in 'C'
  c.A::B::C::c;              // finds 'A' in global
  c.::A::B::C::c;

  a.~A();
  a.A::~A();
  a.::A::~A();
  //ERROR(7): a.::A::~B();

  b.~B();
  b.A::B::~B();
  b.::A::B::~B();

  //ERROR(8): c.~C();        // 'C' looks up differently
  //ERROR(9): a.~INT();      // not a class (found in A)
  //ERROR(10): a.~INT2();    // not a class (found in global)

  //ERROR(11): a.INT::~INT();    // not a class
  //ERROR(12): a.INT2::~INT2();  // not a class
  
  a.f(1);
  a.A::f(1,2);
  a.::A::f(2);
  //ERROR(13): a.::A::f(4,5,6);     // too many args

  b.g(1);
  b.B::g(1,2);
  b.A::B::g(2);
  //ERROR(14): b.A::f(1);           // wrong class
  
  //ERROR(15): a.INT;               // type
  
  E e;
  e.A::f(1);
  
  (a).f(1);
  ((a)).f(1);
  
  A *p;
  p->f(1);
  (p->f)(1);
  (*p).f(1);
  (*(p)).f(1);
  ((*(p))).f(1);
}


// EOF
