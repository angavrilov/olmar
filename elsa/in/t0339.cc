// t0339.cc
// overload resolution when passing a function

void foo( int (*f)(int) );
void foo( int (*f)(int, int, int, int) );

int g(int, int, int);
int g(int, int);
int g(int);

void bar()
{
  foo(g);        // pass third 'g'
  foo(&g);       // pass third 'g'
}
