// dsw: check pointer arithmetic
int main() {
  int *p, q;
  int *r = p + 1;
  int *s = 1 + p;
  int i = p - q;
}
