// error: more than one ambiguous alternative succeeds

// originally found in package bzip2

struct S{ int x; } *s;

int foo() {
    s->x < 1 || 2 > (3);
}
