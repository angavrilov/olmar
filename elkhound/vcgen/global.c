// global.c
// experimenting with global variables

int x;

void addone()
  thmprv_pre 
    int pre_x = x; true;
  thmprv_post
    x == pre_x + 1;
{
  x = x + 1;
}

int main()
{
  x = 2;
  thmprv_assert x == 2;
  
  addone();
  thmprv_assert x == 3;
  thmprv_assert x != 4;

  return 0;
}

