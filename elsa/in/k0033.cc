// casting function pointer with throw() to/from no throw

// originally found in package abiword

// error: cannot convert argument type `int (*&)() throw()' to parameter 1
// type `int (*)()'

// ERR-MATCH: cannot convert argument type `.*throw\(\)'

int bar1 (int (*func) ());
int bar2 (int (*func) () throw());

int foo1 ();
int foo2 () throw ();

int main() {
    // bar(foo);

    int (*func1) () throw();

    int (*func2) ();

    func1 = func2;
    func2 = func1;

    bar1(func1);
    bar1(func2);

    bar2(func1);
    bar2(func2);

    bar1(foo1);
    bar1(foo2);

    bar2(foo1);
    bar2(foo2);
}
