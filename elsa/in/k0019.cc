// template parameters named for ctor/dtor

// originally found in package apollon

template <typename T>
struct S1 {
    S1<T>() {}
    ~S1<T>() {}

    //ERROR(1): int blargh<T>() {}
    //ERROR(1): S1<U>() {}
};

template <typename T, typename U>
struct S2 {
    S2<T, U>(T t) {}
    virtual ~S2<T, U>() {}
};
