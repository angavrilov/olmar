// CoderInfo-ootm.ii:8:22: error: variable name `D</*ref*/ I0>::P' used as if it were a type

template <int I>
class D {
  union P {};
  static P *L[N];
};

template<int I0> 
D<I0>::P *D<I0>::L[N] = {0, 0};
