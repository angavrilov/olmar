// converting <integral>& to integral

// originally found in package fltk

// error: cannot convert argument type `<integral> &' to parameter 1 type
// `int'

enum {
    FOO = 1
};

void intFunc (int f) {}

int main() {
    int c;

    intFunc(c |= FOO);
}

