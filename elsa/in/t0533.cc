// t0533.cc
// double-typed expr

void f(double,double);     // line 4: this is the right one
void f(int,int);

void foo()
{
  double d;
  f(2*d+4, 3.0);           // calls 'f' on line 4
  f(4+2*d, 3.0);           // calls 'f' on line 4
}
