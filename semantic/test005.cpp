/* $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * Statements 
 */

int main() {
  int i = 0;

  // if
  if (false)
    i = 1;
  else
    i = 2;

  // switch
  switch (i) {
  case 0:
    i = 3;
    break;
  case 1:
    i = 4;
    break;
  default:
    i = 5;
  }

  // while
  while (i==5) {
    i = 6;
    continue;
    i = 7;
  }

  // do
  do
    i = 8;
  while (false);

  // for
  for (i=9; true; i=10) {
    i = 11;
    break;
  }

  return 0;
}
