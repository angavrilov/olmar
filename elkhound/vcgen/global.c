// global.c
// experimenting with global variables

int x;

void addone()
  thmprv_pre (
    int pre_x = x; true )
  thmprv_post (
    x == pre_x + 1 )
{
  x = x + 1;
}

void bar()
  thmprv_pre (
    int pre_x = x;
    x > 0 )
  thmprv_post (
    x == pre_x )
{}

int main()
{
  x = 2;
  thmprv_assert x == 2;

  addone();
  thmprv_assert x == 3;

  bar();
  thmprv_assert x == 3;

  return 0;
}

