// find.c
// C.A.R. Hoare's "Proof of a Program: FIND", CACM, January 1971

int *object(int *ptr);
int offset(int *ptr);
int length(int *obj);

// input is an array 'A' of 'N' elements, and an integer 'f' in [1,N]
// output is a rearranged A such that the f-th biggest element is in
// position f, all elements smaller than f are to the left of it, and
// all elements larger than f are to the right of it, i.e.:
//   for all p,q: (1 <= p <= f <= q <= N) ==> (A[p] <= A[f] <= A[q])
void find(int *A, int N, int f)
  thmprv_pre(
    offset(A) == 0 && length(object(A)) == N+1 &&   // plus 1 to allow 1-based indexing
    1 <= f && f <= N)
  thmprv_post(
    (thmprv_forall int p, q;
      (1 <= p && p <= f && f <= q && q <= N) ==>
        (A[p] <= A[f] && A[f] <= A[q])))
{
  int m = 1;         // works its way from the left
  int n = N;         // .. from the right
           
  // while there's still disorganized stuff between m and n ..
  while (m < n) {
    // f is between n and m, and both m and n are pivot points
    // (everything left is less than everything right)
    thmprv_invariant
      offset(A) == 0 && length(object(A)) == N+1 &&
      1 <= f && f <= N &&

      1 <= m && m <= f && f <= n && n <= N &&
      (thmprv_forall int p, q;
        ((1 <= p && p < m && m <= q && q <= N) ==> (A[p] <= A[q])) &&
        ((1 <= p && p <= n && n < q && q <= N) ==> (A[p] <= A[q]))) &&

      m < n ;

    int r = A[f];    // approximation of the f-th element
    int i = m;
    int j = n;

    // pivot around the chosen 'r', so everything less is left
    // and everything greater is right; loop until 'i' and 'j'
    // arrive at 'r'
    while (i <= j) {
      // i and j are working inward from m and n, and both i and j
      // are half-pivots: one side of each is all less than r
      thmprv_invariant
        i <= j &&

        offset(A) == 0 && length(object(A)) == N+1 &&
        1 <= f && f <= N &&

        1 <= m && m <= f && f <= n && n <= N &&
        (thmprv_forall int p, q;
          ((1 <= p && p < m && m <= q && q <= N) ==> (A[p] <= A[q])) &&
          ((1 <= p && p <= n && n < q && q <= N) ==> (A[p] <= A[q]))) &&

        m <= i && i < n &&
        m < j && j <= n &&
        (thmprv_forall int p;
          (1 <= p && p < i) ==> (A[p] <= r)) &&
        (thmprv_forall int q;
          (j < q && q <= N) ==> (r <= A[q])) &&

        m < n &&

        //r == A[f] &&
        true;

      while (A[i] < r) {
        thmprv_assume i < n;     // HACK

        thmprv_invariant
          //i < f &&

          offset(A) == 0 && length(object(A)) == N+1 &&
          1 <= f && f <= N &&

          1 <= m && m <= f && f <= n && n <= N &&
          (thmprv_forall int p, q;
            ((1 <= p && p < m && m <= q && q <= N) ==> (A[p] <= A[q])) &&
            ((1 <= p && p <= n && n < q && q <= N) ==> (A[p] <= A[q]))) &&

          m <= i && i < n &&         // same invariant as above..
          m < j && j <= n &&
          (thmprv_forall int p;
            (1 <= p && p < i) ==> (A[p] <= r)) &&
          (thmprv_forall int q;
            (j < q && q <= N) ==> (r <= A[q])) &&

          m < n &&

          //r == A[f] &&
          A[i] < r ;        // loop guard, plus

        i = i+1;    // skip past small-enough elts
      }

      while (r < A[j]) {
        thmprv_assume m < j;        // HACK

        thmprv_invariant
          //f < j &&

          offset(A) == 0 && length(object(A)) == N+1 &&
          1 <= f && f <= N &&

          1 <= m && m <= f && f <= n && n <= N &&
          (thmprv_forall int p, q;
            ((1 <= p && p < m && m <= q && q <= N) ==> (A[p] <= A[q])) &&
            ((1 <= p && p <= n && n < q && q <= N) ==> (A[p] <= A[q]))) &&

          m <= i && i <= n &&         // almost same invariant as above..
          m < j && j <= n &&
          (thmprv_forall int p;
            (1 <= p && p < i) ==> (A[p] <= r)) &&
          (thmprv_forall int q;
            (j < q && q <= N) ==> (r <= A[q])) &&

          m < n &&

          //r == A[f] &&
          r < A[j] ;        // loop guard, plus

        j = j-1;    // skip past large-enough elts
      }

      thmprv_invariant
        offset(A) == 0 && length(object(A)) == N+1 &&
        1 <= f && f <= N &&

        1 <= m && m <= f && f <= n && n <= N &&
        (thmprv_forall int p, q;
          ((1 <= p && p < m && m <= q && q <= N) ==> (A[p] <= A[q])) &&
          ((1 <= p && p <= n && n < q && q <= N) ==> (A[p] <= A[q]))) &&

        m <= i && i <= n &&
        m <= j && j <= n &&
        (thmprv_forall int p;
          (1 <= p && p < i) ==> (A[p] <= r)) &&
        (thmprv_forall int q;
          (j < q && q <= N) ==> (r <= A[q])) &&

        m < n ;

      thmprv_assume A[j] <= r && r <= A[i];     // TEMPORARY HACK
      thmprv_assert A[j] <= r && r <= A[i];

      // if A[j] is left of A[i], swap them
      if (i <= j) {
        int w = A[i];    // swap A[i] and A[j]
        A[i] = A[j];
        A[j] = w;

        // verify that now they're in the right order
        thmprv_assert A[i] <= r && r <= A[j];

        // skip past these two now-properly-ordered elements
        i = i+1;
        j = j-1;
      }
    }

    // at this point, the array has been accurately pivoted around
    // whatever element 'r' was, so we then look to see whether 'r'
    // is to the right or left of 'f'

    if (f <= j) {
      // we pivoted high; everything right of 'j' is in place
      n = j;
    }
    else if (i <= f) {
      // we pivoted low; everything left of 'i' is in place
      m = i;
    }
    else {
      // we pivoted in exactly the right place
      break;
    }
  }
}






