// cc_flags.cc
// code for cc_flags.h

#include "cc_flags.h"     // this module
#include "macros.h"       // STATIC_ASSERT
#include "xassert.h"      // xassert
#include "trace.h"        // tracingSys


// -------------------- TypeIntr -------------------------
char const * const typeIntrNames[NUM_TYPEINTRS] = {
  "struct",
  "class",
  "union",
  "enum"
};

#define MAKE_TOSTRING(T, limit, array)        \
  string toString(T index)                    \
  {                                           \
    xassert((unsigned)index < limit);         \
    return string(array[index]);              \
  }

MAKE_TOSTRING(TypeIntr, NUM_TYPEINTRS, typeIntrNames)


// ---------------- CVFlags -------------
#ifdef MLVALUE
MAKE_ML_TAG(attribute, 0, AId)
MAKE_ML_TAG(attribute, 1, ACons)

MLValue cvToMLAttrs(CVFlags cv)
{
  // AId of string

  MLValue list = mlNil();
  if (cv & CV_CONST) {
    list = mlCons(mlTuple1(attribute_AId, mlString("const")), list);
  }
  if (cv & CV_VOLATILE) {
    list = mlCons(mlTuple1(attribute_AId, mlString("volatile")), list);
  }
  if (cv & CV_OWNER) {
    list = mlCons(mlTuple1(attribute_AId, mlString("owner")), list);
  }
  return list;
}
#endif // MLVALUE


char const * const cvFlagNames[NUM_CVFLAGS] = {
  "const",
  "volatile",
  "owner"
};


string bitmapString(int bitmap, char const * const *names, int numflags)
{
  stringBuilder sb;
  int count=0;
  for (int i=0; i<numflags; i++) {
    if (bitmap & (1 << i)) {
      if (count++) {
        sb << " ";
      }
      sb << names[i];
    }
  }

  return sb;
}

string toString(CVFlags cv)
{
  return bitmapString(cv, cvFlagNames, NUM_CVFLAGS);
}


// ------------------- DeclFlags --------------
char const * const declFlagNames[NUM_DECLFLAGS] = {
  "inline",       // 0
  "virtual",
  "friend",
  "mutable",
  "typedef",      // 4
  "auto",
  "register",
  "static",
  "extern",
  "enumval",      // 9
  "global",
  "initialized",
  "builtin",
  "thmprv",       // 13
};


string toString(DeclFlags df)
{
  return bitmapString(df, declFlagNames, NUM_DECLFLAGS);
}


// ---------------------- SimpleTypeId --------------------------
bool isValid(SimpleTypeId id)
{
  return 0 <= id && id <= NUM_SIMPLE_TYPES;
}


static SimpleTypeInfo const simpleTypeInfoArray[] = {
  //name                   size  int?
  { "char",                1,    true    },
  { "unsigned char",       1,    true    },
  { "signed char",         1,    true    },
  { "bool",                4,    true    },
  { "int",                 4,    true    },
  { "unsigned int",        4,    true    },
  { "long int",            4,    true    },
  { "unsigned long int",   4,    true    },
  { "long long",           8,    true    },
  { "unsigned long long",  8,    true    },
  { "short int",           2,    true    },
  { "unsigned short int",  2,    true    },
  { "wchar_t",             2,    true    },
  { "float",               4,    false   },
  { "double",              8,    false   },
  { "long double",         10,   false   },

  // gnu: sizeof(void) is 1
  { "void",                1,    false   },
  { "...",                 0,    false   },
  { "<error>",             0,    false   },
};

SimpleTypeInfo const &simpleTypeInfo(SimpleTypeId id)
{
  STATIC_ASSERT(TABLESIZE(simpleTypeInfoArray) == NUM_SIMPLE_TYPES);
  xassert(isValid(id));
  return simpleTypeInfoArray[id];
}


// ------------------------ UnaryOp -----------------------------
char const * const unaryOpNames[NUM_UNARYOPS] = {
  "++/*postfix*/",
  "--/*postfix*/",
  "++/*prefix*/",
  "--/*prefix*/",
  "sizeof",
  "+",
  "-",
  "!",
  "~"
};

MAKE_TOSTRING(UnaryOp, NUM_UNARYOPS, unaryOpNames)

bool hasSideEffect(UnaryOp op)
{
  return op <= UNY_PREDEC;
}


// ---------------------- BinaryOp -------------------------
char const * const binaryOpNames[NUM_BINARYOPS] = {
  "==",
  "!=",
  "<",
  ">",
  "<=",
  ">=",

  "*",
  "/",
  "%",
  "+",
  "-",
  "<<",
  ">>",
  "&",
  "^",
  "|",
  "&&",
  "||",
  
  "==>"
};

MAKE_TOSTRING(BinaryOp, NUM_BINARYOPS, binaryOpNames)
