// if.c
// some vcgen in presence of control flow

void foo()
{
  int x,y;
  if (x) {
    y = 2;
  }
  else {
    thmprv_assert x == 0;
    y = 3;
  }

  thmprv_assert y >= 2;
}


void bar(int x, int y)
{   
  int z = 0;

  if (x) {
    z = z + 1;
  }     
  else {
    z = z + 2;
  }
  
  if (y) {
    z = z + 10;
  }
  else {
    z = z + 20;
  }
  
  thmprv_assert z==11 || z==12 ||
                z==21 || z==22;
}


