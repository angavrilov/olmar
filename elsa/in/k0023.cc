// typedef pointer to function in Elsa

// originally seen in xccparse."

template<class T> struct S1 {
    typedef T * (S1<T>::*funcType)();
};
