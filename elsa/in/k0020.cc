// deleting pointers of template-type

// error: can only delete pointers, not `T'

// originally found in package qt-x11-free

template <class T> struct S1 {
    static void foo() {
        void * x;
        delete (T)x;
    }
};
