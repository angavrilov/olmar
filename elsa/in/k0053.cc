// calling a function with a pointer to an undefined template class

// originally found in package ace_5.4.2.1-1

// a.ii:9:9: error: attempt to instantiate `::S1<int>', but no definition has
// been provided for `::S1<T>'

// ERR-MATCH: attempt to instantiate `.*?', but no definition has been provided

template <typename T> struct S1;

void foo(S1<int> *);

int main() {
    S1<int>* ptr;
    foo(ptr);
    return 0;
}
