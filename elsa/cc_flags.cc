// cc_flags.cc            see license.txt for copyright and terms of use
// code for cc_flags.h

#include "cc_flags.h"     // this module
#include "macros.h"       // STATIC_ASSERT
#include "xassert.h"      // xassert
#include "trace.h"        // tracingSys
#include "strtokp.h"      // StrtokParse
#include "strutil.h"      // DelimStr


// the check for array[limit-1] is meant to ensure that there
// are as many specified entries as there are total entries
#define MAKE_TOSTRING(T, limit, array)        \
  char const *toString(T index)               \
  {                                           \
    xassert((unsigned)index < limit);         \
    xassert(array[limit-1] != NULL);          \
    return array[index];                      \
  }

#define PRINTENUM(X) case X: return #X
#define READENUM(X) else if (streq(str, #X)) out = (X)

#define PRINTFLAG(X) if (id & (X)) b << #X
#define READFLAG(X) else if (streq(token, #X)) out |= (X)

// -------------------- TypeIntr -------------------------
char const * const typeIntrNames[NUM_TYPEINTRS] = {
  "struct",
  "class",
  "union",
  "enum"
};

MAKE_TOSTRING(TypeIntr, NUM_TYPEINTRS, typeIntrNames)

char const *toXml(TypeIntr id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(TI_STRUCT);
  PRINTENUM(TI_CLASS);
  PRINTENUM(TI_UNION);
  PRINTENUM(TI_ENUM);
  }
}
void fromXml(TypeIntr &out, rostring str) {
  if(0) xfailure("?");
  READENUM(TI_STRUCT);
  READENUM(TI_CLASS);
  READENUM(TI_UNION);
  READENUM(TI_ENUM);
  else xfailure("bad enum string");
}


// ---------------- CVFlags -------------
char const * const cvFlagNames[NUM_CVFLAGS] = {
  "const",
  "volatile",
  "restrict",
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
  return bitmapString(cv >> CV_SHIFT_AMOUNT, cvFlagNames, NUM_CVFLAGS);
}

string toXml(CVFlags id) {
  if (id == CV_NONE) return "CV_NONE";
  DelimStr b('|');
  PRINTFLAG(CV_CONST);
  PRINTFLAG(CV_VOLATILE);
  PRINTFLAG(CV_RESTRICT);
  PRINTFLAG(CV_OWNER);
  return b.sb;
}
void fromXml(CVFlags &out, rostring str) {
  StrtokParse tok(str, "|");
  for (int i=0; i<tok; ++i) {
    char const * const token = tok[i];
    if(0) xfailure("?");
    READFLAG(CV_NONE);
    READFLAG(CV_CONST);
    READFLAG(CV_VOLATILE);
    READFLAG(CV_RESTRICT);
    READFLAG(CV_OWNER);
    else xfailure("illegal flag");
  }
}


// ------------------- DeclFlags --------------
char const * const declFlagNames[NUM_DECLFLAGS] = {
  "auto",           // 0
  "register",
  "static",
  "extern",
  "mutable",        // 4
  "inline",
  "virtual",
  "explicit",
  "friend",
  "typedef",        // 9

  "<enumerator>",
  "<global>",
  "<initialized>",
  "<builtin>",
  "<bound tparam>", // 14
  "<addrtaken>",
  "<parameter>",
  "<universal>",
  "<existential>",
  "<member>",       // 19
  "<definition>",
  "<inline_defn>",
  "<implicit>",
  "<forward>",
  "<temporary>",    // 24

  "<unused>",
  "namespace",
  "<extern \"C\">",
  "<selfname>",
  "<templ param>",  // 29
  "<using alias>",
  "<bitfield>",
};


string toString(DeclFlags df)
{ 
  // make sure I haven't added a flag without adding a string for it
  xassert(declFlagNames[NUM_DECLFLAGS-1] != NULL);

  return bitmapString(df, declFlagNames, NUM_DECLFLAGS);
}

