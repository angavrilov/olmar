// swap1.c
// accesses through pointers
// NUMERRORS 1

// function symbols used
//  int *object(int *ptr);
//  int offset(int *ptr);
//  int length(int *obj);
//  int select(int *mem, int *obj, int offset);
//  int update(int *mem, int *obj, int offset, int value);
//int validPointer(int *ptr);

thmprv_predicate int okSelOffset(int mem, int *offset);
int updOffset(int mem, int *offset, int value);
int selOffset(int mem, int *offset);
int firstIndexOf(int *offset);
int *sub(int *index, int *rest);


void foo(int *ptr, int *x)
  thmprv_pre (
    int pre_mem = mem;
    okSelOffset(mem, ptr) && 
    okSelOffset(mem, x) &&
    //offset(ptr) >= 0 && offset(ptr) < length(object(ptr)) &&
    //offset(x) >= 0 && offset(x) < length(object(x)) &&
    firstIndexOf(ptr) != firstIndexOf(x)
  )
  thmprv_post (
    mem == updOffset(updOffset(pre_mem,
             ptr, 5),
             x, 7)
  )
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
