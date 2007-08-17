
class Z {
public:
  Z(int i) {};
};

int d();
void f();
void g();

class A {
public:
  Z a;

  A() : a(d()) { f(); };
};

class B : public A {
public:
  int b;

  B() : b(0) { g(); };
};

void h () {
  B b;
}

B c;