string toXml(DeclFlags id) {
  if (id == DF_NONE) return "DF_NONE";
  DelimStr b('|');

  PRINTFLAG(DF_AUTO);
  PRINTFLAG(DF_REGISTER);
  PRINTFLAG(DF_STATIC);
  PRINTFLAG(DF_EXTERN);
  PRINTFLAG(DF_MUTABLE);
  PRINTFLAG(DF_INLINE);
  PRINTFLAG(DF_VIRTUAL);
  PRINTFLAG(DF_EXPLICIT);
  PRINTFLAG(DF_FRIEND);
  PRINTFLAG(DF_TYPEDEF);

  PRINTFLAG(DF_NAMESPACE);

  PRINTFLAG(DF_ENUMERATOR);
  PRINTFLAG(DF_GLOBAL);
  PRINTFLAG(DF_INITIALIZED);
  PRINTFLAG(DF_BUILTIN);
  PRINTFLAG(DF_PARAMETER);
  PRINTFLAG(DF_MEMBER);
  PRINTFLAG(DF_DEFINITION);
  PRINTFLAG(DF_INLINE_DEFN);
  PRINTFLAG(DF_IMPLICIT);

  PRINTFLAG(DF_FORWARD);
  PRINTFLAG(DF_TEMPORARY);
  PRINTFLAG(DF_EXTERN_C);
  PRINTFLAG(DF_SELFNAME);
  PRINTFLAG(DF_BOUND_TPARAM);
  PRINTFLAG(DF_TEMPL_PARAM);
  PRINTFLAG(DF_USING_ALIAS);
  PRINTFLAG(DF_BITFIELD);

  PRINTFLAG(DF_ADDRTAKEN);
  PRINTFLAG(DF_UNIVERSAL);
  PRINTFLAG(DF_EXISTENTIAL);

  return b.sb;
}
void fromXml(DeclFlags &out, rostring str) {
  StrtokParse tok(str, "|");
  for (int i=0; i<tok; ++i) {
    char const * const token = tok[i];
    if(0) xfailure("?");
    READFLAG(DF_NONE);

    READFLAG(DF_AUTO);
    READFLAG(DF_REGISTER);
    READFLAG(DF_STATIC);
    READFLAG(DF_EXTERN);
    READFLAG(DF_MUTABLE);
    READFLAG(DF_INLINE);
    READFLAG(DF_VIRTUAL);
    READFLAG(DF_EXPLICIT);
    READFLAG(DF_FRIEND);
    READFLAG(DF_TYPEDEF);

    READFLAG(DF_NAMESPACE);

    READFLAG(DF_ENUMERATOR);
    READFLAG(DF_GLOBAL);
    READFLAG(DF_INITIALIZED);
    READFLAG(DF_BUILTIN);
    READFLAG(DF_PARAMETER);
    READFLAG(DF_MEMBER);
    READFLAG(DF_DEFINITION);
    READFLAG(DF_INLINE_DEFN);
    READFLAG(DF_IMPLICIT);

    READFLAG(DF_FORWARD);
    READFLAG(DF_TEMPORARY);
    READFLAG(DF_EXTERN_C);
    READFLAG(DF_SELFNAME);
    READFLAG(DF_BOUND_TPARAM);
    READFLAG(DF_TEMPL_PARAM);
    READFLAG(DF_USING_ALIAS);
    READFLAG(DF_BITFIELD);

    READFLAG(DF_ADDRTAKEN);
    READFLAG(DF_UNIVERSAL);
    READFLAG(DF_EXISTENTIAL);
    else xfailure("illegal flag");
  }
}


// ----------------------- ScopeKind ----------------------------
char const *toString(ScopeKind sk)
{
  static char const * const arr[] = {
    "unknown",
    "global",
    "parameter",
    "function",
    "class",
    "template_params",
    "template_args",
    "namespace",
  };
  STATIC_ASSERT(TABLESIZE(arr) == NUM_SCOPEKINDS);

  xassert((unsigned)sk < NUM_SCOPEKINDS);
  return arr[sk];
}


// ---------------------- SimpleTypeId --------------------------
bool isValid(SimpleTypeId id)
{
  return 0 <= id && id <= NUM_SIMPLE_TYPES;
}


