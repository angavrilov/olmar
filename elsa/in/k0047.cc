// template<T> operator T*

// originally found in package djvulibre_3.5.14-2

// k0047.cc:14:36: error: there is no type called `T'

template <class T> struct S1 {
    operator T*();
};

template <class T> S1<T>::operator T*()
{
    return T();
}
