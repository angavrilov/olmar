// nestwhile.c
// two nested while loops which arose while working on find.c, and
// caused problems with my path counting

void find(int f)
{
  while (f) {
    while (f) {
      thmprv_invariant true;
    }

    thmprv_invariant true;
  }
}


void find2(int f)
{
  while (f) {
    thmprv_invariant
      true ;

    if (f) {
    }
    else if (f) {
    }
  }
}




