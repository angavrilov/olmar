// t0228.cc
// test a recent fix to 'computeArraySizeFromLiteral'

void f()
{
  char sName[] = "SOAPPropertyBag";
  char arr[16];

  // should be same type
  __checkType(sName, arr);

  // not the same type!
  char const arr2[16];
  //ERROR(1): __checkType(sName, arr2);
  
  // this is not a legal type
  //ERROR(2): char arr3[];
}
