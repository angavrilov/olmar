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

//  int *object(int *ptr);
//  int offset(int *ptr);
//  int length(int *obj);
//  int select(int *mem, int *obj, int offset);
//  int update(int *mem, int *obj, int offset, int value);

thmprv_predicate int okSelOffset(int *mem, int *offset);
int updOffset(int *mem, int *offset, int value);
int selOffset(int *mem, int *offset);
int firstIndexOf(int *offset);
int *sub(int *index, int *rest);

void inc(int *x)
  thmprv_pre (
    int *pre_mem = mem;
    int pre_y = y;

    thmprv_exists(
      int *x_object, *x_offset;          // give names to x's toplevel components
      x == sub(x_object, x_offset) &&
      x_object != firstIndexOf(&y)) &&   // at the top level, they differ

    //object(x) != object(&y) &&         // alternate way, not necessary anymore
    x != &y &&
    okSelOffset(mem, x)
  )
  thmprv_post (
    mem == updOffset(pre_mem, x,
             selOffset(pre_mem, x) + 1)  &&
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

