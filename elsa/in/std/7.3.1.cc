// 7.3.1.cc

asm("collectLookupResults i 6:7 i 9:9");

namespace Outer {
  int i /*6:7*/;
  namespace Inner {
    void f() { i++; }     // Outer::i
    int i /*9:9*/;
    void g() { i++; }     // Inner::i
  }
}

