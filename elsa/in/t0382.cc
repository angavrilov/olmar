// t0382.cc
// use a 'using' declaration to name an enum

namespace N {
  enum E { e };
}

void foo()
{
  using namespace N;     // get it all

  enum E e1;

  int x = e;      // 'e' does not come along

  int y = N::e;   // must explicitly qualify
}


void bar()
{
  using N::E;     // get just 'E'

  enum E e1;

  //ERROR(1): int x = e;      // 'e' does not come along

  int y = N::e;   // must explicitly qualify
}