#define S(x) ((SimpleTypeFlags)(x))    // work around bitwise-OR in initializers..
static SimpleTypeInfo const simpleTypeInfoArray[] = {
  //name                   size,    flags
  { "char",                   1,    S(STF_INTEGER                          ) },
  { "unsigned char",          1,    S(STF_INTEGER | STF_UNSIGNED           ) },
  { "signed char",            1,    S(STF_INTEGER                          ) },
  { "bool",                   4,    S(STF_INTEGER                          ) },
  { "int",                    4,    S(STF_INTEGER | STF_PROM               ) },
  { "unsigned int",           4,    S(STF_INTEGER | STF_PROM | STF_UNSIGNED) },
  { "long int",               4,    S(STF_INTEGER | STF_PROM               ) },
  { "unsigned long int",      4,    S(STF_INTEGER | STF_PROM | STF_UNSIGNED) },
  { "long long int",          8,    S(STF_INTEGER | STF_PROM               ) },
  { "unsigned long long int", 8,    S(STF_INTEGER | STF_PROM | STF_UNSIGNED) },
  { "short int",              2,    S(STF_INTEGER                          ) },
  { "unsigned short int",     2,    S(STF_INTEGER | STF_UNSIGNED           ) },
  { "wchar_t",                2,    S(STF_INTEGER                          ) },
  { "float",                  4,    S(STF_FLOAT                            ) },
  { "double",                 8,    S(STF_FLOAT | STF_PROM                 ) },
  { "long double",           10,    S(STF_FLOAT                            ) },
  { "float _Complex",         8,    S(STF_FLOAT                            ) },
  { "double _Complex",       16,    S(STF_FLOAT                            ) },
  { "long double _Complex",  20,    S(STF_FLOAT                            ) },
  { "float _Imaginary",       4,    S(STF_FLOAT                            ) },
  { "double _Imaginary",      8,    S(STF_FLOAT                            ) },
  { "long double _Imaginary",10,    S(STF_FLOAT                            ) },
  { "void",                   1,    S(STF_NONE                             ) },    // gnu: sizeof(void) is 1

  // these should go away early on in typechecking
  { "...",                    0,    S(STF_NONE                             ) },
  { "/*cdtor*/",              0,    S(STF_NONE                             ) },    // dsw: don't want to print <cdtor>
  { "<error>",                0,    S(STF_NONE                             ) },
  { "<dependent>",            0,    S(STF_NONE                             ) },
  { "<implicit-int>",         0,    S(STF_NONE                             ) },
  { "<notfound>",             0,    S(STF_NONE                             ) },


  { "<prom_int>",             0,    S(STF_NONE                             ) },
  { "<prom_arith>",           0,    S(STF_NONE                             ) },
  { "<integral>",             0,    S(STF_NONE                             ) },
  { "<arith>",                0,    S(STF_NONE                             ) },
  { "<arith_nobool>",         0,    S(STF_NONE                             ) },
  { "<any_obj>",              0,    S(STF_NONE                             ) },
  { "<non_void>",             0,    S(STF_NONE                             ) },
  { "<any_type>",             0,    S(STF_NONE                             ) },
  

  { "<pret_strip_ref>",       0,    S(STF_NONE                             ) },
  { "<pret_ptm>",             0,    S(STF_NONE                             ) },
  { "<pret_arith_conv>",      0,    S(STF_NONE                             ) },
  { "<pret_first>",           0,    S(STF_NONE                             ) },
  { "<pret_first_ptr2ref>",   0,    S(STF_NONE                             ) },
  { "<pret_second>",          0,    S(STF_NONE                             ) },
  { "<pret_second_ptr2ref>",  0,    S(STF_NONE                             ) },
};
#undef S

SimpleTypeInfo const &simpleTypeInfo(SimpleTypeId id)
{
  STATIC_ASSERT(TABLESIZE(simpleTypeInfoArray) == NUM_SIMPLE_TYPES);
  xassert(isValid(id));
  return simpleTypeInfoArray[id];
}


bool isComplexOrImaginary(SimpleTypeId id)
{
  return ST_FLOAT_COMPLEX <= id && id <= ST_DOUBLE_IMAGINARY;
}

