// invoking a pointer to member function of a template class

// originally found in:
//   package: apt
//   file: sourcelist.cc

//ERROR: left side of .* must be a class or reference to a class
template <class T> void foo() {
    T t;
    int (T::*bar)();
    t.*bar;
}
