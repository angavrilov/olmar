/* weimer
 * Wed Sep 26 14:09:47 PDT 2001
 *
 * example test for BLAST: can you handle variables
 * with similar names? 
 *
 * ERROR should not be reachable
 */
int bar(int x) {        // 10
    x = x * 2;          // 20
    return x+1;         // return 21
}

int foo(int x) {        // 5
    x = x * 2;          // 10
    x = bar(x);         // bar(10) = 21
    return x+1;         // return 22
}

int main() {
    int x = 5;

    x = foo(x);         // foo(5) = 22

    assert (x == 22);

    return 0;
}