char const *toXml(SimpleTypeId id) {
  switch(id) {
  default: xfailure("bad enum"); break;
    PRINTENUM(ST_CHAR);
    PRINTENUM(ST_UNSIGNED_CHAR);
    PRINTENUM(ST_SIGNED_CHAR);
    PRINTENUM(ST_BOOL);
    PRINTENUM(ST_INT);
    PRINTENUM(ST_UNSIGNED_INT);
    PRINTENUM(ST_LONG_INT);
    PRINTENUM(ST_UNSIGNED_LONG_INT);
    PRINTENUM(ST_LONG_LONG);
    PRINTENUM(ST_UNSIGNED_LONG_LONG);
    PRINTENUM(ST_SHORT_INT);
    PRINTENUM(ST_UNSIGNED_SHORT_INT);
    PRINTENUM(ST_WCHAR_T);
    PRINTENUM(ST_FLOAT);
    PRINTENUM(ST_DOUBLE);
    PRINTENUM(ST_LONG_DOUBLE);
    PRINTENUM(ST_FLOAT_COMPLEX);
    PRINTENUM(ST_DOUBLE_COMPLEX);
    PRINTENUM(ST_LONG_DOUBLE_COMPLEX);
    PRINTENUM(ST_FLOAT_IMAGINARY);
    PRINTENUM(ST_DOUBLE_IMAGINARY);
    PRINTENUM(ST_LONG_DOUBLE_IMAGINARY);
    PRINTENUM(ST_VOID);

    PRINTENUM(ST_ELLIPSIS);
    PRINTENUM(ST_CDTOR);
    PRINTENUM(ST_ERROR);
    PRINTENUM(ST_DEPENDENT);
    PRINTENUM(ST_IMPLINT);
    PRINTENUM(ST_NOTFOUND);

    PRINTENUM(ST_PROMOTED_INTEGRAL);
    PRINTENUM(ST_PROMOTED_ARITHMETIC);
    PRINTENUM(ST_INTEGRAL);
    PRINTENUM(ST_ARITHMETIC);
    PRINTENUM(ST_ARITHMETIC_NON_BOOL);
    PRINTENUM(ST_ANY_OBJ_TYPE);
    PRINTENUM(ST_ANY_NON_VOID);
    PRINTENUM(ST_ANY_TYPE);

    PRINTENUM(ST_PRET_STRIP_REF);
    PRINTENUM(ST_PRET_PTM);
    PRINTENUM(ST_PRET_ARITH_CONV);
    PRINTENUM(ST_PRET_FIRST);
    PRINTENUM(ST_PRET_FIRST_PTR2REF);
    PRINTENUM(ST_PRET_SECOND);
    PRINTENUM(ST_PRET_SECOND_PTR2REF);
  }
}
void fromXml(SimpleTypeId &out, rostring str) {
  if(0) xfailure("?");
  READENUM(ST_CHAR);
  READENUM(ST_UNSIGNED_CHAR);
  READENUM(ST_SIGNED_CHAR);
  READENUM(ST_BOOL);
  READENUM(ST_INT);
  READENUM(ST_UNSIGNED_INT);
  READENUM(ST_LONG_INT);
  READENUM(ST_UNSIGNED_LONG_INT);
  READENUM(ST_LONG_LONG);
  READENUM(ST_UNSIGNED_LONG_LONG);
  READENUM(ST_SHORT_INT);
  READENUM(ST_UNSIGNED_SHORT_INT);
  READENUM(ST_WCHAR_T);
  READENUM(ST_FLOAT);
  READENUM(ST_DOUBLE);
  READENUM(ST_LONG_DOUBLE);
  READENUM(ST_FLOAT_COMPLEX);
  READENUM(ST_DOUBLE_COMPLEX);
  READENUM(ST_LONG_DOUBLE_COMPLEX);
  READENUM(ST_FLOAT_IMAGINARY);
  READENUM(ST_DOUBLE_IMAGINARY);
  READENUM(ST_LONG_DOUBLE_IMAGINARY);
  READENUM(ST_VOID);

  READENUM(ST_ELLIPSIS);
  READENUM(ST_CDTOR);
  READENUM(ST_ERROR);
  READENUM(ST_DEPENDENT);
  READENUM(ST_IMPLINT);
  READENUM(ST_NOTFOUND);

  READENUM(ST_PROMOTED_INTEGRAL);
  READENUM(ST_PROMOTED_ARITHMETIC);
  READENUM(ST_INTEGRAL);
  READENUM(ST_ARITHMETIC);
  READENUM(ST_ARITHMETIC_NON_BOOL);
  READENUM(ST_ANY_OBJ_TYPE);
  READENUM(ST_ANY_NON_VOID);
  READENUM(ST_ANY_TYPE);

  READENUM(ST_PRET_STRIP_REF);
  READENUM(ST_PRET_PTM);
  READENUM(ST_PRET_ARITH_CONV);
  READENUM(ST_PRET_FIRST);
  READENUM(ST_PRET_FIRST_PTR2REF);
  READENUM(ST_PRET_SECOND);
  READENUM(ST_PRET_SECOND_PTR2REF);
  else xfailure("bad enum string");
}


// ------------------------ UnaryOp -----------------------------
char const * const unaryOpNames[NUM_UNARYOPS] = {
  "+",
  "-",
  "!",
  "~"
};

