// t0124.cc
// some more tests of pointer-to-member

class A {
public:
  int q(int);
  int qc(int) const;
};


void f()
{
  int (A::*quint)(int);
  int (A::*quintc)(int) const;
  
  quint = &A::q;
  quintc = &A::qc;
}
