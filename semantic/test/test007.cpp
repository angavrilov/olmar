/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * struct
 */

struct S {
  int i;
  bool b;
  int get_i() { return i; }
  void set_i(int j) { i = j; }
};

int main() {
  S s;

  s.i = 0;
  s.b = true;
  s.get_i();
  s.set_i(0);

  S t(s);

  t = s;

  return 0;
}