MAKE_TOSTRING(UnaryOp, NUM_UNARYOPS, unaryOpNames)

char const *toXml(UnaryOp id) {
  switch(id) {
  default: xfailure("bad enum"); break;
    PRINTENUM(UNY_PLUS);
    PRINTENUM(UNY_MINUS);
    PRINTENUM(UNY_NOT);
    PRINTENUM(UNY_BITNOT);
  }
}
void fromXml(UnaryOp &out, rostring str) {
  if(0) xfailure("?");
  READENUM(UNY_PLUS);
  READENUM(UNY_MINUS);
  READENUM(UNY_NOT);
  READENUM(UNY_BITNOT);
  else xfailure("bad enum string");
}


char const * const effectOpNames[NUM_EFFECTOPS] = {
  "++/*postfix*/",
  "--/*postfix*/",
  "++/*prefix*/",
  "--/*prefix*/",
};

MAKE_TOSTRING(EffectOp, NUM_EFFECTOPS, effectOpNames)
char const *toXml(EffectOp id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(EFF_POSTINC);
  PRINTENUM(EFF_POSTDEC);
  PRINTENUM(EFF_PREINC);
  PRINTENUM(EFF_PREDEC);
  }
}
void fromXml(EffectOp &out, rostring str) {
  if(0) xfailure("?");
  READENUM(EFF_POSTINC);
  READENUM(EFF_POSTDEC);
  READENUM(EFF_PREINC);
  READENUM(EFF_PREDEC);
  else xfailure("bad enum string");
}


bool isPostfix(EffectOp op)
{
  return op <= EFF_POSTDEC;
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
  ",",

  "<?",
  ">?",

  "[]",

  "=",

  ".*",
  "->*",

  "==>",
  "<==>",
};

MAKE_TOSTRING(BinaryOp, NUM_BINARYOPS, binaryOpNames)

char const *toXml(BinaryOp id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(BIN_EQUAL);
  PRINTENUM(BIN_NOTEQUAL);
  PRINTENUM(BIN_LESS);
  PRINTENUM(BIN_GREATER);
  PRINTENUM(BIN_LESSEQ);
  PRINTENUM(BIN_GREATEREQ);

  PRINTENUM(BIN_MULT);
  PRINTENUM(BIN_DIV);
  PRINTENUM(BIN_MOD);
  PRINTENUM(BIN_PLUS);
  PRINTENUM(BIN_MINUS);
  PRINTENUM(BIN_LSHIFT);
  PRINTENUM(BIN_RSHIFT);
  PRINTENUM(BIN_BITAND);
  PRINTENUM(BIN_BITXOR);
  PRINTENUM(BIN_BITOR);
  PRINTENUM(BIN_AND);
  PRINTENUM(BIN_OR);
  PRINTENUM(BIN_COMMA);

  // gcc extensions
  PRINTENUM(BIN_MINIMUM);
  PRINTENUM(BIN_MAXIMUM);

  // this exists only between parsing and typechecking
  PRINTENUM(BIN_BRACKETS);

  PRINTENUM(BIN_ASSIGN);

  // C++ operators
  PRINTENUM(BIN_DOT_STAR);
  PRINTENUM(BIN_ARROW_STAR);

  // theorem prover extension
  PRINTENUM(BIN_IMPLIES);
  PRINTENUM(BIN_EQUIVALENT);
  }
}
void fromXml(BinaryOp &out, rostring str) {
  if(0) xfailure("?");
  READENUM(BIN_EQUAL);
  READENUM(BIN_NOTEQUAL);
  READENUM(BIN_LESS);
  READENUM(BIN_GREATER);
  READENUM(BIN_LESSEQ);
  READENUM(BIN_GREATEREQ);

  READENUM(BIN_MULT);
  READENUM(BIN_DIV);
  READENUM(BIN_MOD);
  READENUM(BIN_PLUS);
  READENUM(BIN_MINUS);
  READENUM(BIN_LSHIFT);
  READENUM(BIN_RSHIFT);
  READENUM(BIN_BITAND);
  READENUM(BIN_BITXOR);
  READENUM(BIN_BITOR);
  READENUM(BIN_AND);
  READENUM(BIN_OR);
  READENUM(BIN_COMMA);

  // gcc extensions
  READENUM(BIN_MINIMUM);
  READENUM(BIN_MAXIMUM);

  // this exists only between parsing and typechecking
  READENUM(BIN_BRACKETS);

  READENUM(BIN_ASSIGN);

  // C++ operators
  READENUM(BIN_DOT_STAR);
  READENUM(BIN_ARROW_STAR);

  // theorem prover extension
  READENUM(BIN_IMPLIES);
  READENUM(BIN_EQUIVALENT);
  else xfailure("bad enum string");
}


