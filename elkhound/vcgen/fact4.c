// fact4.c
// factorial with explicit iteration, from 1 to n

// mathematical function
int factorial(int n);


int fact(int n)
  thmprv_pre(
    n >= 0
  )
  thmprv_post(
    result == factorial(n)
  )
{
  int r = 1;
  int i;

  for (i = 1; i <= n; i=i+1) {
    r = r * i;
    thmprv_invariant(
      i >= 1 &&
      r == factorial(i)
    );
  }

  return r;
}

