/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * enum
 */

enum En {
  A,
  B,
  C = 0,
  D,
  E = 42,
  F
};

int main() {
  En e = A;

  int i = e;

  return 0;
}
