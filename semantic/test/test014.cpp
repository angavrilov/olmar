/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * Class with constructors, destructor, member function, implementation given
 * separately
 */

class A {
public:
  A();
  A(const A& a);
  ~A();
  void f();
};

A::A() {};

A::A(const A& a) {};

A::~A() {};

void A::f() {};

int main() {
  A a;
  A b(a);

  a.f();

  return 0;
}
