// g0008.cc
// test __FUNCTION__ etc. in C++ mode

void f()
{
  char *b = __FUNCTION__;
  char *c = __PRETTY_FUNCTION__;

  //ERROR(1):   char *d = something_else;             // undefined
  //ERROR(3):   char *f = "x" __FUNCTION__;           // can't concat
  //ERROR(4):   char *g = "x" __PRETTY_FUNCTION__;    // can't concat

  //ERROR(5):   char *h = __func__;                   // not in C++
}
