// cc.in55
// member initializer where member is a templatized base class
      
template <class T>
class A {
public:
  int f();
  A(T t);
  int g();
};

class B : A<int> {
public:
  B() : A<int>(5) {}
};
