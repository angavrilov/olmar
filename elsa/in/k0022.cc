// explicit instantiation of template loses original template parameter names

// error: there is no type called `K1' (inst from a.ii:13:13)

// originally found in package kdelibs

template <class K1> struct S1 {
};

template<class K1> struct S2 {
    typedef S1< K1 > S1;
};

template <class K2> struct S2;

int foo() {
    S2<int> m;
}
