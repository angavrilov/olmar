// global.c
// experimenting with global variables

int x thmprv_attr(addrtaken);
int y thmprv_attr(addrtaken);

void addone()
  thmprv_pre(
    int pre_x = x; true)
  thmprv_post(
    x == pre_x + 1)
{
  x = x + 1;
}

void bar()
  thmprv_pre(
    int pre_x = x;
    x > 0)
  thmprv_post(
    x == pre_x)
{}

int *object(int *ptr);
int offset(int *ptr);
int length(int *obj);
int select(int *mem, int *obj, int offset);
int update(int *mem, int *obj, int offset, int value);

void inc(int *x)
  thmprv_pre (
    int *pre_mem = mem;
    int pre_y = y;
    //object(x) != object(&y) &&      // alternate way, not necessary anymore
    x != &y &&
    offset(x) >= 0 && offset(x) < length(object(x)) )
  thmprv_post (
    mem == update(pre_mem, object(x), offset(x),
                  select(pre_mem, object(x), offset(x))+1) &&
    y == pre_y )
{
  *x = *x + 1;
}


int main()
{
  x = 2;
  thmprv_assert x == 2;

  addone();
  thmprv_assert x == 3;

  bar();
  thmprv_assert x == 3;

  y = 6;
  thmprv_assert y == 6;

  inc(&x);
  thmprv_assert x == 4;
  thmprv_assert y == 6;

  return 0;
}

