/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * References
 */

int f(int& r) {
  return r;
}

int& g(int& r) {
  return r;
}

int main() {
  int i = 0;

  // variables of reference type are not supported yet
  // int &r = i;

  f(i);

  g(i) = 0;

  return 0;
}
