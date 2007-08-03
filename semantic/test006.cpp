/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * union
 */

union U {
  int i;
  bool b;
  int get_i() { return i; }
  void set_i(int j) { i = j; }
};

int main() {
  U u;

  u.i = 0;
  u.b = true;
  u.get_i();
  u.set_i(0);

  U v(u);

  v = u;

  return 0;
}
