// error: more than one ambiguous alternative succeeds

struct S{ int x; } *s;

int foo() {
    s->x < 1 || 2 > (3);
}
