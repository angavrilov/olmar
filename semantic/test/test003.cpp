/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * Function calls
 */

void f() {
  return;
}

int g(int i, bool b) {
  return i;
}

int main() {
  f();
  g(0, true);
  return 0;
}
