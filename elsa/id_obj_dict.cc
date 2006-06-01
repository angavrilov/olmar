// quarl 2006-05-30

#include "id_obj_dict.h"

// parse an unsigned decimal integer from a string; returns true iff string is
// an integer only.
inline
bool atoiFull(char const *p, unsigned &result)
{
  result = 0;

  while (*p) {
    if (!isdigit(*p)) return false;
    result = result * 10 + (*p - '0');
    ++p;
  }
  return true;
}

// static
bool IdObjDict::parseId(char const *id,
                        char prefix[3],
                        unsigned &idnum)
{
  if (!isupper(id[0]) || !isupper(id[1]))
    return false;

  if (!atoiFull(id+2, idnum)) return false;

  prefix[0] = id[0];
  prefix[1] = id[1];
  prefix[2] = '\0';

  return true;
}

void *IdObjDict::queryif(char const *id)
{
  char prefix[3];
  unsigned idnum;

  if (parseId(id, prefix, idnum)) {
    ObjIdArray * a = objectsById.queryif(prefix);
    if (!a) return false;
    if (idnum+1 > (unsigned) a->size()) {
      return false;
    }
    return (*a)[idnum];
  } else {
    return objectsOther.queryif(id);
  }
}

void IdObjDict::add(char const *id, void *obj)
{
  char prefix[3];
  unsigned idnum;

  if (parseId(id, prefix, idnum)) {
    ObjIdArray * a = objectsById.queryif(prefix);
    if (!a) {
      a = new ObjIdArray(max((unsigned)64, 2*idnum));
      memset(a->getArrayNC(), 0, sizeof(void*) * a->size());
      objectsById.add(prefix, a);
    } else {
      int oldSize = a->size();
      a->ensureIndexDoubler(idnum+1);
      memset(a->getArrayNC()+oldSize, 0, sizeof(void*) * (a->size() - oldSize));
    }
    (*a)[idnum] = obj;
  } else {
    objectsOther.add(id, obj);
  }
}

