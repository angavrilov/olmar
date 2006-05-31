
// quarl 2006-05-30
//    IdObjDict is a data structure with the same interface as
//    StringSObjDict<void> (actually subset), but optimized for the case where
//    most (or all) ids are of the form "XY1234" (with the integer IDs densely
//    increasing).
//
//    It is implemented via a map from strings (2-character prefixes) to
//    arrays, where the integer ID is used directly as the array index.
//
//    This optimization reduces deserialization time by 60% and memory usage
//    by 16%.
//
//    Other IDs are supported (using the old hashtable algorithm), but when
//    parsing output produced by ourselves, we should never see IDs not of the
//    form "XY1234".

#ifndef IDOBJDICT_H
#define IDOBJDICT_H

#include "strobjdict.h"         // StringObjDict
#include "strsobjdict.h"        // StringSObjDict
#include "array.h"              // GrowArray

class IdObjDict {
  typedef GrowArray<void *> ObjIdArray;
  StringObjDict<ObjIdArray> objectsById;
  StringSObjDict<void> objectsOther;

public:
  void *queryif(char const *id);
  void *queryif(string const &id) { return queryif(id.c_str()); }
  void add(char const *id, void *obj);
  void add(string const &id, void *obj) { add(id.c_str(), obj); }
  bool isMapped(char const *id) { return queryif(id) != NULL; }

protected:
  static bool parseId(char const *id, char prefix[3], unsigned &idnum);
};

#endif
