// t0006.c
// implicit return type of int

// ordinary
int f(int x)
{
  return 1;
}

// implicit
g(int x)
{
  return 2;
}

// they are the same
void foo()
{
  __checkType(f, g);
}




