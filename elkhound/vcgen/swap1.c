// swap1.c
// accesses through pointers
// NUMERRORS 1

// function symbols used
int *object(int *ptr);
int offset(int *ptr);
int length(int *obj);
int select(int *mem, int *obj, int offset);
int update(int *mem, int *obj, int offset, int value);
//int validPointer(int *ptr);

void foo(int *ptr, int *x)
  thmprv_pre
    int *pre_mem = mem;
    //validPointer(ptr) && validPointer(x) &&
    offset(ptr) >= 0 && offset(ptr) < length(object(ptr)) &&
    offset(x) >= 0 && offset(x) < length(object(x)) &&
    ptr != x;
  thmprv_post
    mem == update(update(pre_mem, 
             object(ptr), offset(ptr), 5),
             object(x), offset(x), 7);
{
  *ptr = 5;
  *x = 7;
  return;
}

int main() 
{
  int x, y;

  int final_x, final_y;

  x = 55;
  y = 77;

  foo(&x,&y);

  final_x = x;
  final_y = y;

  thmprv_assert(final_x == 5);
  thmprv_assert(final_x == 7);    // ERROR(1)

  return 0;
}
