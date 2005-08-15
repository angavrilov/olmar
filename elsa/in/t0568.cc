// t0568.cc
// declaration after use...

// GCC and ICC both accept this.

template <class T>
struct A {
  int capacity;
  void f (int nsz);
};

template <class T>
void A<T>::f(int nsz)
{
  Min(this->capacity, nsz);
}

template void A<int>::f(int nsz);

template <class T>
T Min (const T & x, const T & y);
