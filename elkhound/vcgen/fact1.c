// fact1.c
// igarashi/london/luckham "factorial as a function"
  
// this is the mathematical function; it is defined (recursively) in
// bgpred.sx, for now
int factorial(int n);


int fact(int n)
  thmprv_pre(
    n >= 0
  )
  thmprv_post(
    result == factorial(n)
  )
{
  if (n == 0) {
    return 1;
  }
  else {
    return n * fact(n-1);
  }
}

