/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_NAME = 258,
     TOK_INTLIT = 259,
     TOK_EMBEDDED_CODE = 260,
     TOK_LBRACE = 261,
     TOK_RBRACE = 262,
     TOK_SEMICOLON = 263,
     TOK_ARROW = 264,
     TOK_LPAREN = 265,
     TOK_RPAREN = 266,
     TOK_LANGLE = 267,
     TOK_RANGLE = 268,
     TOK_STAR = 269,
     TOK_AMPERSAND = 270,
     TOK_COMMA = 271,
     TOK_EQUALS = 272,
     TOK_COLON = 273,
     TOK_CLASS = 274,
     TOK_PUBLIC = 275,
     TOK_PRIVATE = 276,
     TOK_PROTECTED = 277,
     TOK_VERBATIM = 278,
     TOK_IMPL_VERBATIM = 279,
     TOK_OCAML_TYPE_VERBATIM = 280,
     TOK_XML_VERBATIM = 281,
     TOK_CTOR = 282,
     TOK_DTOR = 283,
     TOK_PURE_VIRTUAL = 284,
     TOK_CUSTOM = 285,
     TOK_OPTION = 286,
     TOK_NEW = 287,
     TOK_ENUM = 288
   };
#endif
#define TOK_NAME 258
#define TOK_INTLIT 259
#define TOK_EMBEDDED_CODE 260
#define TOK_LBRACE 261
#define TOK_RBRACE 262
#define TOK_SEMICOLON 263
#define TOK_ARROW 264
#define TOK_LPAREN 265
#define TOK_RPAREN 266
#define TOK_LANGLE 267
#define TOK_RANGLE 268
#define TOK_STAR 269
#define TOK_AMPERSAND 270
#define TOK_COMMA 271
#define TOK_EQUALS 272
#define TOK_COLON 273
#define TOK_CLASS 274
#define TOK_PUBLIC 275
#define TOK_PRIVATE 276
#define TOK_PROTECTED 277
#define TOK_VERBATIM 278
#define TOK_IMPL_VERBATIM 279
#define TOK_OCAML_TYPE_VERBATIM 280
#define TOK_XML_VERBATIM 281
#define TOK_CTOR 282
#define TOK_DTOR 283
#define TOK_PURE_VIRTUAL 284
#define TOK_CUSTOM 285
#define TOK_OPTION 286
#define TOK_NEW 287
#define TOK_ENUM 288




/* Copy the first part of user declarations.  */
#line 6 "agrampar.y"


#include "agrampar.h"       // agrampar_yylex, etc.

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

// enable debugging the parser
#ifndef NDEBUG
  #define YYDEBUG 1
#endif

