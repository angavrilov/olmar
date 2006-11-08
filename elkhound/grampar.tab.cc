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
     TOK_INTEGER = 258,
     TOK_NAME = 259,
     TOK_STRING = 260,
     TOK_LIT_CODE = 261,
     TOK_LBRACE = 262,
     TOK_RBRACE = 263,
     TOK_COLON = 264,
     TOK_SEMICOLON = 265,
     TOK_ARROW = 266,
     TOK_LPAREN = 267,
     TOK_RPAREN = 268,
     TOK_COMMA = 269,
     TOK_TERMINALS = 270,
     TOK_TOKEN = 271,
     TOK_NONTERM = 272,
     TOK_FUN = 273,
     TOK_VERBATIM = 274,
     TOK_IMPL_VERBATIM = 275,
     TOK_PRECEDENCE = 276,
     TOK_OPTION = 277,
     TOK_EXPECT = 278,
     TOK_CONTEXT_CLASS = 279,
     TOK_SUBSETS = 280,
     TOK_DELETE = 281,
     TOK_REPLACE = 282,
     TOK_FORBID_NEXT = 283
   };
#endif
#define TOK_INTEGER 258
#define TOK_NAME 259
#define TOK_STRING 260
#define TOK_LIT_CODE 261
#define TOK_LBRACE 262
#define TOK_RBRACE 263
#define TOK_COLON 264
#define TOK_SEMICOLON 265
#define TOK_ARROW 266
#define TOK_LPAREN 267
#define TOK_RPAREN 268
#define TOK_COMMA 269
#define TOK_TERMINALS 270
#define TOK_TOKEN 271
#define TOK_NONTERM 272
#define TOK_FUN 273
#define TOK_VERBATIM 274
#define TOK_IMPL_VERBATIM 275
#define TOK_PRECEDENCE 276
#define TOK_OPTION 277
#define TOK_EXPECT 278
#define TOK_CONTEXT_CLASS 279
#define TOK_SUBSETS 280
#define TOK_DELETE 281
#define TOK_REPLACE 282
#define TOK_FORBID_NEXT 283




/* Copy the first part of user declarations.  */
#line 6 "grampar.y"


#include "grampar.h"        // yylex, etc.
#include "gramast.ast.gen.h"// grammar syntax AST definition
#include "gramlex.h"        // GrammarLexer
#include "owner.h"          // Owner

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

// enable debugging the parser
#ifndef NDEBUG
  #define YYDEBUG 1
#endif

// name of extra parameter to yylex
#define YYLEX_PARAM parseParam

// make it call my yylex
#define yylex(lv, param) grampar_yylex(lv, param)

// Bison calls yyerror(msg) on error; we need the extra
// parameter too, so the macro shoehorns it in there
#define yyerror(msg) grampar_yyerror(msg, YYPARSE_PARAM)

// rename the externally-visible parsing routine to make it
// specific to this instance, so multiple bison-generated
// parsers can coexist
#define yyparse grampar_yyparse


// grab the parameter
#define PARAM ((ParseParams*)parseParam)

// return a locstring for 'str' with no location information
#define noloc(str)                                                    \
  new LocString(SL_UNKNOWN,      /* unknown location */               \
                PARAM->lexer.strtable.add(str))
                
// locstring for NULL, with no location
#define nolocNULL()                                                   \
  new LocString(SL_UNKNOWN, NULL)

// return a locstring with same location info as something else
// (passed as a pointer to a SourceLocation)
#define sameloc(otherLoc, str)                                        \
  new LocString(otherLoc->loc, PARAM->lexer.strtable.add(str))