bool isPredicateCombinator(BinaryOp op)
{
  return op==BIN_AND || op==BIN_OR || op==BIN_IMPLIES || op==BIN_EQUIVALENT;
}

bool isRelational(BinaryOp op)
{
  return BIN_EQUAL <= op && op <= BIN_GREATEREQ;
}

bool isInequality(BinaryOp op)
{
  return BIN_LESS <= op && op <= BIN_GREATEREQ;
}

bool isOverloadable(BinaryOp op)
{
  return BIN_EQUAL <= op && op <= BIN_BRACKETS ||
         op == BIN_ARROW_STAR;
}


// ------------------- AccessKeyword -------------------
char const * const accessKeywordNames[NUM_ACCESS_KEYWORDS] = {
  "public",
  "protected",
  "private",
  "unspecified"
};

MAKE_TOSTRING(AccessKeyword, NUM_ACCESS_KEYWORDS, accessKeywordNames)

char const *toXml(AccessKeyword id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(AK_PUBLIC);
  PRINTENUM(AK_PROTECTED);
  PRINTENUM(AK_PRIVATE);
  PRINTENUM(AK_UNSPECIFIED);
  }
}
void fromXml(AccessKeyword &out, rostring str) {
  if(0) xfailure("?");
  READENUM(AK_PUBLIC);
  READENUM(AK_PROTECTED);
  READENUM(AK_PRIVATE);
  READENUM(AK_UNSPECIFIED);
  else xfailure("bad enum string");
}


// -------------------- CastKeyword --------------------
char const * const castKeywordNames[NUM_CAST_KEYWORDS] = {
  "dynamic_cast",
  "static_cast",
  "reinterpret_cast",
  "const_cast"
};

MAKE_TOSTRING(CastKeyword, NUM_CAST_KEYWORDS, castKeywordNames)

char const *toXml(CastKeyword id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(CK_DYNAMIC);
  PRINTENUM(CK_STATIC);
  PRINTENUM(CK_REINTERPRET);
  PRINTENUM(CK_CONST);
  }
}
void fromXml(CastKeyword &out, rostring str) {
  if(0) xfailure("?");
  READENUM(CK_DYNAMIC);
  READENUM(CK_STATIC);
  READENUM(CK_REINTERPRET);
  READENUM(CK_CONST);
  else xfailure("bad enum string");
}


// -------------------- OverloadableOp --------------------
char const * const overloadableOpNames[NUM_OVERLOADABLE_OPS] = {
  "!",
  "~",

  "++",
  "--",

  "+",
  "-",
  "*",
  "&",

  "/",
  "%",
  "<<",
  ">>",
  "^",
  "|",

  "=",
  "+=",
  "-=",
  "*=",
  "/=",
  "%=",
  "<<=",
  ">>=",
  "&=",
  "^=",
  "|=",

  "==",
  "!=",
  "<",
  ">",
  "<=",
  ">=",

  "&&",
  "||",

  "->",
  "->*",

  "[]",
  "()",
  ",",
  "?:",
  
  "<?",
  ">?",
};

MAKE_TOSTRING(OverloadableOp, NUM_OVERLOADABLE_OPS, overloadableOpNames)

