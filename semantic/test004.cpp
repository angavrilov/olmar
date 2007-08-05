/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * Variable declarations
 */

// global vars are not allowed
// int i = 0;

int main() {
  // vars MUST be initialized
  // int i;

  int i = 0;
  int j = i;  // FIXME: l2r conversion required

  bool b = false;

  {
    // our scope rules are slightly different from those of C++,
    // so that this declaration would not work as expected --
    // it's best to use unique identifiers everywhere!
    // int i = i;
    int i = 1;
    j = i;
  }

  return 0;
}
