/* A Bison parser, made by GNU Bison 1.875b.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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
     TOK_CTOR = 280,
     TOK_DTOR = 281,
     TOK_PURE_VIRTUAL = 282,
     TOK_CUSTOM = 283,
     TOK_OPTION = 284,
     TOK_NEW = 285,
     TOK_ENUM = 286
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
#define TOK_CTOR 280
#define TOK_DTOR 281
#define TOK_PURE_VIRTUAL 282
#define TOK_CUSTOM 283
#define TOK_OPTION 284
#define TOK_NEW 285
#define TOK_ENUM 286




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
#line 68 "agrampar.y"
typedef union YYSTYPE {
  ASTSpecFile *file;
  ASTList<ToplevelForm> *formList;
  TF_class *tfClass;
  ASTList<CtorArg> *ctorArgList;
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
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 175 "agrampar.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 187 "agrampar.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
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
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
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
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   113

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  32
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  29
/* YYNRULES -- Number of rules. */
#define YYNRULES  70
/* YYNRULES -- Number of states. */
#define YYNSTATES  112

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   286

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
      25,    26,    27,    28,    29,    30,    31
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    15,    18,    21,
      28,    36,    37,    39,    43,    45,    46,    53,    62,    65,
      66,    68,    71,    75,    77,    81,    83,    86,    88,    90,
      94,    96,    98,   100,   102,   104,   108,   109,   112,   115,
     121,   125,   128,   132,   134,   136,   138,   140,   142,   144,
     146,   151,   153,   157,   160,   163,   168,   169,   172,   178,
     185,   187,   191,   193,   194,   197,   199,   203,   205,   207,
     209
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      33,     0,    -1,    34,    -1,    -1,    34,    35,    -1,    34,
      51,    -1,    34,    52,    -1,    34,    54,    -1,    34,     8,
      -1,    36,    19,     3,    39,    57,    37,    -1,    36,    19,
       3,    40,    40,    57,    37,    -1,    -1,    30,    -1,     6,
      38,     7,    -1,     8,    -1,    -1,    38,     9,     3,    39,
      57,     8,    -1,    38,     9,     3,    39,    57,     6,    45,
       7,    -1,    38,    46,    -1,    -1,    40,    -1,    10,    11,
      -1,    10,    41,    11,    -1,    42,    -1,    41,    16,    42,
      -1,    43,    -1,    42,    43,    -1,     3,    -1,     4,    -1,
      12,    44,    13,    -1,    14,    -1,    15,    -1,    17,    -1,
      19,    -1,    42,    -1,    42,    16,    44,    -1,    -1,    45,
      46,    -1,    49,    47,    -1,    49,     5,    17,     5,     8,
      -1,    28,     3,    47,    -1,     5,     8,    -1,     6,     5,
       7,    -1,    20,    -1,    21,    -1,    22,    -1,    25,    -1,
      26,    -1,    27,    -1,    48,    -1,    48,    10,    50,    11,
      -1,     3,    -1,    50,    16,     3,    -1,    23,    47,    -1,
      24,    47,    -1,    29,     3,    53,     8,    -1,    -1,    53,
       3,    -1,    31,     3,     6,    55,     7,    -1,    31,     3,
       6,    55,    16,     7,    -1,    56,    -1,    55,    16,    56,
      -1,     3,    -1,    -1,    18,    58,    -1,    60,    -1,    58,
      16,    60,    -1,    20,    -1,    21,    -1,    22,    -1,    59,
       3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   113,   113,   119,   120,   121,   122,   123,   124,   129,
     133,   141,   142,   154,   156,   164,   165,   167,   169,   177,
     178,   184,   186,   191,   198,   203,   205,   211,   212,   213,
     214,   215,   216,   217,   221,   223,   230,   231,   237,   239,
     241,   247,   249,   255,   256,   257,   258,   259,   260,   264,
     266,   271,   273,   278,   280,   285,   291,   292,   297,   299,
     304,   306,   311,   317,   318,   323,   325,   331,   332,   333,
     337
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
  "\"verbatim\"", "\"impl_verbatim\"", "\"ctor\"", "\"dtor\"", 
  "\"pure_virtual\"", "\"custom\"", "\"option\"", "\"new\"", "\"enum\"", 
  "$accept", "StartSymbol", "Input", "Class", "NewOpt", "ClassBody", 
  "ClassMembersOpt", "CtorArgsOpt", "CtorArgs", "CtorArgList", "Arg", 
  "ArgWord", "ArgList", "CtorMembersOpt", "Annotation", "Embedded", 
  "Public", "AccessMod", "StringList", "Verbatim", "Option", "OptionArgs", 
  "Enum", "EnumeratorSeq", "Enumerator", "BaseClassesOpt", "BaseClassSeq", 
  "BaseAccess", "BaseClass", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    32,    33,    34,    34,    34,    34,    34,    34,    35,
      35,    36,    36,    37,    37,    38,    38,    38,    38,    39,
      39,    40,    40,    41,    41,    42,    42,    43,    43,    43,
      43,    43,    43,    43,    44,    44,    45,    45,    46,    46,
      46,    47,    47,    48,    48,    48,    48,    48,    48,    49,
      49,    50,    50,    51,    51,    52,    53,    53,    54,    54,
      55,    55,    56,    57,    57,    58,    58,    59,    59,    59,
      60
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     2,     2,     2,     6,
       7,     0,     1,     3,     1,     0,     6,     8,     2,     0,
       1,     2,     3,     1,     3,     1,     2,     1,     1,     3,
       1,     1,     1,     1,     1,     3,     0,     2,     2,     5,
       3,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     3,     2,     2,     4,     0,     2,     5,     6,
       1,     3,     1,     0,     2,     1,     3,     1,     1,     1,
       2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     2,     1,     8,     0,     0,     0,    12,     0,
       4,     0,     5,     6,     7,     0,     0,    53,    54,    56,
       0,     0,    41,     0,     0,     0,    19,    42,    57,    55,
      62,     0,    60,     0,    63,    20,    58,     0,    27,    28,
      21,     0,    30,    31,    32,    33,     0,    23,    25,     0,
       0,    63,    59,    61,    34,     0,    22,     0,    26,    67,
      68,    69,    64,     0,    65,    15,    14,     9,     0,     0,
      29,    24,     0,    70,     0,    10,    35,    66,    13,     0,
      43,    44,    45,    46,    47,    48,     0,    18,    49,     0,
      19,     0,     0,     0,    38,    63,    20,    40,    51,     0,
       0,     0,    50,     0,     0,    36,    16,    52,    39,     0,
      17,    37
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     2,    10,    11,    67,    74,    34,    35,    46,
      54,    48,    55,   109,    87,    17,    88,    89,    99,    12,
      13,    24,    14,    31,    32,    50,    62,    63,    64
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -50
static const yysigned_char yypact[] =
{
     -50,     9,     0,   -50,   -50,    49,    49,    12,   -50,    23,
     -50,    24,   -50,   -50,   -50,    34,    27,   -50,   -50,   -50,
      64,    70,   -50,    77,     3,    88,    82,   -50,   -50,   -50,
     -50,    -2,   -50,    57,    75,    82,   -50,    18,   -50,   -50,
     -50,    63,   -50,   -50,   -50,   -50,     1,    63,   -50,    66,
      45,    75,   -50,   -50,    33,    81,   -50,    63,   -50,   -50,
     -50,   -50,    79,    93,   -50,   -50,   -50,   -50,    45,    63,
     -50,    63,    66,   -50,    13,   -50,   -50,   -50,   -50,    94,
     -50,   -50,   -50,   -50,   -50,   -50,    95,   -50,    89,    84,
      82,    49,    97,    -1,   -50,    75,   -50,   -50,   -50,     2,
      96,    73,   -50,    99,    98,   -50,   -50,   -50,   -50,    37,
     -50,   -50
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -50,   -50,   -50,   -50,   -50,    35,   -50,    14,   -34,   -50,
     -29,   -44,    36,   -50,     4,    -6,   -50,   -50,   -50,   -50,
     -50,   -50,   -50,   -50,    71,   -49,   -50,   -50,    38
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -12
static const yysigned_char yytable[] =
{
      18,    51,    68,    58,    47,    36,    28,    22,     4,     3,
      58,    29,    56,   102,    37,    19,   100,    57,   103,   -11,
      78,    30,    79,     5,     6,    52,    20,    58,    71,     7,
       8,     9,    23,    80,    81,    82,    38,    39,    83,    84,
      85,    86,    22,    21,   110,    41,   101,    42,    43,    69,
      44,    65,    45,    66,    15,    16,    96,    80,    81,    82,
      38,    39,    83,    84,    85,    86,    38,    39,    40,    41,
      25,    42,    43,    26,    44,    41,    45,    42,    43,   105,
      44,   106,    45,    94,    27,    97,    59,    60,    61,    93,
      16,    30,    33,    49,    70,    72,    73,    90,    91,    92,
      98,   104,   107,    75,    95,    76,   108,     0,    53,     0,
      77,     0,     0,   111
};

static const yysigned_char yycheck[] =
{
       6,    35,    51,    47,    33,     7,     3,     8,     8,     0,
      54,     8,    11,    11,    16,     3,    17,    16,    16,    19,
       7,     3,     9,    23,    24,     7,     3,    71,    57,    29,
      30,    31,     5,    20,    21,    22,     3,     4,    25,    26,
      27,    28,     8,    19,     7,    12,    95,    14,    15,    16,
      17,     6,    19,     8,     5,     6,    90,    20,    21,    22,
       3,     4,    25,    26,    27,    28,     3,     4,    11,    12,
       6,    14,    15,     3,    17,    12,    19,    14,    15,     6,
      17,     8,    19,    89,     7,    91,    20,    21,    22,     5,
       6,     3,    10,    18,    13,    16,     3,     3,     3,    10,
       3,     5,     3,    68,    90,    69,     8,    -1,    37,    -1,
      72,    -1,    -1,   109
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    33,    34,     0,     8,    23,    24,    29,    30,    31,
      35,    36,    51,    52,    54,     5,     6,    47,    47,     3,
       3,    19,     8,     5,    53,     6,     3,     7,     3,     8,
       3,    55,    56,    10,    39,    40,     7,    16,     3,     4,
      11,    12,    14,    15,    17,    19,    41,    42,    43,    18,
      57,    40,     7,    56,    42,    44,    11,    16,    43,    20,
      21,    22,    58,    59,    60,     6,     8,    37,    57,    16,
      13,    42,    16,     3,    38,    37,    44,    60,     7,     9,
      20,    21,    22,    25,    26,    27,    28,    46,    48,    49,
       3,     3,    10,     5,    47,    39,    40,    47,     3,    50,
      17,    57,    11,    16,     5,     6,     8,     3,     8,    45,
       7,    46
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
#define YYERROR		goto yyerrlab1


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
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
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

#if YYMAXDEPTH == 0
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
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

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
	short *yyss1 = yyss;


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
	short *yyss1 = yyss;
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
#line 114 "agrampar.y"
    { yyval.file = *((ASTSpecFile**)parseParam) = new ASTSpecFile(yyvsp[0].formList); ;}
    break;

  case 3:
#line 119 "agrampar.y"
    { yyval.formList = new ASTList<ToplevelForm>; ;}
    break;

  case 4:
#line 120 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].tfClass); ;}
    break;

  case 5:
#line 121 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].verbatim); ;}
    break;

  case 6:
#line 122 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].tfOption); ;}
    break;

  case 7:
#line 123 "agrampar.y"
    { (yyval.formList=yyvsp[-1].formList)->append(yyvsp[0].tfEnum); ;}
    break;

  case 8:
#line 124 "agrampar.y"
    { yyval.formList=yyvsp[-1].formList; ;}
    break;

  case 9:
#line 130 "agrampar.y"
    { (yyval.tfClass=yyvsp[0].tfClass)->super->name = unbox(yyvsp[-3].str); 
           yyval.tfClass->super->args.steal(yyvsp[-2].ctorArgList); 
           yyval.tfClass->super->bases.steal(yyvsp[-1].baseClassList); ;}
    break;

  case 10:
#line 134 "agrampar.y"
    { (yyval.tfClass=yyvsp[0].tfClass)->super->name = unbox(yyvsp[-4].str);
           yyval.tfClass->super->args.steal(yyvsp[-3].ctorArgList);
           yyval.tfClass->super->lastArgs.steal(yyvsp[-2].ctorArgList);
           yyval.tfClass->super->bases.steal(yyvsp[-1].baseClassList); ;}
    break;

  case 11:
#line 141 "agrampar.y"
    {;}
    break;

  case 12:
#line 142 "agrampar.y"
    {;}
    break;

  case 13:
#line 155 "agrampar.y"
    { yyval.tfClass=yyvsp[-1].tfClass; ;}
    break;

  case 14:
#line 157 "agrampar.y"
    { yyval.tfClass = new TF_class(new ASTClass("(placeholder)", NULL, NULL, NULL, NULL), NULL); ;}
    break;

  case 15:
#line 164 "agrampar.y"
    { yyval.tfClass = new TF_class(new ASTClass("(placeholder)", NULL, NULL, NULL, NULL), NULL); ;}
    break;

  case 16:
#line 166 "agrampar.y"
    { (yyval.tfClass=yyvsp[-5].tfClass)->ctors.append(new ASTClass(unbox(yyvsp[-3].str), yyvsp[-2].ctorArgList, NULL, yyvsp[-1].baseClassList, NULL)); ;}
    break;

  case 17:
#line 168 "agrampar.y"
    { (yyval.tfClass=yyvsp[-7].tfClass)->ctors.append(new ASTClass(unbox(yyvsp[-5].str), yyvsp[-4].ctorArgList, NULL, yyvsp[-3].baseClassList, yyvsp[-1].userDeclList)); ;}
    break;

  case 18:
#line 170 "agrampar.y"
    { (yyval.tfClass=yyvsp[-1].tfClass)->super->decls.append(yyvsp[0].annotation); ;}
    break;

  case 19:
#line 177 "agrampar.y"
    { yyval.ctorArgList = new ASTList<CtorArg>; ;}
    break;

  case 20:
#line 179 "agrampar.y"
    { yyval.ctorArgList = yyvsp[0].ctorArgList; ;}
    break;

  case 21:
#line 185 "agrampar.y"
    { yyval.ctorArgList = new ASTList<CtorArg>; ;}
    break;

  case 22:
#line 187 "agrampar.y"
    { yyval.ctorArgList = yyvsp[-1].ctorArgList; ;}
    break;

  case 23:
#line 192 "agrampar.y"
    { yyval.ctorArgList = new ASTList<CtorArg>;
                 {
                   string tmp = unbox(yyvsp[0].str);
                   yyval.ctorArgList->append(parseCtorArg(tmp));
                 }
               ;}
    break;

  case 24:
#line 199 "agrampar.y"
    { (yyval.ctorArgList=yyvsp[-2].ctorArgList)->append(parseCtorArg(unbox(yyvsp[0].str))); ;}
    break;

  case 25:
#line 204 "agrampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 26:
#line 206 "agrampar.y"
    { yyval.str = appendStr(yyvsp[-1].str, yyvsp[0].str); ;}
    break;

  case 27:
#line 211 "agrampar.y"
    { yyval.str = appendStr(yyvsp[0].str, box(" ")); ;}
    break;

  case 28:
#line 212 "agrampar.y"
    { yyval.str = appendStr(yyvsp[0].str, box(" ")); ;}
    break;

  case 29:
#line 213 "agrampar.y"
    { yyval.str = appendStr(box("<"), appendStr(yyvsp[-1].str, box(">"))); ;}
    break;

  case 30:
#line 214 "agrampar.y"
    { yyval.str = box("*"); ;}
    break;

  case 31:
#line 215 "agrampar.y"
    { yyval.str = box("&"); ;}
    break;

  case 32:
#line 216 "agrampar.y"
    { yyval.str = box("="); ;}
    break;

  case 33:
#line 217 "agrampar.y"
    { yyval.str = box("class "); ;}
    break;

  case 34:
#line 222 "agrampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 35:
#line 224 "agrampar.y"
    { yyval.str = appendStr(yyvsp[-2].str, appendStr(box(","), yyvsp[0].str)); ;}
    break;

  case 36:
#line 230 "agrampar.y"
    { yyval.userDeclList = new ASTList<Annotation>; ;}
    break;

  case 37:
#line 232 "agrampar.y"
    { (yyval.userDeclList=yyvsp[-1].userDeclList)->append(yyvsp[0].annotation); ;}
    break;

  case 38:
#line 238 "agrampar.y"
    { yyval.annotation = new UserDecl(yyvsp[-1].accessMod, unbox(yyvsp[0].str), ""); ;}
    break;

  case 39:
#line 240 "agrampar.y"
    { yyval.annotation = new UserDecl(yyvsp[-4].accessMod, unbox(yyvsp[-3].str), unbox(yyvsp[-1].str)); ;}
    break;

  case 40:
#line 242 "agrampar.y"
    { yyval.annotation = new CustomCode(unbox(yyvsp[-1].str), unbox(yyvsp[0].str)); ;}
    break;

  case 41:
#line 248 "agrampar.y"
    { yyval.str = yyvsp[-1].str; ;}
    break;

  case 42:
#line 250 "agrampar.y"
    { yyval.str = yyvsp[-1].str; ;}
    break;

  case 43:
#line 255 "agrampar.y"
    { yyval.accessCtl = AC_PUBLIC; ;}
    break;

  case 44:
#line 256 "agrampar.y"
    { yyval.accessCtl = AC_PRIVATE; ;}
    break;

  case 45:
#line 257 "agrampar.y"
    { yyval.accessCtl = AC_PROTECTED; ;}
    break;

  case 46:
#line 258 "agrampar.y"
    { yyval.accessCtl = AC_CTOR; ;}
    break;

  case 47:
#line 259 "agrampar.y"
    { yyval.accessCtl = AC_DTOR; ;}
    break;

  case 48:
#line 260 "agrampar.y"
    { yyval.accessCtl = AC_PUREVIRT; ;}
    break;

  case 49:
#line 265 "agrampar.y"
    { yyval.accessMod = new AccessMod(yyvsp[0].accessCtl, NULL); ;}
    break;

  case 50:
#line 267 "agrampar.y"
    { yyval.accessMod = new AccessMod(yyvsp[-3].accessCtl, yyvsp[-1].stringList); ;}
    break;

  case 51:
#line 272 "agrampar.y"
    { yyval.stringList = new ASTList<string>(yyvsp[0].str); ;}
    break;

  case 52:
#line 274 "agrampar.y"
    { (yyval.stringList=yyvsp[-2].stringList)->append(yyvsp[0].str); ;}
    break;

  case 53:
#line 279 "agrampar.y"
    { yyval.verbatim = new TF_verbatim(unbox(yyvsp[0].str)); ;}
    break;

  case 54:
#line 281 "agrampar.y"
    { yyval.verbatim = new TF_impl_verbatim(unbox(yyvsp[0].str)); ;}
    break;

  case 55:
#line 286 "agrampar.y"
    { yyval.tfOption = new TF_option(unbox(yyvsp[-2].str), yyvsp[-1].stringList); ;}
    break;

  case 56:
#line 291 "agrampar.y"
    { yyval.stringList = new ASTList<string>; ;}
    break;

  case 57:
#line 293 "agrampar.y"
    { (yyval.stringList=yyvsp[-1].stringList)->append(yyvsp[0].str); ;}
    break;

  case 58:
#line 298 "agrampar.y"
    { yyval.tfEnum = new TF_enum(unbox(yyvsp[-3].str), yyvsp[-1].enumeratorList); ;}
    break;

  case 59:
#line 300 "agrampar.y"
    { yyval.tfEnum = new TF_enum(unbox(yyvsp[-4].str), yyvsp[-2].enumeratorList); ;}
    break;

  case 60:
#line 305 "agrampar.y"
    { yyval.enumeratorList = new ASTList<string>(yyvsp[0].enumerator); ;}
    break;

  case 61:
#line 307 "agrampar.y"
    { (yyval.enumeratorList=yyvsp[-2].enumeratorList)->append(yyvsp[0].enumerator); ;}
    break;

  case 62:
#line 312 "agrampar.y"
    { yyval.enumerator = yyvsp[0].str; ;}
    break;

  case 63:
#line 317 "agrampar.y"
    { yyval.baseClassList = new ASTList<BaseClass>; ;}
    break;

  case 64:
#line 319 "agrampar.y"
    { yyval.baseClassList = yyvsp[0].baseClassList; ;}
    break;

  case 65:
#line 324 "agrampar.y"
    { yyval.baseClassList = new ASTList<BaseClass>(yyvsp[0].baseClass); ;}
    break;

  case 66:
#line 326 "agrampar.y"
    { (yyval.baseClassList=yyvsp[-2].baseClassList)->append(yyvsp[0].baseClass); ;}
    break;

  case 67:
#line 331 "agrampar.y"
    { yyval.accessCtl = AC_PUBLIC; ;}
    break;

  case 68:
#line 332 "agrampar.y"
    { yyval.accessCtl = AC_PRIVATE; ;}
    break;

  case 69:
#line 333 "agrampar.y"
    { yyval.accessCtl = AC_PROTECTED; ;}
    break;

  case 70:
#line 338 "agrampar.y"
    { yyval.baseClass = new BaseClass(yyvsp[-1].accessCtl, unbox(yyvsp[0].str)); ;}
    break;


    }

/* Line 999 of yacc.c.  */
#line 1527 "agrampar.tab.c"

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

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
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
      yyvsp--;
      yystate = *--yyssp;

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


#line 341 "agrampar.y"


/* ----------------- extra C code ------------------- */


