// static inline function implicitly returning int

// originally found in package framerd_2.4.1-1.1

// In state 954, I expected one of these tokens:
//   (, =, ,, ;,
// a.i:4:1: Parse error (state 954) at {

static inline foo()
{
    return 0;
}

int main()
{
    return foo();
}
