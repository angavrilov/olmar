// mlvalue.cc
// code for mlvalue.h

#include "mlvalue.h"     // this module
#include "strutil.h"     // quoted
#include "xassert.h"     // xassert
#include <string.h>      // strrchr


// ============== representation-dependent ===================
// ----------- literals -------------
MLValue mlString(char const *s)
{
  // let's assume C's escape sequences work
  return quoted(s);
}


MLValue mlInt(int i)
{
  return stringc << (unsigned)i;
}


MLValue mlBool(bool b)
{
  return string(b? "true" : "false");
}


// --------------- lists ------------
MLValue mlNil()
{
  return "[]";
}


MLValue mlCons(MLValue hd, MLValue tl)
{
  if (mlIsNil(tl)) {
    return stringc << "[" << hd << "]";
  }
  else {
    // find the leading '[' in 'tl'
    char const *br = strchr(tl, '[');
    xassert(br);
    br++;       // skip it
    return stringc << "[" << hd << "; " << br;
  }
}

bool mlIsNil(MLValue v)
{
  return v.equals("[]");
}


// ------------------ tuples --------------------
MLValue mlTuple0(MLTag tag)
{
  return tag;
}

MLValue mlTuple1(MLTag tag, MLValue v)
{
  return stringc << "(" << tag << " " << v << ")";
}

MLValue mlTuple2(MLTag tag, MLValue v1, MLValue v2)
{
  return stringc << "(" << tag << " "
                 << v1 << " " << v2 << ")";
}

MLValue mlTuple3(MLTag tag, MLValue v1, MLValue v2, MLValue v3)
{
  return stringc << "(" << tag << " "
                 << v1 << " " << v2 << " " << v3 << ")";
}

MLValue mlTuple4(MLTag tag, MLValue v1, MLValue v2, MLValue v3, MLValue v4)
{
  return stringc << "(" << tag << " "
                 << v1 << " " << v2 << " " << v3 << " " << v4 << ")";
}


// ---------------- records ------------------
MLValue mlRecord3(char const *name1, MLValue field1,
                  char const *name2, MLValue field2,
                  char const *name3, MLValue field3)
{
  return stringc << "{ "
                 << name1 << "=" << field1 << "; "
                 << name2 << "=" << field2 << "; "
                 << name3 << "=" << field3 << "; "
                 << "}";
}

MLValue mlRecord4(char const *name1, MLValue field1,
                  char const *name2, MLValue field2,
                  char const *name3, MLValue field3,
                  char const *name4, MLValue field4)
{
  return stringc << "{ "
                 << name1 << "=" << field1 << "; "
                 << name2 << "=" << field2 << "; "
                 << name3 << "=" << field3 << "; "
                 << name4 << "=" << field4 << "; "
                 << "}";
}

MLValue mlRecord7(char const *name1, MLValue field1,
                  char const *name2, MLValue field2,
                  char const *name3, MLValue field3,
                  char const *name4, MLValue field4,
                  char const *name5, MLValue field5,
                  char const *name6, MLValue field6,
                  char const *name7, MLValue field7)
{
  return stringc << "{ "
                 << name1 << "=" << field1 << "; "
                 << name2 << "=" << field2 << "; "
                 << name3 << "=" << field3 << "; "
                 << name4 << "=" << field4 << "; "
                 << name5 << "=" << field5 << "; "
                 << name6 << "=" << field6 << "; "
                 << name7 << "=" << field7 << "; "
                 << "}";
}


// ----------------- references ------------------
MLValue mlRef(MLValue v)
{
  return stringc << "(ref " << v << ")";
}


// ================ representation-independent ==================
DECLARE_ML_TAG(tag_Some, "Some")
DECLARE_ML_TAG(tag_None, "None")

MLValue mlSome(MLValue v)
{
  return mlTuple1(tag_Some, v);
}

MLValue mlNone()
{
  return "None";
}





