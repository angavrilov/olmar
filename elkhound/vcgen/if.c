// if.c
// some vcgen in presence of control flow

void foo()
{
  int x,y;
  if (x) {
    y = 2;
  }
  else {
    y = 3;
  }
  
  thmprv_assert y >= 2;
}

