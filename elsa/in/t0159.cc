// t0159.cc
// namespace alias

asm("collectLookupResults x=7");

namespace N1 {
  int x;                 // line 7
}

namespace N2 = N1;

int f()
{
  return N2::x;
}
