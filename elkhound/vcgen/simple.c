// simple.c
// simple demonstration of proof obligations & preconditions

// here's a function which demands its argument be greater than 3
int foo(int x)
//  pre x > 3;
{
  return x-3;
}

// and a function to call it
void bar(int y)
{
  int z = 1;

  thmprv_assert z+3 > 3 && 1+2;
  thmprv_assert 3;
  thmprv_assert !0;
  thmprv_assert !!(1 < 2);
  thmprv_assert 1 || 0;
  thmprv_assert y>0    ==> y+1>1;
  thmprv_assert !(y>0) ==> y+1<=1;
  thmprv_assert y>0? y+1>1 : y+1<=1;

  z = foo(z+3);
}
