// inherited constructors

void f(void);
void g(void);

class A{
  virtual ~A() { f(); }
};


class B : public A {
  virtual ~B() { g(); }
};
