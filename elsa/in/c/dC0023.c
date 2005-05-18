// in gcc, in C mode, a foo and a struct foo have nothing to do with
// one another
typedef char *foo;
struct foo *q;

// the test exhibits the bug without the below, but the below extends
// the test
int main() {
  struct foo *gronk = q;
  foo zork;
  char *bork = zork;
}

