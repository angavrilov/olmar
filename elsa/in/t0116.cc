// t0116.c
// experimenting with overloading

void foo(int i)
{
  i;
}

void foo(float f)
{  
  f;
}

void bar()
{
  foo(3);
  foo(3.4);
}

