int main() {
  struct a {
    int a;
    int b;
  };
  int x[sizeof (struct a)];
}
