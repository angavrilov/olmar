// fact2.c
// igarash/london/luckham "factorial procedure"

int selOffset(int mem, int *offset);
thmprv_predicate int okSelOffset(int mem, int *offset);
int updOffset(int mem, int *offset, int value);

// mathematical function
int factorial(int n);


void fact(int *r, int a)
  thmprv_pre(
    int pre_mem = mem;
    okSelOffset(mem, r) &&
    a >= 0
  )
  thmprv_post(
    // changes: *r = factorial(a)
    // mem == pre_mem[r := factorial(a)]
    mem == updOffset(pre_mem, r, factorial(a))
  )
{
  if (a == 0) {
    *r = 1;
  }
  else {
    fact(r, a-1);
    *r = a * *r;
  }
}

int callFact(int n)
  thmprv_pre(
    n >= 0
  )
  thmprv_post(
    result == factorial(n)
  )
{
  int r;
  fact(&r, n);
  return r;
}

