// bunch.c
// show off a bunch of the abstract interp features
    
int bar(int k)
{
  return k;
}


void foo()
{
  int w, x=1, y=2, z;

  w = z;
  z = 6;

  x += z;
  y = w? x : z;
  x = bar(8);
}


