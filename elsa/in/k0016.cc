// error: no template parameter list supplied for `S'

// explicit instantiation of template member function

// originally found in package aspell

// ERR-MATCH: no template parameter list supplied

template <typename T> struct S {
    void foo() {}
};
template void S<int>::foo();
