// t0127.cc
// some operator overload resolution tests

class A {};

class B {
public:
  void operator+(A &a);             // 8
};

void operator+(A &a, A &b);         // 11
void operator+(B &a, B &b);         // 12

void f()                            // 14
{
  A a1,a2;
  B b1,b2;

  // turn on overload resolution
  __testOverload(f(), 14);

  __testOverload(a1+a2, 11);
  
  __testOverload(b1+b2, 12);

  __testOverload(b1+a1, 8);
}