char const *toXml(OverloadableOp id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(OP_NOT);
  PRINTENUM(OP_BITNOT);

  // unary, both prefix and postfix; latter is declared as 2-arg function
  PRINTENUM(OP_PLUSPLUS);
  PRINTENUM(OP_MINUSMINUS);

  // unary or binary
  PRINTENUM(OP_PLUS);
  PRINTENUM(OP_MINUS);
  PRINTENUM(OP_STAR);
  PRINTENUM(OP_AMPERSAND);

  // arithmetic
  PRINTENUM(OP_DIV);
  PRINTENUM(OP_MOD);
  PRINTENUM(OP_LSHIFT);
  PRINTENUM(OP_RSHIFT);
  PRINTENUM(OP_BITXOR);
  PRINTENUM(OP_BITOR);

  // arithmetic+assignment
  PRINTENUM(OP_ASSIGN);
  PRINTENUM(OP_PLUSEQ);
  PRINTENUM(OP_MINUSEQ);
  PRINTENUM(OP_MULTEQ);
  PRINTENUM(OP_DIVEQ);
  PRINTENUM(OP_MODEQ);
  PRINTENUM(OP_LSHIFTEQ);
  PRINTENUM(OP_RSHIFTEQ);
  PRINTENUM(OP_BITANDEQ);
  PRINTENUM(OP_BITXOREQ);
  PRINTENUM(OP_BITOREQ);

  // comparison
  PRINTENUM(OP_EQUAL);
  PRINTENUM(OP_NOTEQUAL);
  PRINTENUM(OP_LESS);
  PRINTENUM(OP_GREATER);
  PRINTENUM(OP_LESSEQ);
  PRINTENUM(OP_GREATEREQ);

  // logical
  PRINTENUM(OP_AND);
  PRINTENUM(OP_OR);

  // arrows
  PRINTENUM(OP_ARROW);
  PRINTENUM(OP_ARROW_STAR);

  // misc
  PRINTENUM(OP_BRACKETS);
  PRINTENUM(OP_PARENS);
  PRINTENUM(OP_COMMA);
  PRINTENUM(OP_QUESTION);
  
  // gcc extensions
  PRINTENUM(OP_MINIMUM);
  PRINTENUM(OP_MAXIMUM);
  }
}
void fromXml(OverloadableOp &out, rostring str) {
  if(0) xfailure("?");
  READENUM(OP_NOT);
  READENUM(OP_BITNOT);

  // unary, both prefix and postfix; latter is declared as 2-arg function
  READENUM(OP_PLUSPLUS);
  READENUM(OP_MINUSMINUS);

  // unary or binary
  READENUM(OP_PLUS);
  READENUM(OP_MINUS);
  READENUM(OP_STAR);
  READENUM(OP_AMPERSAND);

  // arithmetic
  READENUM(OP_DIV);
  READENUM(OP_MOD);
  READENUM(OP_LSHIFT);
  READENUM(OP_RSHIFT);
  READENUM(OP_BITXOR);
  READENUM(OP_BITOR);

  // arithmetic+assignment
  READENUM(OP_ASSIGN);
  READENUM(OP_PLUSEQ);
  READENUM(OP_MINUSEQ);
  READENUM(OP_MULTEQ);
  READENUM(OP_DIVEQ);
  READENUM(OP_MODEQ);
  READENUM(OP_LSHIFTEQ);
  READENUM(OP_RSHIFTEQ);
  READENUM(OP_BITANDEQ);
  READENUM(OP_BITXOREQ);
  READENUM(OP_BITOREQ);

  // comparison
  READENUM(OP_EQUAL);
  READENUM(OP_NOTEQUAL);
  READENUM(OP_LESS);
  READENUM(OP_GREATER);
  READENUM(OP_LESSEQ);
  READENUM(OP_GREATEREQ);

  // logical
  READENUM(OP_AND);
  READENUM(OP_OR);

  // arrows
  READENUM(OP_ARROW);
  READENUM(OP_ARROW_STAR);

  // misc
  READENUM(OP_BRACKETS);
  READENUM(OP_PARENS);
  READENUM(OP_COMMA);
  READENUM(OP_QUESTION);
  
  // gcc extensions
  READENUM(OP_MINIMUM);
  READENUM(OP_MAXIMUM);
  else xfailure("bad enum string");
}


char const * const operatorFunctionNames[NUM_OVERLOADABLE_OPS] = {
  "operator!",
  "operator~",

  "operator++",
  "operator--",

  "operator+",
  "operator-",
  "operator*",
  "operator&",

  "operator/",
  "operator%",
  "operator<<",
  "operator>>",
  "operator^",
  "operator|",

  "operator=",
  "operator+=",
  "operator-=",
  "operator*=",
  "operator/=",
  "operator%=",
  "operator<<=",
  "operator>>=",
  "operator&=",
  "operator^=",
  "operator|=",

  "operator==",
  "operator!=",
  "operator<",
  "operator>",
  "operator<=",
  "operator>=",

  "operator&&",
  "operator||",

  "operator->",
  "operator->*",

  "operator[]",
  "operator()",
  "operator,",
  "operator?",
  
  "operator<?",
  "operator>?",
};


