int f(int q) {return q;}
int main() {
  int $tainted x;
  int $untainted y;
  y = f(x);                        // bad
}
