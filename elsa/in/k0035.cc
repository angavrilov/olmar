// ambiguous int+sizeof(struct{})

// originally found in package buildtool

// Assertion failed: env.disambiguationNestingLevel == 0, file cc_tcheck.cc
// line 1836

// ERR-MATCH: env.disambiguationNestingLevel == 0

int main() {
    int n;
    (n) + (sizeof(struct {}));
}