OverloadableOp toOverloadableOp(UnaryOp op)
{
  static OverloadableOp const map[] = {
    OP_PLUS,
    OP_MINUS,
    OP_NOT,
    OP_BITNOT
  };
  ASSERT_TABLESIZE(map, NUM_UNARYOPS);
  xassert(validCode(op));
  return map[op];
}

OverloadableOp toOverloadableOp(EffectOp op)
{
  static OverloadableOp const map[] = {
    OP_PLUSPLUS,
    OP_MINUSMINUS,
    OP_PLUSPLUS,
    OP_MINUSMINUS,
  };
  ASSERT_TABLESIZE(map, NUM_EFFECTOPS);
  xassert(validCode(op));
  return map[op];
}

OverloadableOp toOverloadableOp(BinaryOp op, bool isAssignment)
{ 
  xassert(validCode(op));

  // in the table, this means that an operator cannot be overloaded
  #define BAD_ENTRY NUM_OVERLOADABLE_OPS
  OverloadableOp ret = BAD_ENTRY;

  if (!isAssignment) {
    static OverloadableOp const map[] = {
      OP_EQUAL,
      OP_NOTEQUAL,
      OP_LESS,
      OP_GREATER,
      OP_LESSEQ,
      OP_GREATEREQ,

      OP_STAR,
      OP_DIV,
      OP_MOD,
      OP_PLUS,
      OP_MINUS,
      OP_LSHIFT,
      OP_RSHIFT,
      OP_AMPERSAND,
      OP_BITXOR,
      OP_BITOR,
      OP_AND,
      OP_OR,
      OP_COMMA,
      
      OP_MINIMUM,
      OP_MAXIMUM,

      OP_BRACKETS,

      BAD_ENTRY,      // isAssignment is false

      BAD_ENTRY,      // cannot overload
      OP_ARROW_STAR,

      BAD_ENTRY,      // extension..
      BAD_ENTRY,
    };
    ASSERT_TABLESIZE(map, NUM_BINARYOPS);
    ret = map[op];
  }

  else {
    static OverloadableOp const map[] = {
      BAD_ENTRY,
      BAD_ENTRY,
      BAD_ENTRY,
      BAD_ENTRY,
      BAD_ENTRY,
      BAD_ENTRY,

      OP_MULTEQ,
      OP_DIVEQ,
      OP_MODEQ,
      OP_PLUSEQ,
      OP_MINUSEQ,
      OP_LSHIFTEQ,
      OP_RSHIFTEQ,
      OP_BITANDEQ,
      OP_BITXOREQ,
      OP_BITOREQ,
      BAD_ENTRY,
      BAD_ENTRY,
      BAD_ENTRY,

      BAD_ENTRY,
      BAD_ENTRY,

      BAD_ENTRY,

      OP_ASSIGN,

      BAD_ENTRY,
      BAD_ENTRY,

      BAD_ENTRY,
      BAD_ENTRY,
    };
    ASSERT_TABLESIZE(map, NUM_BINARYOPS);
    ret = map[op];
  }
  
  xassert(ret != BAD_ENTRY);    // otherwise why did you try to map it?
  return ret;

  #undef BAD_ENTRY
}

// ------------------------ UberModifiers ---------------------
char const * const uberModifierNames[UM_NUM_FLAGS] = {
  "auto",            // 0x00000001
  "register",
  "static",
  "extern",
  "mutable",         // 0x00000010
  "inline",
  "virtual",
  "explicit",
  "friend",          // 0x00000100
  "typedef",

  "const",
  "volatile",
  "restrict",        // 0x00001000

  "wchar_t",
  "bool",
  "short",
  "int",             // 0x00010000
  "long",
  "signed",
  "unsigned",
  "float",           // 0x00100000
  "double",
  "void",
  "long long",
  "char",            // 0x01000000
  "complex",
  "imaginary"
};

string toString(UberModifiers m)
{
  xassert(uberModifierNames[UM_NUM_FLAGS-1] != NULL);
  return bitmapString(m, uberModifierNames, UM_NUM_FLAGS);
}


// ---------------------- SpecialExpr -----------------
char const *toString(SpecialExpr se)
{
  switch (se) {       
    default: xfailure("bad se code");
    case SE_NONE:       return "SE_NONE";
    case SE_ZERO:       return "SE_ZERO";
    case SE_STRINGLIT:  return "SE_STRINGLIT";
  }
}
