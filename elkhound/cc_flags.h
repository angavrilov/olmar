// cc_flags.h
// enumerated flags for parsing C

#ifndef CC_FLAGS_H
#define CC_FLAGS_H

#include "str.h"     // string

// ----------------------- TypeIntr ----------------------
// type introducer keyword
// NOTE: keep consistent with CompoundType::Keyword (cc_type.h)
enum TypeIntr {
  TI_STRUCT,
  TI_CLASS,
  TI_UNION,
  TI_ENUM,
  NUM_TYPEINTRS
};

extern char const * const typeIntrNames[NUM_TYPEINTRS];    // "struct", ...
string toString(TypeIntr tr);


// --------------------- CVFlags ---------------------
// set: which of "const" and/or "volatile" is specified
// I leave the lower 8 bits to represent SimpleTypeId, so I can
// freely OR them together during parsing
enum CVFlags {
  CV_NONE     = 0x000,
  CV_CONST    = 0x100,
  CV_VOLATILE = 0x200,
  CV_OWNER    = 0x400,     // experimental extension
  CV_ALL      = 0x700,
  NUM_CVFLAGS = 3
};

extern char const * const cvFlagNames[NUM_CVFLAGS];      // 0="const", 1="volatile", 2="owner"
string toString(CVFlags cv);


// ----------------------- DeclFlags ----------------------
// set of declaration modifiers present;
// these modifiers apply to variable names
enum DeclFlags {
  DF_NONE        = 0x0000,

  // syntactic declaration modifiers
  DF_INLINE      = 0x0001,
  DF_VIRTUAL     = 0x0002,
  DF_FRIEND      = 0x0004,
  DF_MUTABLE     = 0x0008,
  DF_TYPEDEF     = 0x0010,
  DF_AUTO        = 0x0020,
  DF_REGISTER    = 0x0040,
  DF_STATIC      = 0x0080,
  DF_EXTERN      = 0x0100,
  DF_SOURCEFLAGS = 0x01FF,    // all flags that come from source declarations

  // other stuff that's convenient for me
  DF_ENUMVAL     = 0x0200,    // not really a decl flag, but a Variable flag..
  DF_GLOBAL      = 0x0400,    // set for globals, unset for locals
  DF_INITIALIZED = 0x0800,    // true if has been declared with an initializer (or, for functions, with code)
  DF_BUILTIN     = 0x1000,    // true for e.g. __builtin_constant_p -- don't emit later
  DF_LOGIC       = 0x2000,    // true for logic variables
  DF_ADDRTAKEN   = 0x4000,    // true if it's address has been (or can be) taken

  ALL_DECLFLAGS  = 0x7FFF,
  NUM_DECLFLAGS  = 15         // # bits set to 1 in ALL_DECLFLAGS
};

extern char const * const declFlagNames[NUM_DECLFLAGS];      // 0="inline", 1="virtual", 2="friend", ..
string toString(DeclFlags df);


// ------------------------- SimpleTypeId ----------------------------
// C's built-in scalar types
enum SimpleTypeId {
  ST_CHAR,
  ST_UNSIGNED_CHAR,
  ST_SIGNED_CHAR,
  ST_BOOL,
  ST_INT,
  ST_UNSIGNED_INT,
  ST_LONG_INT,
  ST_UNSIGNED_LONG_INT,
  ST_LONG_LONG,                      // GNU extension
  ST_UNSIGNED_LONG_LONG,             // GNU extension
  ST_SHORT_INT,
  ST_UNSIGNED_SHORT_INT,
  ST_WCHAR_T,
  ST_FLOAT,
  ST_DOUBLE,
  ST_LONG_DOUBLE,
  ST_VOID,
  ST_ELLIPSIS,                       // used to encode vararg functions
  ST_ERROR,                          // this type is returned for typechecking errors
  NUM_SIMPLE_TYPES,
  ST_BITMASK = 0xFF                  // for extraction for OR with CVFlags
};

// info about each simple type
struct SimpleTypeInfo {
  char const *name;       // e.g. "unsigned char"
  int reprSize;           // # of bytes to store
  bool isInteger;         // ST_INT, etc., but not e.g. ST_FLOAT
};

bool isValid(SimpleTypeId id);                          // bounds check
SimpleTypeInfo const &simpleTypeInfo(SimpleTypeId id);

inline char const *simpleTypeName(SimpleTypeId id)
  { return simpleTypeInfo(id).name; }
inline int simpleTypeReprSize(SimpleTypeId id)
  { return simpleTypeInfo(id).reprSize; }
inline string toString(SimpleTypeId id)
  { return string(simpleTypeName(id)); }


// ---------------------------- UnaryOp ---------------------------
enum UnaryOp {
  // side-effecting ops must be first
  UNY_POSTINC,   // ++ (postfix)
  UNY_POSTDEC,   // -- (postfix)
  UNY_PREINC,    // ++
  UNY_PREDEC,    // --

  UNY_SIZEOF,    // sizeof
  UNY_PLUS,      // +
  UNY_MINUS,     // -
  UNY_NOT,       // !
  UNY_BITNOT,    // ~
  NUM_UNARYOPS
};

extern char const * const unaryOpNames[NUM_UNARYOPS];     // "++", ...
string toString(UnaryOp op);
bool hasSideEffect(UnaryOp op);


// ------------------------ BinaryOp --------------------------
enum BinaryOp {
  // the relationals come first, and in this order, to correspond
  // to RelationOp in predicate.ast
  BIN_EQUAL,     // ==
  BIN_NOTEQUAL,  // !=
  BIN_LESS,      // <
  BIN_GREATER,   // >
  BIN_LESSEQ,    // <=
  BIN_GREATEREQ, // >=

  BIN_MULT,      // *
  BIN_DIV,       // /
  BIN_MOD,       // %
  BIN_PLUS,      // +
  BIN_MINUS,     // -
  BIN_LSHIFT,    // <<
  BIN_RSHIFT,    // >>
  BIN_BITAND,    // &
  BIN_BITXOR,    // ^
  BIN_BITOR,     // |
  BIN_AND,       // &&
  BIN_OR,        // ||
  
  // theorem prover extension
  BIN_IMPLIES,   // ==>

  NUM_BINARYOPS
};

extern char const * const binaryOpNames[NUM_BINARYOPS];   // "*", ..
string toString(BinaryOp op);


#endif // CC_FLAGS_H
