// cc.in16
// overloading

int f(int);

int f(double);


class Foo {
public:
  int g(int);
  int g(double);
  //ERROR1: int g(int);
  //ERROR2: char g(int);
};
