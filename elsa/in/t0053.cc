// cc.in53
// problem with names of enums from within context of an inner class

class Foo {
public:
  enum Enum1 { E1_VAL = 3 };

  class Another {
  public:
    enum Enum2 { E2_VAL = 4 };
  };

  class Bar {
  public:
    int f();
  };

};

int Foo::Bar::f()
{
  int x;
 
  x = E1_VAL;      // ok
  //ERROR1: x = E2_VAL;      // can't look into Foo::Another without qualifier
  
  return x;
}
