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
    thmprv_forall(int p, q;
      (1 <= p && p <= f && f <= q && q <= N) ==>
        (A[p] <= A[f] && A[f] <= A[q])))
;

int printf(char const *fmt, ...);

int main()
{
  int arr[11] = { 0,    // index 1 to 10
    5, 8, 1, 45, 3, 7, 9, 2, 8, 12 };
  int i;

  find(arr, 10, 6);

  printf("result:");
  for (i=1; i<=10; i=i+1) {
    thmprv_invariant i >= 1;
    printf(" %d", arr[i]);
  }
  printf("\n");

  return 0;
}







