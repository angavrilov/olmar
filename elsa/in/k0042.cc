// computed array size expression as an assignment

// originally found in package centericq_4.13.0-2

// (expected-token info not available due to nondeterministic mode)
// a.ii:5:15: Parse error (state -1) at =

int main() {
    int i;
    char str[i=4];

    return 0;
}
