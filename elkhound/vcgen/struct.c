// struct.c
// some simple struct stuff
// NUMERRORS 1

struct S {
  int x;
  int y;
};

struct T {
  int x;
  int y;
  int z;
};

int foo()
{
  struct S s;
  struct T t;

  s.x = 1;
  s.y = 2;
  
  t.x = s.y;       // 2
  t.y = t.x + 5;   // 7
  t.z = 8;

  thmprv_assert s.x == 1;
  thmprv_assert s.y == 2;

  thmprv_assert t.x == 2;
  thmprv_assert t.y == 7;
  thmprv_assert t.z == 8;

  thmprv_assert t.z == s.y + 6;
  thmprv_assert t.z == s.y + 5;    // ERROR(1)
}
