// gb0011.cc
// gcc accepts duplicate parameter names in prototypes

int f(int x, int x);

// it rejects this, at least
//int f(int x, int x) { return 1; }

