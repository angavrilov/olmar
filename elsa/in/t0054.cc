// cc.in54
// explicit template class specialization

// primary template
template <class T>
class A {
  T f();
};

// explicit specialization
template <>
class A<char> {
  // it's not the case that specializations must declare the same
  // member functions as the primary template, but I'm going to
  // assume that they do anyway, since I do not want to implement
  // specialization matching
  char f() { return 'f'; }
};

int main()
{
  A<char> a;
  
  // since specialization matching is not implemented, this call to 'f'
  // will actually be looked up in the primary template class
  a.f();

  return 0;
}
