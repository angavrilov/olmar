// I needed a place for random utilities.  I don't mind moving these
// if you want them somewhere else.

#include "dswutil.h"

// Yes, they are exactly the same.

PQName *appendToDeclaratorName(Declarator *d, StringRef suffix) {
  PQName const *pqname = d->getDeclaratorId();
  char const *basename = NULL;
  if (pqname) basename = pqname->getName();
  if (!basename) basename = "__anonymous";
  stringBuilder sb;
  sb << basename << suffix;
  return new PQ_name(sb);
}

PQName *appendToDeclaratorName(IDeclarator *d, StringRef suffix) {
  PQName const *pqname = d->getDeclaratorId();
  char const *basename = NULL;
  if (pqname) basename = pqname->getName();
  if (!basename) basename = "__anonymous";
  stringBuilder sb;
  sb << basename << suffix;
  return new PQ_name(sb);
}
