//ERROR: multiply defined enum `option'

// originally found in package tcl8.4

void foo()
{
    enum option { a=1 };
    if(1) {
        enum option { a=2 };
    }
}
