// t0324.cc
// isolated problem with nsCSSFrameConstructor.i

template <class T>
class nsDerivedSafe : public T {
};

class nsIStyleContext {};

template <class T>
void
GetStyleData(nsIStyleContext* aStyleContext, const T** aStyleStruct);

template <class T>
void
GetStyleData(int, int);

struct A {};

void foo()
{
  const A *a;
  nsDerivedSafe<nsIStyleContext> *ptr = 0;

  GetStyleData(ptr, &a);
}

// EOF
