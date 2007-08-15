/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * User-Defined Conversions
 */

class A {
private:
  A() {};
public:
  A(int i) {};
  operator int() {};
};

int main() {
  // 12.3.1: Conversion by constructor
  A a = 0;

  // 12.3.2: Conversion functions
  int i = a;

  return 0;
}
