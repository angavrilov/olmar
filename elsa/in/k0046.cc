// anonymous inline struct fields

// originally found in package elinks_0.9.3-1

// a.i:4:62: error: field `dummy2' is not a class member

int main () {
    int x = (int) &((struct {int dummy1; int dummy2;} *) 0)->dummy2;
}
