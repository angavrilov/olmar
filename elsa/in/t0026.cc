// cc.in26
// template functions

template <class T>
// dsw: it would help to compile these examples before using them as tests.
//  t0026.cc:19: could not convert `9' to `int&'
//  int f(T &t)
int f(T t)
{
  T(z);    // ambiguous, but I can disambiguate!

  int q = T(z);    // ctor call

  // this error is correct:
  // non-compound `int' doesn't have fields to access
//    int y = t.x;
}


int main()
{
  f(9);

  f<int>(8);
    
  // I get the wrong answer for this one..
  //f<int()>(8);
}
