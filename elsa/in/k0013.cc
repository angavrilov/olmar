
// error: cannot convert argument type `float (*)[4]' to parameter 1 type
// `float const (*)[4]'

// originally found in package xfree86.

void bar(const float M[4][4]);

void foo() {
    float *m[4];
    bar((float (*)[4]) m);
}
