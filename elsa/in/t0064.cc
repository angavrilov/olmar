// cc.in64
// problem with (void) parameter lists

class Foo {
public:
  int f(void);
};

int Foo::f()
{}


// no!
//ERROR1: int g(void x);

// no!
//ERROR2: int h(void, int);

// no!
//ERROR3: int i(void = 4);

