// cc.in13
// inline member functions

class Foo {
public:
  int func()
  {
    //ERROR1: return y;
    return x;
  }

  //ERROR3: int func();
  
  //ERROR4: int func() { return 5; }

  int bar();
  //ERROR2: int bar();

  int x;
};

int main()
{
  Foo x;
  x.func();
}