// interpret the word into an associativity kind specification
AssocKind whichKind(LocString * /*owner*/ kind);



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
#line 105 "grampar.y"
typedef union YYSTYPE {
  int num;
  LocString *str;
  SourceLoc loc;

  ASTList<TopForm> *topFormList;
  TopForm *topForm;

  ASTList<TermDecl> *termDecls;
  TermDecl *termDecl;
  ASTList<TermType> *termTypes;
  TermType *termType;
  ASTList<PrecSpec> *precSpecs;

  ASTList<SpecFunc> *specFuncs;
  SpecFunc *specFunc;
  ASTList<LocString> *stringList;

  ASTList<ProdDecl> *prodDecls;
  ProdDecl *prodDecl;
  ASTList<RHSElt> *rhsList;
  RHSElt *rhsElt;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 209 "grampar.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 221 "grampar.tab.c"

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
#define YYLAST   112

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  29
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  28
/* YYNRULES -- Number of rules. */
#define YYNRULES  59
/* YYNRULES -- Number of states. */
#define YYNSTATES  106

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

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
      25,    26,    27,    28
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    11,    13,    15,    17,
      19,    23,    26,    29,    33,    38,    45,    46,    49,    54,
      60,    62,    63,    64,    67,    72,    79,    80,    85,    86,
      92,    93,    96,    98,   100,   101,   104,   111,   112,   114,
     116,   120,   125,   134,   135,   138,   142,   147,   152,   154,
     156,   157,   160,   162,   166,   168,   172,   177,   182,   183
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      30,     0,    -1,    31,    -1,    -1,    31,    32,    -1,    33,
      -1,    34,    -1,    35,    -1,    36,    -1,    50,    -1,    24,
       6,    10,    -1,    19,     6,    -1,    20,     6,    -1,    22,
       4,    10,    -1,    22,     4,     3,    10,    -1,    15,     7,
      37,    40,    42,     8,    -1,    -1,    37,    38,    -1,     3,
       9,     4,    10,    -1,     3,     9,     4,     5,    10,    -1,
       6,    -1,    -1,    -1,    40,    41,    -1,    16,    39,     4,
      10,    -1,    16,    39,     4,     7,    46,     8,    -1,    -1,
      21,     7,    43,     8,    -1,    -1,    43,     4,     3,    44,
      10,    -1,    -1,    44,    45,    -1,     4,    -1,     5,    -1,
      -1,    46,    47,    -1,    18,     4,    12,    48,    13,     6,
      -1,    -1,    49,    -1,     4,    -1,    49,    14,     4,    -1,
      17,    39,     4,    52,    -1,    17,    39,     4,     7,    46,
      51,    56,     8,    -1,    -1,    51,    52,    -1,    11,    54,
      53,    -1,    27,    11,    54,    53,    -1,    26,    11,    54,
      10,    -1,     6,    -1,    10,    -1,    -1,    54,    55,    -1,
       4,    -1,     4,     9,     4,    -1,     5,    -1,     4,     9,
       5,    -1,    21,    12,    45,    13,    -1,    28,    12,    45,
      13,    -1,    -1,    25,    49,    10,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   162,   162,   167,   168,   172,   173,   174,   175,   176,
     180,   185,   186,   191,   192,   203,   208,   209,   217,   219,
     224,   225,   229,   230,   234,   236,   241,   242,   247,   248,
     253,   254,   258,   259,   265,   266,   270,   275,   276,   280,
     281,   292,   295,   300,   301,   305,   306,   307,   311,   312,
     316,   317,   326,   327,   328,   329,   330,   331,   335,   336
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_INTEGER", "TOK_NAME", "TOK_STRING",
  "TOK_LIT_CODE", "\"{\"", "\"}\"", "\":\"", "\";\"", "\"->\"", "\"(\"",
  "\")\"", "\",\"", "\"terminals\"", "\"token\"", "\"nonterm\"", "\"fun\"",
  "\"verbatim\"", "\"impl_verbatim\"", "\"precedence\"", "\"option\"",
  "\"expect\"", "\"context_class\"", "\"subsets\"", "\"delete\"",
  "\"replace\"", "\"forbid_next\"", "$accept", "StartSymbol",
  "TopFormList", "TopForm", "ContextClass", "Verbatim", "Option",
  "Terminals", "TermDecls", "TerminalDecl", "Type", "TermTypes",
  "TermType", "Precedence", "PrecSpecs", "NameOrStringList",
  "NameOrString", "SpecFuncs", "SpecFunc", "FormalsOpt", "Formals",
  "Nonterminal", "Productions", "Production", "Action", "RHS", "RHSElt",
  "Subsets", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    29,    30,    31,    31,    32,    32,    32,    32,    32,
      33,    34,    34,    35,    35,    36,    37,    37,    38,    38,
      39,    39,    40,    40,    41,    41,    42,    42,    43,    43,
      44,    44,    45,    45,    46,    46,    47,    48,    48,    49,
      49,    50,    50,    51,    51,    52,    52,    52,    53,    53,
      54,    54,    55,    55,    55,    55,    55,    55,    56,    56
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     1,     1,     1,     1,     1,
       3,     2,     2,     3,     4,     6,     0,     2,     4,     5,
       1,     0,     0,     2,     4,     6,     0,     4,     0,     5,
       0,     2,     1,     1,     0,     2,     6,     0,     1,     1,
       3,     4,     8,     0,     2,     3,     4,     4,     1,     1,
       0,     2,     1,     3,     1,     3,     4,     4,     0,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     2,     1,     0,    21,     0,     0,     0,     0,
       4,     5,     6,     7,     8,     9,    16,    20,     0,    11,
      12,     0,     0,    22,     0,     0,    13,    10,     0,    17,
      26,    34,    50,     0,     0,    41,    14,     0,    21,     0,
      23,     0,    43,     0,    50,    50,     0,     0,    28,    15,
       0,    35,    58,    52,    54,    48,    49,     0,     0,    45,
      51,     0,     0,     0,    18,     0,     0,     0,     0,    44,
       0,     0,     0,     0,    47,    46,    19,    34,    24,     0,
      27,    37,    39,     0,    42,    53,    55,    32,    33,     0,
       0,     0,    30,     0,    38,    59,     0,    56,    57,    25,
       0,     0,    40,    29,    31,    36
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     2,    10,    11,    12,    13,    14,    23,    29,
      18,    30,    40,    41,    66,   100,    89,    42,    51,    93,
      83,    15,    52,    35,    59,    43,    60,    70
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -74
static const yysigned_char yypact[] =
{
     -74,    39,    21,   -74,     1,    26,    31,    49,    48,    50,
     -74,   -74,   -74,   -74,   -74,   -74,   -74,   -74,    53,   -74,
     -74,    10,    51,    55,     4,    52,   -74,   -74,    54,   -74,
      -7,   -74,   -74,    56,    57,   -74,   -74,    60,    26,    58,
     -74,    61,    41,    -3,   -74,   -74,     7,    62,   -74,   -74,
      66,   -74,     8,    63,   -74,   -74,   -74,    59,    64,   -74,
     -74,     0,    -3,    65,   -74,    37,    18,    67,    69,   -74,
      70,    44,    46,    46,   -74,   -74,   -74,   -74,   -74,    71,
     -74,    69,   -74,    32,   -74,   -74,   -74,   -74,   -74,    47,
      68,    -2,   -74,    72,    73,   -74,    76,   -74,   -74,   -74,
      19,    77,   -74,   -74,   -74,   -74
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -74,   -74,   -74,   -74,   -74,   -74,   -74,   -74,   -74,   -74,
      74,   -74,   -74,   -74,   -74,   -74,   -73,     5,   -74,   -74,
      -4,   -74,   -74,    34,    22,     9,   -74,   -74
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      90,    53,    54,    55,    53,    54,    99,    56,    16,    38,
      74,    31,    63,    25,    39,    32,    50,    64,    57,    32,
      26,    57,    79,    87,    88,    58,    80,   104,    58,   103,
      33,    34,    17,    68,    33,    34,     4,    19,     5,     3,
       6,     7,    95,     8,    77,     9,    96,    78,    85,    86,
      87,    88,    21,    61,    62,    20,    22,    24,    28,    50,
      97,    27,    36,    37,    46,    48,    65,    44,    45,    49,
      67,    72,    71,    82,    92,    76,    73,    94,    84,    81,
     102,    98,    91,   105,    75,   101,    69,    96,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    47
};

static const yysigned_char yycheck[] =
{
      73,     4,     5,     6,     4,     5,     8,    10,     7,    16,
      10,     7,     5,     3,    21,    11,    18,    10,    21,    11,
      10,    21,     4,     4,     5,    28,     8,   100,    28,    10,
      26,    27,     6,    25,    26,    27,    15,     6,    17,     0,
      19,    20,    10,    22,     7,    24,    14,    10,     4,     5,
       4,     5,     4,    44,    45,     6,     6,     4,     3,    18,
      13,    10,    10,     9,     4,     7,     4,    11,    11,     8,
       4,    12,     9,     4,     3,    10,    12,    81,     8,    12,
       4,    13,    77,     6,    62,    13,    52,    14,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    38
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    30,    31,     0,    15,    17,    19,    20,    22,    24,
      32,    33,    34,    35,    36,    50,     7,     6,    39,     6,
       6,     4,     6,    37,     4,     3,    10,    10,     3,    38,
      40,     7,    11,    26,    27,    52,    10,     9,    16,    21,
      41,    42,    46,    54,    11,    11,     4,    39,     7,     8,
      18,    47,    51,     4,     5,     6,    10,    21,    28,    53,
      55,    54,    54,     5,    10,     4,    43,     4,    25,    52,
      56,     9,    12,    12,    10,    53,    10,     7,    10,     4,
       8,    12,     4,    49,     8,     4,     5,     4,     5,    45,
      45,    46,     3,    48,    49,    10,    14,    13,    13,     8,
      44,    13,     4,    10,    45,     6
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
#line 163 "grampar.y"
    { ((ParseParams*)parseParam)->treeTop = new GrammarAST(yyvsp[0].topFormList); yyval.num=0; ;}
    break;

  case 3:
#line 167 "grampar.y"
    { yyval.topFormList = new ASTList<TopForm>; ;}
    break;

  case 4:
#line 168 "grampar.y"
    { (yyval.topFormList=yyvsp[-1].topFormList)->append(yyvsp[0].topForm); ;}
    break;

  case 5:
#line 172 "grampar.y"
    { yyval.topForm = yyvsp[0].topForm; ;}
    break;

  case 6:
#line 173 "grampar.y"
    { yyval.topForm = yyvsp[0].topForm; ;}
    break;

  case 7:
#line 174 "grampar.y"
    { yyval.topForm = yyvsp[0].topForm; ;}
    break;

  case 8:
#line 175 "grampar.y"
    { yyval.topForm = yyvsp[0].topForm; ;}
    break;

  case 9:
#line 176 "grampar.y"
    { yyval.topForm = yyvsp[0].topForm; ;}
    break;

  case 10:
#line 181 "grampar.y"
    { yyval.topForm = new TF_context(yyvsp[-1].str); ;}
    break;

  case 11:
#line 185 "grampar.y"
    { yyval.topForm = new TF_verbatim(false, yyvsp[0].str); ;}
    break;

  case 12:
#line 186 "grampar.y"
    { yyval.topForm = new TF_verbatim(true, yyvsp[0].str); ;}
    break;

  case 13:
#line 191 "grampar.y"
    { yyval.topForm = new TF_option(yyvsp[-1].str, 1); ;}
    break;

  case 14:
#line 192 "grampar.y"
    { yyval.topForm = new TF_option(yyvsp[-2].str, yyvsp[-1].num); ;}
    break;

  case 15:
#line 204 "grampar.y"
    { yyval.topForm = new TF_terminals(yyvsp[-3].termDecls, yyvsp[-2].termTypes, yyvsp[-1].precSpecs); ;}
    break;

  case 16:
#line 208 "grampar.y"
    { yyval.termDecls = new ASTList<TermDecl>; ;}
    break;

  case 17:
#line 209 "grampar.y"
    { (yyval.termDecls=yyvsp[-1].termDecls)->append(yyvsp[0].termDecl); ;}
    break;

  case 18:
#line 218 "grampar.y"
    { yyval.termDecl = new TermDecl(yyvsp[-3].num, yyvsp[-1].str, sameloc(yyvsp[-1].str, "")); ;}
    break;

  case 19:
#line 220 "grampar.y"
    { yyval.termDecl = new TermDecl(yyvsp[-4].num, yyvsp[-2].str, yyvsp[-1].str); ;}
    break;

  case 20:
#line 224 "grampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 21:
#line 225 "grampar.y"
    { yyval.str = nolocNULL(); ;}
    break;

  case 22:
#line 229 "grampar.y"
    { yyval.termTypes = new ASTList<TermType>; ;}
    break;

  case 23:
#line 230 "grampar.y"
    { (yyval.termTypes=yyvsp[-1].termTypes)->append(yyvsp[0].termType); ;}
    break;

  case 24:
#line 235 "grampar.y"
    { yyval.termType = new TermType(yyvsp[-1].str, yyvsp[-2].str, new ASTList<SpecFunc>); ;}
    break;

  case 25:
#line 237 "grampar.y"
    { yyval.termType = new TermType(yyvsp[-3].str, yyvsp[-4].str, yyvsp[-1].specFuncs); ;}
    break;

  case 26:
#line 241 "grampar.y"
    { yyval.precSpecs = new ASTList<PrecSpec>; ;}
    break;

  case 27:
#line 242 "grampar.y"
    { yyval.precSpecs = yyvsp[-1].precSpecs; ;}
    break;

  case 28:
#line 247 "grampar.y"
    { yyval.precSpecs = new ASTList<PrecSpec>; ;}
    break;

  case 29:
#line 249 "grampar.y"
    { (yyval.precSpecs=yyvsp[-4].precSpecs)->append(new PrecSpec(whichKind(yyvsp[-3].str), yyvsp[-2].num, yyvsp[-1].stringList)); ;}
    break;

  case 30:
#line 253 "grampar.y"
    { yyval.stringList = new ASTList<LocString>; ;}
    break;

  case 31:
#line 254 "grampar.y"
    { (yyval.stringList=yyvsp[-1].stringList)->append(yyvsp[0].str); ;}
    break;

  case 32:
#line 258 "grampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 33:
#line 259 "grampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 34:
#line 265 "grampar.y"
    { yyval.specFuncs = new ASTList<SpecFunc>; ;}
    break;

  case 35:
#line 266 "grampar.y"
    { (yyval.specFuncs=yyvsp[-1].specFuncs)->append(yyvsp[0].specFunc); ;}
    break;

  case 36:
#line 271 "grampar.y"
    { yyval.specFunc = new SpecFunc(yyvsp[-4].str, yyvsp[-2].stringList, yyvsp[0].str); ;}
    break;

  case 37:
#line 275 "grampar.y"
    { yyval.stringList = new ASTList<LocString>; ;}
    break;

  case 38:
#line 276 "grampar.y"
    { yyval.stringList = yyvsp[0].stringList; ;}
    break;

  case 39:
#line 280 "grampar.y"
    { yyval.stringList = new ASTList<LocString>(yyvsp[0].str); ;}
    break;

  case 40:
#line 281 "grampar.y"
    { (yyval.stringList=yyvsp[-2].stringList)->append(yyvsp[0].str); ;}
    break;

  case 41:
#line 293 "grampar.y"
    { yyval.topForm = new TF_nonterm(yyvsp[-1].str, yyvsp[-2].str, new ASTList<SpecFunc>,
                                     new ASTList<ProdDecl>(yyvsp[0].prodDecl), NULL); ;}
    break;

  case 42:
#line 296 "grampar.y"
    { yyval.topForm = new TF_nonterm(yyvsp[-5].str, yyvsp[-6].str, yyvsp[-3].specFuncs, yyvsp[-2].prodDecls, yyvsp[-1].stringList); ;}
    break;

  case 43:
#line 300 "grampar.y"
    { yyval.prodDecls = new ASTList<ProdDecl>; ;}
    break;

  case 44:
#line 301 "grampar.y"
    { (yyval.prodDecls=yyvsp[-1].prodDecls)->append(yyvsp[0].prodDecl); ;}
    break;

  case 45:
#line 305 "grampar.y"
    { yyval.prodDecl = new ProdDecl(yyvsp[-2].loc, PDK_NEW, yyvsp[-1].rhsList, yyvsp[0].str); ;}
    break;

  case 46:
#line 306 "grampar.y"
    { yyval.prodDecl = new ProdDecl(yyvsp[-2].loc, PDK_REPLACE,yyvsp[-1].rhsList, yyvsp[0].str); ;}
    break;

  case 47:
#line 307 "grampar.y"
    { yyval.prodDecl = new ProdDecl(yyvsp[-2].loc, PDK_DELETE, yyvsp[-1].rhsList, nolocNULL()); ;}
    break;

  case 48:
#line 311 "grampar.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 49:
#line 312 "grampar.y"
    { yyval.str = nolocNULL(); ;}
    break;

  case 50:
#line 316 "grampar.y"
    { yyval.rhsList = new ASTList<RHSElt>; ;}
    break;

  case 51:
#line 317 "grampar.y"
    { (yyval.rhsList=yyvsp[-1].rhsList)->append(yyvsp[0].rhsElt); ;}
    break;

  case 52:
#line 326 "grampar.y"
    { yyval.rhsElt = new RH_name(sameloc(yyvsp[0].str, ""), yyvsp[0].str); ;}
    break;

  case 53:
#line 327 "grampar.y"
    { yyval.rhsElt = new RH_name(yyvsp[-2].str, yyvsp[0].str); ;}
    break;

  case 54:
#line 328 "grampar.y"
    { yyval.rhsElt = new RH_string(sameloc(yyvsp[0].str, ""), yyvsp[0].str); ;}
    break;

  case 55:
#line 329 "grampar.y"
    { yyval.rhsElt = new RH_string(yyvsp[-2].str, yyvsp[0].str); ;}
    break;

  case 56:
#line 330 "grampar.y"
    { yyval.rhsElt = new RH_prec(yyvsp[-1].str); ;}
    break;

  case 57:
#line 331 "grampar.y"
    { yyval.rhsElt = new RH_forbid(yyvsp[-1].str); ;}
    break;

  case 58:
#line 335 "grampar.y"
    { yyval.stringList = NULL; ;}
    break;

  case 59:
#line 336 "grampar.y"
    { yyval.stringList = yyvsp[-1].stringList; ;}
    break;


    }

/* Line 1010 of yacc.c.  */
#line 1490 "grampar.tab.c"

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


#line 340 "grampar.y"

/* ------------------ extra C code ------------------ */
AssocKind whichKind(LocString * /*owner*/ kind)
{ 
  // delete 'kind' however we exit
  Owner<LocString> killer(kind);
  
  #define CHECK(syntax, value)   \
    if (kind->equals(syntax)) {  \
      return value;              \
    }
  CHECK("left", AK_LEFT);
  CHECK("right", AK_RIGHT);
  CHECK("nonassoc", AK_NONASSOC);
  CHECK("prec", AK_NEVERASSOC);
  CHECK("assoc_split", AK_SPLIT);
  #undef CHECK

  xbase(stringc << kind->locString()
                << ": invalid associativity kind: " << *kind);
}

