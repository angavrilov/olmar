/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * Pointers
 */

int main() {
  int i = 0;
  int *p = 0;

  p = &i;

  i = *p;

  return 0;
}