// permit having other parser's codes in the same program
#define yyparse agrampar_yyparse



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 70 "agrampar.y"
typedef union YYSTYPE {
  ASTSpecFile *file;
  ASTList<ToplevelForm> *formList;
  TF_class *tfClass;
  ASTList<FieldOrCtorArg> *ctorArgList;
  ASTList<Annotation> *userDeclList;
  string *str;
  enum AccessCtl accessCtl;
  AccessMod *accessMod;
  ToplevelForm *verbatim;
  Annotation *annotation;
  TF_option *tfOption;
  ASTList<string> *stringList;
  TF_enum *tfEnum;
  ASTList<string> *enumeratorList;
  string *enumerator;
  ASTList<BaseClass> *baseClassList;
  BaseClass *baseClass;
  CustomCode *customCode;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 180 "agrampar.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 192 "agrampar.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   118

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  34
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  30
/* YYNRULES -- Number of rules. */
#define YYNRULES  74
/* YYNRULES -- Number of states. */
#define YYNSTATES  118

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   288

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    15,    18,    21,
      24,    31,    39,    40,    42,    46,    48,    49,    56,    65,
      68,    69,    71,    74,    78,    80,    84,    86,    89,    91,
      93,    97,    99,   101,   103,   105,   107,   111,   112,   115,
     118,   124,   126,   130,   133,   137,   139,   141,   143,   145,
     147,   149,   151,   156,   158,   162,   165,   168,   171,   174,
     179,   180,   183,   189,   196,   198,   202,   204,   205,   208,
     210,   214,   216,   218,   220
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      35,     0,    -1,    36,    -1,    -1,    36,    37,    -1,    36,
      54,    -1,    36,    55,    -1,    36,    57,    -1,    36,    49,
      -1,    36,     8,    -1,    38,    19,     3,    41,    60,    39,
      -1,    38,    19,     3,    42,    42,    60,    39,    -1,    -1,
      32,    -1,     6,    40,     7,    -1,     8,    -1,    -1,    40,
       9,     3,    41,    60,     8,    -1,    40,     9,     3,    41,
      60,     6,    47,     7,    -1,    40,    48,    -1,    -1,    42,
      -1,    10,    11,    -1,    10,    43,    11,    -1,    44,    -1,
      43,    16,    44,    -1,    45,    -1,    44,    45,    -1,     3,
      -1,     4,    -1,    12,    46,    13,    -1,    14,    -1,    15,
      -1,    17,    -1,    19,    -1,    44,    -1,    44,    16,    46,
      -1,    -1,    47,    48,    -1,    52,    50,    -1,    52,     5,
      17,     5,     8,    -1,    49,    -1,    30,     3,    50,    -1,
       5,     8,    -1,     6,     5,     7,    -1,    20,    -1,    21,
      -1,    22,    -1,    27,    -1,    28,    -1,    29,    -1,    51,
      -1,    51,    10,    53,    11,    -1,     3,    -1,    53,    16,
       3,    -1,    23,    50,    -1,    24,    50,    -1,    25,    50,
      -1,    26,    50,    -1,    31,     3,    56,     8,    -1,    -1,
      56,     3,    -1,    33,     3,     6,    58,     7,    -1,    33,
       3,     6,    58,    16,     7,    -1,    59,    -1,    58,    16,
      59,    -1,     3,    -1,    -1,    18,    61,    -1,    63,    -1,
      61,    16,    63,    -1,    20,    -1,    21,    -1,    22,    -1,
      62,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   117,   117,   123,   124,   125,   126,   127,   128,   129,
     134,   139,   148,   149,   161,   163,   171,   172,   179,   184,
     192,   193,   199,   201,   206,   213,   218,   220,   226,   227,
     228,   229,   230,   231,   232,   236,   238,   245,   246,   252,
     254,   256,   262,   268,   270,   276,   277,   278,   279,   280,
     281,   285,   287,   292,   294,   299,   301,   303,   305,   310,
     316,   317,   322,   324,   329,   331,   336,   342,   343,   348,
     350,   356,   357,   358,   362
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_NAME", "TOK_INTLIT",
  "TOK_EMBEDDED_CODE", "\"{\"", "\"}\"", "\";\"", "\"->\"", "\"(\"",
  "\")\"", "\"<\"", "\">\"", "\"*\"", "\"&\"", "\",\"", "\"=\"", "\":\"",
  "\"class\"", "\"public\"", "\"private\"", "\"protected\"",
  "\"verbatim\"", "\"impl_verbatim\"", "\"ocaml_type_verbatim\"",
  "\"xml_verbatim\"", "\"ctor\"", "\"dtor\"", "\"pure_virtual\"",
  "\"custom\"", "\"option\"", "\"new\"", "\"enum\"", "$accept",
  "StartSymbol", "Input", "Class", "NewOpt", "ClassBody",
  "ClassMembersOpt", "CtorArgsOpt", "CtorArgs", "CtorArgList", "Arg",
  "ArgWord", "ArgList", "CtorMembersOpt", "Annotation", "CustomCode",
  "Embedded", "Public", "AccessMod", "StringList", "Verbatim", "Option",
  "OptionArgs", "Enum", "EnumeratorSeq", "Enumerator", "BaseClassesOpt",
  "BaseClassSeq", "BaseAccess", "BaseClass", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    34,    35,    36,    36,    36,    36,    36,    36,    36,
      37,    37,    38,    38,    39,    39,    40,    40,    40,    40,
      41,    41,    42,    42,    43,    43,    44,    44,    45,    45,
      45,    45,    45,    45,    45,    46,    46,    47,    47,    48,
      48,    48,    49,    50,    50,    51,    51,    51,    51,    51,
      51,    52,    52,    53,    53,    54,    54,    54,    54,    55,
      56,    56,    57,    57,    58,    58,    59,    60,    60,    61,
      61,    62,    62,    62,    63
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     2,     2,     2,     2,
       6,     7,     0,     1,     3,     1,     0,     6,     8,     2,
       0,     1,     2,     3,     1,     3,     1,     2,     1,     1,
       3,     1,     1,     1,     1,     1,     3,     0,     2,     2,
       5,     1,     3,     2,     3,     1,     1,     1,     1,     1,
       1,     1,     4,     1,     3,     2,     2,     2,     2,     4,
       0,     2,     5,     6,     1,     3,     1,     0,     2,     1,
       3,     1,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     2,     1,     9,     0,     0,     0,     0,     0,
       0,    13,     0,     4,     0,     8,     5,     6,     7,     0,
       0,    55,    56,    57,    58,     0,    60,     0,     0,    43,
       0,    42,     0,     0,    20,    44,    61,    59,    66,     0,
      64,     0,    67,    21,    62,     0,    28,    29,    22,     0,
      31,    32,    33,    34,     0,    24,    26,     0,     0,    67,
      63,    65,    35,     0,    23,     0,    27,    71,    72,    73,
      68,     0,    69,    16,    15,    10,     0,     0,    30,    25,
       0,    74,     0,    11,    36,    70,    14,     0,    45,    46,
      47,    48,    49,    50,    19,    41,    51,     0,    20,     0,
       0,    39,    67,    21,    53,     0,     0,     0,    52,     0,
       0,    37,    17,    54,    40,     0,    18,    38
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     2,    13,    14,    75,    82,    42,    43,    54,
      62,    56,    63,   115,    94,    95,    21,    96,    97,   105,
      16,    17,    32,    18,    39,    40,    58,    70,    71,    72
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -56
static const yysigned_char yypact[] =
{
     -56,    10,    -3,   -56,   -56,    50,    50,    50,    50,    23,
      31,   -56,    39,   -56,    29,   -56,   -56,   -56,   -56,    49,
      64,   -56,   -56,   -56,   -56,    50,   -56,    62,    73,   -56,
      72,   -56,     9,    90,    86,   -56,   -56,   -56,   -56,    -1,
     -56,    63,    82,    86,   -56,    78,   -56,   -56,   -56,    80,
     -56,   -56,   -56,   -56,    33,    80,   -56,    66,     5,    82,
     -56,   -56,    21,    85,   -56,    80,   -56,   -56,   -56,   -56,
      87,    98,   -56,   -56,   -56,   -56,     5,    80,   -56,    80,
      66,   -56,    32,   -56,   -56,   -56,   -56,    99,   -56,   -56,
     -56,   -56,   -56,   -56,   -56,   -56,    94,    84,    86,   102,
       1,   -56,    82,   -56,   -56,    35,   101,    37,   -56,   104,
     100,   -56,   -56,   -56,   -56,    43,   -56,   -56
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -56,   -56,   -56,   -56,   -56,    34,   -56,    11,   -40,   -56,
     -33,   -48,    36,   -56,    -4,   110,    -6,   -56,   -56,   -56,
     -56,   -56,   -56,   -56,   -56,    69,   -55,   -56,   -56,    38
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -13
static const yysigned_char yytable[] =
{
      22,    23,    24,    59,    76,     4,    44,    66,    55,    29,
       3,    73,    36,    74,    66,    45,   -12,    37,   106,    31,
       5,     6,     7,     8,    46,    47,    25,     9,    10,    11,
      12,    66,    79,    49,    26,    50,    51,    77,    52,    86,
      53,    87,    27,   111,    64,   112,   108,   107,    28,    65,
     116,   109,    88,    89,    90,    19,    20,    29,   103,    91,
      92,    93,     9,    88,    89,    90,    46,    47,    33,    30,
      91,    92,    93,     9,    48,    49,    34,    50,    51,    35,
      52,    38,    53,    46,    47,    60,    67,    68,    69,   100,
      20,   101,    49,    38,    50,    51,    41,    52,    78,    53,
      57,    81,    98,    80,    99,   104,   110,   113,   114,   102,
      83,   117,    15,    84,    61,     0,     0,     0,    85
};

static const yysigned_char yycheck[] =
{
       6,     7,     8,    43,    59,     8,     7,    55,    41,     8,
       0,     6,     3,     8,    62,    16,    19,     8,    17,    25,
      23,    24,    25,    26,     3,     4,     3,    30,    31,    32,
      33,    79,    65,    12,     3,    14,    15,    16,    17,     7,
      19,     9,     3,     6,    11,     8,    11,   102,    19,    16,
       7,    16,    20,    21,    22,     5,     6,     8,    98,    27,
      28,    29,    30,    20,    21,    22,     3,     4,     6,     5,
      27,    28,    29,    30,    11,    12,     3,    14,    15,     7,
      17,     3,    19,     3,     4,     7,    20,    21,    22,     5,
       6,    97,    12,     3,    14,    15,    10,    17,    13,    19,
      18,     3,     3,    16,    10,     3,     5,     3,     8,    98,
      76,   115,     2,    77,    45,    -1,    -1,    -1,    80
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    35,    36,     0,     8,    23,    24,    25,    26,    30,
      31,    32,    33,    37,    38,    49,    54,    55,    57,     5,
       6,    50,    50,    50,    50,     3,     3,     3,    19,     8,
       5,    50,    56,     6,     3,     7,     3,     8,     3,    58,
      59,    10,    41,    42,     7,    16,     3,     4,    11,    12,
      14,    15,    17,    19,    43,    44,    45,    18,    60,    42,
       7,    59,    44,    46,    11,    16,    45,    20,    21,    22,
      61,    62,    63,     6,     8,    39,    60,    16,    13,    44,
      16,     3,    40,    39,    46,    63,     7,     9,    20,    21,
      22,    27,    28,    29,    48,    49,    51,    52,     3,    10,
       5,    50,    41,    42,     3,    53,    17,    60,    11,    16,
       5,     6,     8,     3,     8,    47,     7,    48
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;


  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 118 "agrampar.y"
    { yyval.file = *((ASTSpecFile**)parseParam) = new ASTSpecFile(yyvsp[0].formList); ;}
    break;

  case 3:
#line 123 "agrampar.y"
    { yyval.formList = new ASTList<ToplevelForm>; ;}
    break;

  case 4:
#line 124 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].tfClass); ;}
    break;

  case 5:
#line 125 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].verbatim); ;}
    break;

  case 6:
#line 126 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].tfOption); ;}
    break;

  case 7:
#line 127 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].tfEnum); ;}
    break;

  case 8:
#line 128 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(new TF_custom(yyvsp[0].customCode)); ;}
    break;

  case 9:
#line 129 "agrampar.y"
    { yyval.formList=yyvsp[-1].formList; ;}
    break;

  case 10:
#line 135 "agrampar.y"
    { (yyval.tfClass=yyvsp[0].tfClass)->super->name = unbox(yyvsp[-3].str); 
           yyval.tfClass->super->args.steal(yyvsp[-2].ctorArgList); 
           yyval.tfClass->super->bases.steal(yyvsp[-1].baseClassList); 
           yyval.tfClass->super->init_fields(); ;}
    break;

  case 11:
#line 140 "agrampar.y"
    { (yyval.tfClass=yyvsp[0].tfClass)->super->name = unbox(yyvsp[-4].str);
           yyval.tfClass->super->args.steal(yyvsp[-3].ctorArgList);
           yyval.tfClass->super->lastArgs.steal(yyvsp[-2].ctorArgList);
           yyval.tfClass->super->bases.steal(yyvsp[-1].baseClassList); 
           yyval.tfClass->super->init_fields(); ;}
    break;

  case 12:
#line 148 "agrampar.y"
    {;}
    break;

  case 13:
#line 149 "agrampar.y"
    {;}
    break;

  case 14:
#line 162 "agrampar.y"
    { yyval.tfClass=yyvsp[-1].tfClass; ;}
    break;

  case 15:
#line 164 "agrampar.y"
    { yyval.tfClass = new TF_class(new ASTClass("(placeholder)", NULL, NULL, NULL, NULL), NULL); ;}
    break;

  case 16:
#line 171 "agrampar.y"
    { yyval.tfClass = new TF_class(new ASTClass("(placeholder)", NULL, NULL, NULL, NULL), NULL); ;}
    break;

  case 17:
#line 173 "agrampar.y"
    { ASTClass * cl = new ASTClass(unbox(yyvsp[-3].str), yyvsp[-2].ctorArgList, NULL, yyvsp[-1].baseClassList, NULL);
        // unnecessary call to init_fields, since decls is empty, 
        // but who knows, maybe this will change some day ...
        cl->init_fields();
	(yyval.tfClass=yyvsp[-5].tfClass)->ctors.append(cl); 
      ;}
    break;

  case 18:
#line 180 "agrampar.y"
    { ASTClass * cl = new ASTClass(unbox(yyvsp[-5].str), yyvsp[-4].ctorArgList, NULL, yyvsp[-3].baseClassList, yyvsp[-1].userDeclList);
        cl->init_fields();
	(yyval.tfClass=yyvsp[-7].tfClass)->ctors.append(cl); 
      ;}
    break;

  case 19:
#line 185 "agrampar.y"
    { (yyval.tfClass=yyvsp[-1].tfClass)->super->decls.append(yyvsp[0].annotation); ;}
    break;

  case 20:
#line 192 "agrampar.y"
    { yyval.ctorArgList = new ASTList<FieldOrCtorArg>; ;}
    break;

  case 21:
#line 194 "agrampar.y"
    { yyval.ctorArgList = yyvsp[0].ctorArgList; ;}
    break;

  case 22:
#line 200 "agrampar.y"
    { yyval.ctorArgList = new ASTList<FieldOrCtorArg>; ;}
    break;

  case 23:
#line 202 "agrampar.y"
    { yyval.ctorArgList = yyvsp[-1].ctorArgList; ;}
    break;

  case 24:
#line 207 "agrampar.y"
    { yyval.ctorArgList = new ASTList<FieldOrCtorArg>;
                 {
                   string tmp = unbox(yyvsp[0].str);
                   yyval.ctorArgList->append(parseCtorArg(tmp));
                 }
               ;}
    break;

  case 25:
#line 214 "agrampar.y"
    { (yyval.ctorArgList=yyvsp[-2].ctorArgList)->append(parseCtorArg(unbox(yyvsp[0].str))); ;}
    break;

  case 26:
#line 219 "agrampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 27:
#line 221 "agrampar.y"
    { yyval.str = appendStr(yyvsp[-1].str, yyvsp[0].str); ;}
    break;

  case 28:
#line 226 "agrampar.y"
    { yyval.str = appendStr(yyvsp[0].str, box(" ")); ;}
    break;

  case 29:
#line 227 "agrampar.y"
    { yyval.str = appendStr(yyvsp[0].str, box(" ")); ;}
    break;

  case 30:
#line 228 "agrampar.y"
    { yyval.str = appendStr(box("<"), appendStr(yyvsp[-1].str, box(">"))); ;}
    break;

  case 31:
#line 229 "agrampar.y"
    { yyval.str = box("*"); ;}
    break;

  case 32:
#line 230 "agrampar.y"
    { yyval.str = box("&"); ;}
    break;

  case 33:
#line 231 "agrampar.y"
    { yyval.str = box("="); ;}
    break;

  case 34:
#line 232 "agrampar.y"
    { yyval.str = box("class "); ;}
    break;

  case 35:
#line 237 "agrampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 36:
#line 239 "agrampar.y"
    { yyval.str = appendStr(yyvsp[-2].str, appendStr(box(","), yyvsp[0].str)); ;}
    break;

  case 37:
#line 245 "agrampar.y"
    { yyval.userDeclList = new ASTList<Annotation>; ;}
    break;

  case 38:
#line 247 "agrampar.y"
    { (yyval.userDeclList=yyvsp[-1].userDeclList)->append(yyvsp[0].annotation); ;}
    break;

  case 39:
#line 253 "agrampar.y"
    { yyval.annotation = new UserDecl(yyvsp[-1].accessMod, unbox(yyvsp[0].str), ""); ;}
    break;

  case 40:
#line 255 "agrampar.y"
    { yyval.annotation = new UserDecl(yyvsp[-4].accessMod, unbox(yyvsp[-3].str), unbox(yyvsp[-1].str)); ;}
    break;

  case 41:
#line 257 "agrampar.y"
    { yyval.annotation = yyvsp[0].customCode; ;}
    break;

  case 42:
#line 263 "agrampar.y"
    { yyval.customCode = new CustomCode(unbox(yyvsp[-1].str), unbox(yyvsp[0].str)); ;}
    break;

  case 43:
#line 269 "agrampar.y"
    { yyval.str = yyvsp[-1].str; ;}
    break;

  case 44:
#line 271 "agrampar.y"
    { yyval.str = yyvsp[-1].str; ;}
    break;

  case 45:
#line 276 "agrampar.y"
    { yyval.accessCtl = AC_PUBLIC; ;}
    break;

  case 46:
#line 277 "agrampar.y"
    { yyval.accessCtl = AC_PRIVATE; ;}
    break;

  case 47:
#line 278 "agrampar.y"
    { yyval.accessCtl = AC_PROTECTED; ;}
    break;

  case 48:
#line 279 "agrampar.y"
    { yyval.accessCtl = AC_CTOR; ;}
    break;

  case 49:
#line 280 "agrampar.y"
    { yyval.accessCtl = AC_DTOR; ;}
    break;

  case 50:
#line 281 "agrampar.y"
    { yyval.accessCtl = AC_PUREVIRT; ;}
    break;

  case 51:
#line 286 "agrampar.y"
    { yyval.accessMod = new AccessMod(yyvsp[0].accessCtl, NULL); ;}
    break;

  case 52:
#line 288 "agrampar.y"
    { yyval.accessMod = new AccessMod(yyvsp[-3].accessCtl, yyvsp[-1].stringList); ;}
    break;

  case 53:
#line 293 "agrampar.y"
    { yyval.stringList = new ASTList<string>(yyvsp[0].str); ;}
    break;

  case 54:
#line 295 "agrampar.y"
    { (yyval.stringList=yyvsp[-2].stringList)->append(yyvsp[0].str); ;}
    break;

  case 55:
#line 300 "agrampar.y"
    { yyval.verbatim = new TF_verbatim(unbox(yyvsp[0].str)); ;}
    break;

  case 56:
#line 302 "agrampar.y"
    { yyval.verbatim = new TF_impl_verbatim(unbox(yyvsp[0].str)); ;}
    break;

  case 57:
#line 304 "agrampar.y"
    { yyval.verbatim = new TF_ocaml_type_verbatim(unbox(yyvsp[0].str)); ;}
    break;

  case 58:
#line 306 "agrampar.y"
    { yyval.verbatim = new TF_xml_verbatim(unbox(yyvsp[0].str)); ;}
    break;

  case 59:
#line 311 "agrampar.y"
    { yyval.tfOption = new TF_option(unbox(yyvsp[-2].str), yyvsp[-1].stringList); ;}
    break;

  case 60:
#line 316 "agrampar.y"
    { yyval.stringList = new ASTList<string>; ;}
    break;

  case 61:
#line 318 "agrampar.y"
    { (yyval.stringList=yyvsp[-1].stringList)->append(yyvsp[0].str); ;}
    break;

  case 62:
#line 323 "agrampar.y"
    { yyval.tfEnum = new TF_enum(unbox(yyvsp[-3].str), yyvsp[-1].enumeratorList); ;}
    break;

  case 63:
#line 325 "agrampar.y"
    { yyval.tfEnum = new TF_enum(unbox(yyvsp[-4].str), yyvsp[-2].enumeratorList); ;}
    break;

  case 64:
#line 330 "agrampar.y"
    { yyval.enumeratorList = new ASTList<string>(yyvsp[0].enumerator); ;}
    break;

  case 65:
#line 332 "agrampar.y"
    { (yyval.enumeratorList=yyvsp[-2].enumeratorList)->append(yyvsp[0].enumerator); ;}
    break;

  case 66:
#line 337 "agrampar.y"
    { yyval.enumerator = yyvsp[0].str; ;}
    break;

  case 67:
#line 342 "agrampar.y"
    { yyval.baseClassList = new ASTList<BaseClass>; ;}
    break;

  case 68:
#line 344 "agrampar.y"
    { yyval.baseClassList = yyvsp[0].baseClassList; ;}
    break;

  case 69:
#line 349 "agrampar.y"
    { yyval.baseClassList = new ASTList<BaseClass>(yyvsp[0].baseClass); ;}
    break;

  case 70:
#line 351 "agrampar.y"
    { (yyval.baseClassList=yyvsp[-2].baseClassList)->append(yyvsp[0].baseClass); ;}
    break;

  case 71:
#line 356 "agrampar.y"
    { yyval.accessCtl = AC_PUBLIC; ;}
    break;

  case 72:
#line 357 "agrampar.y"
    { yyval.accessCtl = AC_PRIVATE; ;}
    break;

  case 73:
#line 358 "agrampar.y"
    { yyval.accessCtl = AC_PROTECTED; ;}
    break;

  case 74:
#line 363 "agrampar.y"
    { yyval.baseClass = new BaseClass(yyvsp[-1].accessCtl, unbox(yyvsp[0].str)); ;}
    break;


    }

/* Line 1010 of yacc.c.  */
#line 1572 "agrampar.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 366 "agrampar.y"


/* ----------------- extra C code ------------------- */


