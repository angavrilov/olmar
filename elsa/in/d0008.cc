template < class T > struct D:T {};
struct Q {};
template < class T > struct C {
  C (Q &);
  operator D <T> *();
};

struct A {};
struct B {};
int g(A*);
int g(B*);

void f() {
  Q q;
  C <B> r(q);
  g(r);
}
