// global3.c
// fn which sets a global..

int x;

void gtzero()
  thmprv_post (
    x > 4
  )
{
  x = 5;
}

int main()
{
  x = 2;
  thmprv_assert x == 2;

  gtzero();
  thmprv_assert x > 3;

  return 0;
}

