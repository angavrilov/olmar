/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * class
 */

class C {
public:  // access modifiers are ignored; everything is assumed to be public
  int i;
  bool b;
  int get_i() { return i; }
  void set_i(int j) { i = j; }
};

int main() {
  C c;

  c.i = 0;
  c.get_i();
  c.set_i(0);

  C d(c);

  d = c;

  return 0;
}
