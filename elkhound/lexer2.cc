// lexer2.cc
// code for lexer2.h

#include "lexer2.h"      // this module
#include "trace.h"       // tracingSys

#include <stdlib.h>      // strtoul


// ------------------ token type descriptions ----------------------
struct Lexer2TokenTypeDesc {
  Lexer2TokenType tokType;     // type code; should equal index in l2TokTypes[] (but not checked)
  char const *typeName;        // name as it appears in C code
  char const *spelling;        // spelling for keywords and operators; descriptive name for others
};
	 
// name it once, get both the symbol and the string
#define N(name) name, #name

Lexer2TokenTypeDesc const l2TokTypes[] = {
  // eof
  { N(L2_EOF),             	"EOF" },

  // name
  { N(L2_NAME),	 		"NAME" },

  // literals
  { N(L2_INT_LITERAL),     	"INT_LITERAL" },
  { N(L2_FLOAT_LITERAL),   	"FLOAT_LITERAL" },
  { N(L2_STRING_LITERAL),  	"STRING_LITERAL" },
  { N(L2_CHAR_LITERAL),    	"CHAR_LITERAL" },

  // keywords
  { N(L2_ASM),			"asm" },
  { N(L2_AUTO),			"auto" },
  { N(L2_BREAK),		"break" },
  { N(L2_CASE),			"case" },
  { N(L2_CATCH),		"catch" },
  { N(L2_CDECL),  		"cdecl" },
  { N(L2_CHAR),   		"char" },
  { N(L2_CLASS),		"class" },
  { N(L2_CONST),		"const" },
  { N(L2_CONTINUE),		"continue" },
  { N(L2_DEFAULT),		"default" },
  { N(L2_DELETE),		"delete" },
  { N(L2_DO),			"do" },
  { N(L2_DOUBLE),		"double" },
  { N(L2_ELSE),			"else" },
  { N(L2_ENUM),			"enum" },
  { N(L2_EXTERN),		"extern" },
  { N(L2_FLOAT),		"float" },
  { N(L2_FOR),			"for" },
  { N(L2_FRIEND),		"friend" },
  { N(L2_GOTO),			"goto" },
  { N(L2_IF),			"if" },
  { N(L2_INLINE),		"inline" },
  { N(L2_INT),			"int" },
  { N(L2_LONG),			"long" },
  { N(L2_NEW),			"new" },
  { N(L2_OPERATOR),		"operator" },
  { N(L2_PASCAL),		"pascal" },
  { N(L2_PRIVATE),	   	"private" },
  { N(L2_PROTECTED),		"protected" },
  { N(L2_PUBLIC),		"public" },
  { N(L2_REGISTER),		"register" },
  { N(L2_RETURN),		"return" },
  { N(L2_SHORT),		"short" },
  { N(L2_SIGNED),	     	"signed" },
  { N(L2_SIZEOF),		"sizeof" },
  { N(L2_STATIC),		"static" },
  { N(L2_STRUCT),		"struct" },
  { N(L2_SWITCH),		"switch" },
  { N(L2_TEMPLATE),		"template" },
  { N(L2_THIS),			"this" },
  { N(L2_THROW),		"throw" },
  { N(L2_TRY),			"try" },
  { N(L2_TYPEDEF),		"typedef" },
  { N(L2_UNION),		"union" },
  { N(L2_UNSIGNED),		"unsigned" },
  { N(L2_VIRTUAL),		"virtual" },
  { N(L2_VOID),			"void" },
  { N(L2_VOLATILE),		"volatile" },
  { N(L2_WCHAR_T),		"wchar_t" },
  { N(L2_WHILE),		"while" },
  
  // operators
  { N(L2_LPAREN),		"(" },
  { N(L2_RPAREN),		")" },
  { N(L2_LBRACKET),		"[" },
  { N(L2_RBRACKET),		"]" },
  { N(L2_ARROW),		"->" },
  { N(L2_COLONCOLON),		"::" },
  { N(L2_DOT),			"." },
  { N(L2_BANG),			"!" },
  { N(L2_TILDE),	   	"~" },
  { N(L2_PLUS),			"+" },
  { N(L2_MINUS),		"-" },
  { N(L2_PLUSPLUS),		"++" },
  { N(L2_MINUSMINUS),		"--" },
  { N(L2_AND),			"&" },
  { N(L2_STAR),			"*" },
  { N(L2_DOTSTAR),		".*" },
  { N(L2_ARROWSTAR),		"->*" },
  { N(L2_SLASH),		"/" },
  { N(L2_PERCENT),		"%" },
  { N(L2_LEFTSHIFT),		"<<" },
  { N(L2_RIGHTSHIFT),		">>" },
  { N(L2_LESSTHAN),		"<" },
  { N(L2_LESSEQ),		"<=" },
  { N(L2_GREATERTHAN),		">" },
  { N(L2_GREATEREQ),		">=" },
  { N(L2_EQUALEQUAL),		"==" },
  { N(L2_NOTEQUAL),		"!=" },
  { N(L2_XOR),			"^" },
  { N(L2_OR),			"|" },
  { N(L2_ANDAND),		"&&" },
  { N(L2_OROR),			"||" },
  { N(L2_QUESTION),		"?" },
  { N(L2_COLON),		":" },
  { N(L2_EQUAL),		"=" },
  { N(L2_STAREQUAL),   		"*=" },
  { N(L2_SLASHEQUAL),  		"/=" },
  { N(L2_PERCENTEQUAL),		"%=" },
  { N(L2_PLUSEQUAL),		"+=" },
  { N(L2_MINUSEQUAL),		"-=" },
  { N(L2_ANDEQUAL),		"&=" },
  { N(L2_XOREQUAL),		"^=" },
  { N(L2_OREQUAL),	   	"|=" },
  { N(L2_LEFTSHIFTEQUAL),  	"<<=" },
  { N(L2_RIGHTSHIFTEQUAL), 	">>=" },
  { N(L2_COMMA),	   	"," },
  { N(L2_ELLIPSIS),		"..." },
  { N(L2_SEMICOLON),		";" },
  { N(L2_LBRACE),          	"{" },
  { N(L2_RBRACE),          	"}" },
};

