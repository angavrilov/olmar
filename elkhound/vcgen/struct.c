// struct.c
// some simple struct stuff

struct S {
  int x;
  int y;
};

int foo()
{
  S s;
  
  s.x = 1;
  s.y = 2;
  
  thmprv_assert(s.x == 1);
  thmprv_assert(s.x == 2);
}
