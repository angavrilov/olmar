// cutpt.c
// simple test of cutpoints

// cutpoint in middle
void foo(int c, int d)
{
  int x,y;

  if (c) {
    x=1;
  }
  else {
    x=2;
  }

  thmprv_invariant x==1 || x==2;

  if (d) {
    y=3;
  }
  else {
    y=4;
  }

  // verify all 4 paths explored
  thmprv_assert
    (x==1 && y==3) ||
    (x==1 && y==4) ||
    (x==2 && y==3) ||
    (x==2 && y==4) ;
}


// no cutpoint
void bar(int c, int d)
{
  int x,y;

  if (c) {
    x=1;
  }
  else {
    x=2;
  }

  if (d) {
    y=3;
  }
  else {
    y=4;
  }
                   
  // verify all 4 paths explored
  thmprv_assert
    (x==1 && y==3) ||
    (x==1 && y==4) ||
    (x==2 && y==3) ||
    (x==2 && y==4) ;
}