#undef N


Lexer2TokenType lookupKeyword(char const *keyword)
{
  xassert(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);

  loopi(L2_NUM_TYPES) {
    if (0==strcmp(l2TokTypes[i].spelling, keyword)) {
      return l2TokTypes[i].tokType;
    }
  }
  return L2_NAME;     // not found, so is user-defined name
}

char const *l2Tok2String(Lexer2TokenType type)
{
  xassert(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);
  xassert(type < L2_NUM_TYPES);

  return l2TokTypes[type].spelling;
}


// list of "%token" declarations for Bison
void printBisonTokenDecls(bool spellings)
{
  loopi(L2_NUM_TYPES) {
    if (i == L2_EOF) {
      continue;    // bison doesn't like me to define a token with code 0
    }

    Lexer2TokenTypeDesc const &desc = l2TokTypes[i];
    printf("%%token %-20s %3d   ", desc.typeName, desc.tokType);
    if (spellings) {
      if (strlen(desc.spelling) == 1) {
	printf("'%c'\n", desc.spelling[0]);      // character token
      }
      else {
	printf("\"%s\"\n", desc.spelling);       // string token
      }
    }
    else {
      printf("\n");    // no spelling.. testing..
    }
  }
}


// ----------------------- Lexer2Token -------------------------------
Lexer2Token::Lexer2Token(Lexer2TokenType aType, FileLocation const &aLoc,
                         SourceFile *aFile)
  : type(aType),
    strValue(),      // null initially
    intValue(0),     // legal? apparently..
    loc(aLoc),
    file(aFile),
    sourceMacro(NULL)
{}

Lexer2Token::~Lexer2Token()
{}


void Lexer2Token::print() const
{
  printf("[L2] Token at line %d, col %d: %s ",
         loc.line, loc.col, l2Tok2String(type));
  switch (type) {
    case L2_NAME:
      printf("%s ", strValue.pcharc());
      break;
      
    case L2_INT_LITERAL:
      printf("%d ", intValue);
      break;

    default:     // silence warning -- what is this, ML??
      break;
  }

  // for now, don't have literal values, so can't print them

  printf("\n");
}


