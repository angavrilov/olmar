// simple.c
// simple demonstration of proof obligations & preconditions

// here's a function which demands its argument be greater than 3
int foo(int x)
//  pre x > 3;
{
  return x-3;
}

// and a function to call it
void bar()
{
  int z = 1;
  
  [[ assert z+3 > 3; ]]

  z = foo(z+3);
}
