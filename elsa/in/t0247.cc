// t0247.cc
// from oink/test/template_func2.cc

// function template
template<int I, class T>
T f(T x)
{
  return sizeof(T) + I;
}

// specialization
template<>
int f<2, int>(int x)
{
  return x;
}

int main()
{
  int y;
  int z = f<1, int>(y);     // use primary
  
  f<2, int>(y);             // use specialziation
}
