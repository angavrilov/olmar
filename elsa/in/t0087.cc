// t0087.cc
// experimenting with ptr-to-member

class A {
public:
  int x;
};
class B {};

class C : public A {};
class D : public C, public A {};

class E : virtual public A {};
class F : public E, virtual public A {};
class G : public E, public A {};

void foo()
{
  int A::*p = &A::x;

  // experiments with .*
  A a;
  B b;
  C c;
  D d;
  E e;
  F f;
  G g;

  a.*p = 7;       // ok, obviously
  //b.*p = 7;     // ERROR(1): not derived
  c.*p = 7;       // ok, A is a base of C
  //d.*p = 7;     // ERROR(2): ambiguous derivation
  e.*p = 7;       // ok, A is a virtual base of E
  f.*p = 7;       // ok, A is a virtual base twice (E and F)
  //g.*p = 7;     // ERROR(3): ambiguous derivation

  // same things with ->*
  A *ap;
  B *bp;
  C *cp;
  D *dp;
  E *ep;
  F *fp;
  G *gp;

  ap->*p = 7;       // ok, obviously
  //bp->*p = 7;     // ERROR(1): not derived
  cp->*p = 7;       // ok, A is a base of C
  //dp->*p = 7;     // ERROR(2): ambiguous derivation
  ep->*p = 7;       // ok, A is a virtual base of E
  fp->*p = 7;       // ok, A is a virtual base twice (E and F)
  //gp->*p = 7;     // ERROR(3): ambiguous derivation
}


