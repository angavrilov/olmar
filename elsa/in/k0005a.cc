// declaring variables and member functions with class name

// originally found in package bombermaze

struct S1 {
    int S1::varName;
};

struct S2 {
    int S2::funcName() {}
};

struct otherS { int funcName(); };

struct S3 {
    //ERROR(1): int otherS::funcName() {}
};

template <typename T>
struct S4 {
    int S4<T>::varName;
};


template <typename T>
struct S5 {
    int S5<T>::funcName() {}
};
