// memswap.c
// a small piece of the find.c code

int *object(int *ptr);
int offset(int *ptr);
int length(int *obj);

void swap(int *A, int N, int i, int j, int r)
  thmprv_pre (
    offset(A) == 0 && length(object(A)) == N+1 &&

    1 <= i && i <= N &&
    1 <= j && j <= N &&

    A[j] <= r && r <= A[i]
  )

  thmprv_post (
    A[i] <= r && r <= A[j] 
  )
{
  int w = A[i];    // swap A[i] and A[j]
  A[i] = A[j];
  A[j] = w;
}
