// typedef pointer to member function in template

// originally seen in package aqsis

template<class T> struct S1 {
    typedef void * (S1<T>::*funcType)();
};
