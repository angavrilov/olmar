// t0159.cc
// namespace alias

asm("collectLookupResults N2::x 7:7");

namespace N1 {
  int x /*7:7*/;
}

namespace N2 = N1;

int f()
{
  return N2::x;
}
