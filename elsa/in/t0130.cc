// t0130.cc
// some specific cases where we want to allow 0-length arrays,
// despite cppstd 8.3.4 para 1

// no
//ERROR(1): int arr[0];

// ok
int array[1];

// no
//ERROR(4): int negGlobalArr[-1];

struct F {
  // yes
  int fieldArr[0];

  // no
  //ERROR(3): int negLengthArr[-1];
};

void f()
{
  // no
  //ERROR(2): int localArr[0];
  
  // yes
  int okLocalArr[1];
}



