/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * Variable initialization
 */

class A {
public:
  A(int i) {};
};

int main() {
  A a0(0);

  A a1 = A(1);

  return 0;
}
