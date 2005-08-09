// t0530.cc
// confusion between operator[] and operator*

struct A {
  operator float *();
  
  // Elsa was getting messed up by the 'const' on 'i', which should
  // not participate in overload resolution
  float operator [] (const int i) const;
};

void foo()
{
  A a;
  a[0];   // operator[] is the right choice
}
