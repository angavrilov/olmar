// corrcond.c
// simple correlated conditionals test

void foo(int x)
{ 
  int y=0, z=0;

  if (x > 5) {
    y = y+1;
  }
  
  z = 2;
  
  if (x <= 5) {
    z = z + 1;
  }
                                
  // can't take both 'then' branches
  thmprv_assert !(y==1 && z==3);    
  
  // there are only 2 paths
  thmprv_assert (y==1 && z==2) || (y==0 && z==3);
}