// ------------------------- lexer 2 itself ----------------------------
// jobs performed:
//  - distinctions are drawn, e.g. keywords and operators
//  - whitespace and comments are stripped
// not performed yet:
//  - meaning is extracted, e.g. for integer literals
//  - preprocessor actions are performed: inclusion and macro expansion
void lexer2_lex(Lexer2 &dest, Lexer1 const &src)
{
  // iterate over all the L1 tokens
  ObjListIter<Lexer1Token> L1_iter(src.tokens);
  for (; !L1_iter.isDone(); L1_iter.adv()) {
    // convenient renaming
    Lexer1Token const *L1 = L1_iter.data();

    if (L1->type == L1_PREPROCESSOR ||     // for now
        L1->type == L1_WHITESPACE   ||
        L1->type == L1_COMMENT      ||
        L1->type == L1_ILLEGAL) {
      continue;    // filter it out entirely
    }

    // create the object for the yielded token; don't know the type
    // yet at this point, so I use L2_NAME as a placeholder
    Lexer2Token *L2 = new Lexer2Token(L2_NAME, L1->loc, NULL /*file (for now)*/);

    switch (L1->type) {
      case L1_IDENTIFIER:
        L2->type = lookupKeyword(L1->text);      // keyword's type or L2_NAME
        if (L2->type == L2_NAME) {
          L2->strValue = L1->text;               // save name's text
        }
        break;

      case L1_INT_LITERAL:
        L2->type = L2_INT_LITERAL;
        L2->intValue = strtoul(L1->text, NULL /*endptr*/, 0 /*radix*/);
        break;

      case L1_FLOAT_LITERAL:
        L2->type = L2_FLOAT_LITERAL;
        break;

      case L1_STRING_LITERAL:
        L2->type = L2_STRING_LITERAL;
        break;

      case L1_CHAR_LITERAL:
        L2->type = L2_CHAR_LITERAL;
        break;

      case L1_OPERATOR:
        L2->type = lookupKeyword(L1->text);      // operator's type
        break;

      default:
        xfailure("unknown L1 type");
    }

    // append this token to the running list
    dest.tokensMut.append(L2);

    // (debugging) print it
    if (tracingSys("lexer2")) {
      L2->print();
    }
  }
}


// --------------------- Lexer2 ------------------
Lexer2::Lexer2()
  : tokens(),
    tokensMut(tokens)
{}

Lexer2::~Lexer2()
{}


// ------------- experimental interface for (e.g.) bison ------------
Lexer2Token const *yylval = NULL;

// returns token types until EOF, at which point L2_EOF is returned
Lexer2TokenType lexer2_gettoken()
{
  static Lexer1 *lexer1 = NULL;
  static Lexer2 *lexer2 = NULL;
  static ObjListIter<Lexer2Token> *iter = NULL;

  if (!lexer1) {
    // do first phase
    lexer1 = new Lexer1;
    lexer1_lex(*lexer1, stdin);

    if (lexer1->errors > 0) {
      printf("%d error(s)\n", lexer1->errors);
      //return L2_EOF;   // done
    }

    // do second phase
    lexer2 = new Lexer2;
    lexer2_lex(*lexer2, *lexer1);

    // prepare to return tokens
    iter = new ObjListIter<Lexer2Token>(lexer2->tokens);
  }

  if (!iter->isDone()) {
    // grab type to return
    yylval = iter->data();
    Lexer2TokenType ret = iter->data()->type;

    // advance to next token
    iter->adv();

    // return one we just advanced past
    return ret;
  }
  else {
    // done; don't bother freeing things
    yylval = NULL;
    return L2_EOF;
  }
}


// ----------------------- testing --------------------
#ifdef TEST_LEXER2

int main(int argc)
{
  if (argc > 1) {
    printBisonTokenDecls(false /*spellings*/);
    return 0;
  }

  while (lexer2_gettoken() != L2_EOF) {
    yylval->print();
  }

  return 0;
}

#endif // TEST_LEXER2
