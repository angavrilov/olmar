// fact2.c
// factorial with explicit iteration

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
  
  for (i = n; i > 0; i=i-1) {
    thmprv_invariant(
      factorial(i) * r == factorial(n)
    );
    r = r * i;
  }

  thmprv_assert(i == 0);
  thmprv_assume(i == 0);
  return r;
}

