// isort.c
// "interchange sort" (insertion sort) from igarashi/london/luckham
// NOTE: as yet unverified by my system

/* 
  This example, taken from [10], sorts by successively finding the largest
  element of the array A.  The assertions include provision for showing that
  the array A at the exit is a permutation of the array A at the entry.  The
  entry array is denoted by the array name A0.  The assertions contain two
  definitions.  sameSet(A, A0, A[arbitrary]) denotes that A and A0 are the
  same set of elements including repetition.  The term A[arbitrary] is a
  trick to allow VCG to check that an array is unaltered over a path between
  assertions.  The truck is needed because array substitution is done by
  array element, not by array name.  The second definition is for 
  multiSet(A, A0, J, K, L, M) where K and M denote array elements of A, and
  J and L denote subscripts of A.  multiSet denotes that A and A0 are the
  same set of elements including repetition even if A[J] := K and A[L] := M
  are simultaneously done.  Thus, e.g.,
    multiSet(A, A0, J, A[J], LOC, A[LOC])
  and
    multiSet(A, A0, J, A[LOC], LOC, A[J])
  both are true, but
    multiSet(A, A0, J, A[J], J+1, A[J])
  is not true generally.
  
  This asserted program and resulting verification conditions were the
  initial input to the Allen-Luckham theorem prover when it was able to
  discover the verification condition which could not be proved.
*/


void isort(int n,       // array size
           int *A,      // array being sorted
           int *A0)     // original array contents
  thmprv_pre(
    n >= 1 &&
    sameSet(A, A0)
  )
  thmprv_post(
    thmprv_forall(int k;
      (1 <= k && k <= n-1  ==>  A[k] <= A[k+1]) &&
      sameSet(A, A0)
    )
  )
{
  int i;
  int j = n;
  int loc;      // intially undefined
  int big;

  while (j >= 2) {
    thmprv_invariant(     // original code has this a before-guard invariant
      thmprv_forall(int k; j+1 <= k && k <= n-1  ==>  A[k] <= A[k+1]) &&
      thmprv_forall(int m; 1 <= m && m <= j && j <= n-1  ==>  A[m] <= A[j+1]) &&
      1 <= j && j <= n &&
      multiSet(A, A0, j+1, A[j+1], loc, A[loc])
    );

    big = A[1];
    loc = 1;
    i = 2;

    while (i <= j) {
      thmprv_invariant(    // also originally a pre-guard invariant
        // these duplicated from above
        thmprv_forall(int k; j+1 <= k && k <= n-1  ==>  A[k] <= A[k+1]) &&
        thmprv_forall(int m; 1 <= m && m <= j && j <= n-1  ==>  A[m] <= A[j+1]) &&
        // these are new
        thmprv_forall(int l; i <= l && l <= i-1 && i-1 <= n  ==>  A[l] <= big) &&
        big == A[loc] &&
        1 <= loc && loc <= j &&
        i >= 2 &&
        2 <= j && j <= n &&
        sameSet(A, A0)
      )
      
      if (A[i] > big) {
        big = A[i];
        loc = i;
      }
      i = i+1;
    }
    
    A[loc] = A[j];
    A[j] = big;
    j = j-1;
  }
}




