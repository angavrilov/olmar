// lexer2.cc
// code for lexer2.h

#include "lexer2.h"      // this module
#include "trace.h"       // tracingSys
#include "strutil.h"     // encodeWithEscapes

#include <stdlib.h>      // strtoul


// ------------------ token type descriptions ----------------------
struct Lexer2TokenTypeDesc
{
  // type code; should equal index in l2TokTypes[] (but not checked)
  Lexer2TokenType tokType;

  // name as it appears in C code
  char const *typeName;

  // spelling for keywords and operators; descriptive name for others
  char const *spelling;

  // true for tokens where we emit spelling as Bison alias; also, this
  // must be true for us to consider 'spelling' to mean a keyword or
  // operator spelling as opposed to an L2_NAME
  bool bisonSpelling;
};

// name it once, get both the symbol and the string
#define N(name) name, #name

Lexer2TokenTypeDesc const l2TokTypes[] = {
  // eof
  { N(L2_EOF),                  "EOF", false },

  // name
  { N(L2_NAME),                 "NAME", false },

  // literals
  { N(L2_INT_LITERAL),          "INT_LITERAL", false },
  { N(L2_FLOAT_LITERAL),        "FLOAT_LITERAL", false },
  { N(L2_STRING_LITERAL),       "STRING_LITERAL", false },
  { N(L2_CHAR_LITERAL),         "CHAR_LITERAL", false },

  // keywords
  { N(L2_ASM),                  "asm", true },
  { N(L2_AUTO),                 "auto", true },
  { N(L2_BREAK),                "break", true },
  { N(L2_BOOL),                 "bool", true },
  { N(L2_CASE),                 "case", true },
  { N(L2_CATCH),                "catch", true },
  { N(L2_CDECL),                "cdecl", true },
  { N(L2_CHAR),                 "char", true },
  { N(L2_CLASS),                "class", true },
  { N(L2_CONST),                "const", true },
  { N(L2_CONST_CAST),           "const_cast", true },
  { N(L2_CONTINUE),             "continue", true },
  { N(L2_DEFAULT),              "default", true },
  { N(L2_DELETE),               "delete", true },
  { N(L2_DO),                   "do", true },
  { N(L2_DOUBLE),               "double", true },
  { N(L2_DYNAMIC_CAST),         "dynamic_cast", true },
  { N(L2_ELSE),                 "else", true },
  { N(L2_ENUM),                 "enum", true },
  { N(L2_EXPLICIT),             "explicit", true },
  { N(L2_EXTERN),               "extern", true },
  { N(L2_FLOAT),                "float", true },
  { N(L2_FOR),                  "for", true },
  { N(L2_FRIEND),               "friend", true },
  { N(L2_GOTO),                 "goto", true },
  { N(L2_IF),                   "if", true },
  { N(L2_INLINE),               "inline", true },
  { N(L2_INT),                  "int", true },
  { N(L2_LONG),                 "long", true },
  { N(L2_MUTABLE),              "mutable", true },
  { N(L2_NEW),                  "new", true },
  { N(L2_OPERATOR),             "operator", true },
  { N(L2_PASCAL),               "pascal", true },
  { N(L2_PRIVATE),              "private", true },
  { N(L2_PROTECTED),            "protected", true },
  { N(L2_PUBLIC),               "public", true },
  { N(L2_REGISTER),             "register", true },
  { N(L2_REINTERPRET_CAST),     "reinterpret_cast", true },
  { N(L2_RETURN),               "return", true },
  { N(L2_SHORT),                "short", true },
  { N(L2_SIGNED),               "signed", true },
  { N(L2_SIZEOF),               "sizeof", true },
  { N(L2_STATIC),               "static", true },
  { N(L2_STATIC_CAST),          "static_cast", true },
  { N(L2_STRUCT),               "struct", true },
  { N(L2_SWITCH),               "switch", true },
  { N(L2_TEMPLATE),             "template", true },
  { N(L2_THIS),                 "this", true },
  { N(L2_THROW),                "throw", true },
  { N(L2_TRY),                  "try", true },
  { N(L2_TYPEDEF),              "typedef", true },
  { N(L2_TYPEID),               "typeid", true },
  { N(L2_UNION),                "union", true },
  { N(L2_UNSIGNED),             "unsigned", true },
  { N(L2_VIRTUAL),              "virtual", true },
  { N(L2_VOID),                 "void", true },
  { N(L2_VOLATILE),             "volatile", true },
  { N(L2_WCHAR_T),              "wchar_t", true },
  { N(L2_WHILE),                "while", true },
  
  // operators
  { N(L2_LPAREN),               "(", true },
  { N(L2_RPAREN),               ")", true },
  { N(L2_LBRACKET),             "[", true },
  { N(L2_RBRACKET),             "]", true },
  { N(L2_ARROW),                "->", true },
  { N(L2_COLONCOLON),           "::", true },
  { N(L2_DOT),                  ".", true },
  { N(L2_BANG),                 "!", true },
  { N(L2_TILDE),                "~", true },
  { N(L2_PLUS),                 "+", true },
  { N(L2_MINUS),                "-", true },
  { N(L2_PLUSPLUS),             "++", true },
  { N(L2_MINUSMINUS),           "--", true },
  { N(L2_AND),                  "&", true },
  { N(L2_STAR),                 "*", true },
  { N(L2_DOTSTAR),              ".*", true },
  { N(L2_ARROWSTAR),            "->*", true },
  { N(L2_SLASH),                "/", true },
  { N(L2_PERCENT),              "%", true },
  { N(L2_LEFTSHIFT),            "<<", true },
  { N(L2_RIGHTSHIFT),           ">>", true },
  { N(L2_LESSTHAN),             "<", true },
  { N(L2_LESSEQ),               "<=", true },
  { N(L2_GREATERTHAN),          ">", true },
  { N(L2_GREATEREQ),            ">=", true },
  { N(L2_EQUALEQUAL),           "==", true },
  { N(L2_NOTEQUAL),             "!=", true },
  { N(L2_XOR),                  "^", true },
  { N(L2_OR),                   "|", true },
  { N(L2_ANDAND),               "&&", true },
  { N(L2_OROR),                 "||", true },
  { N(L2_QUESTION),             "?", true },
  { N(L2_COLON),                ":", true },
  { N(L2_EQUAL),                "=", true },
  { N(L2_STAREQUAL),            "*=", true },
  { N(L2_SLASHEQUAL),           "/=", true },
  { N(L2_PERCENTEQUAL),         "%=", true },
  { N(L2_PLUSEQUAL),            "+=", true },
  { N(L2_MINUSEQUAL),           "-=", true },
  { N(L2_ANDEQUAL),             "&=", true },
  { N(L2_XOREQUAL),             "^=", true },
  { N(L2_OREQUAL),              "|=", true },
  { N(L2_LEFTSHIFTEQUAL),       "<<=", true },
  { N(L2_RIGHTSHIFTEQUAL),      ">>=", true },
  { N(L2_COMMA),                ",", true },
  { N(L2_ELLIPSIS),             "...", true },
  { N(L2_SEMICOLON),            ";", true },
  { N(L2_LBRACE),               "{", true },
  { N(L2_RBRACE),               "}", true },
};

#undef N


Lexer2TokenType lookupKeyword(char const *keyword)
{
  xassert(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);

  loopi(L2_NUM_TYPES) {
    if (l2TokTypes[i].bisonSpelling && 
        0==strcmp(l2TokTypes[i].spelling, keyword)) {
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
    //if (i == L2_EOF) {
    //  continue;    // bison doesn't like me to define a token with code 0
    //}
    // but: now I'm using these for my own parser, and I want the 0

    Lexer2TokenTypeDesc const &desc = l2TokTypes[i];
    xassert(desc.tokType == i);    // check correspondence between index and tokType

    printf("%%token %-20s %3d   ", desc.typeName, desc.tokType);
    if (spellings && desc.bisonSpelling) {
      printf("\"%s\"\n", desc.spelling);       // string token
    }
    else {
      printf("\n");                            // no spelling.. testing..
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


string Lexer2Token::toString() const
{
  string ret = l2Tok2String(type);

  // add the literal value, if any
  switch (type) {
    case L2_NAME:
    case L2_STRING_LITERAL:
      ret &= stringc << "(" << strValue << ")";
      break;

    case L2_INT_LITERAL:
      ret &= stringc << "(" << intValue << ")";
      break;

    default:     // silence warning -- what is this, ML??
      break;
  }

  return ret;
}


string Lexer2Token::unparseString() const
{
  switch (type) {
    case L2_NAME:
      return strValue;
      
    case L2_STRING_LITERAL:
      return stringc << "\"" 
                     << encodeWithEscapes(strValue, strValue.length())
                     << "\"";
                    
    case L2_INT_LITERAL:
      return stringc << intValue;

    default:
      return l2Tok2String(type);     // operators and keywords
  }
}


void Lexer2Token::print() const
{
  printf("[L2] Token at line %d, col %d: %s\n",
         loc.line, loc.col, toString().pcharc());
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
  // keep track of previous L2 token emitted so we can do token
  // collapsing for string juxtaposition
  Lexer2Token *prevToken = NULL;

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

    if (L1->type == L1_STRING_LITERAL         &&
        prevToken != NULL                     &&
        prevToken->type == L2_STRING_LITERAL) {
      // coalesce adjacent strings
      prevToken->strValue &= L1->text;
      continue;
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
        L2->strValue = L1->text;
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
    prevToken = L2;

    // (debugging) print it
    if (tracingSys("lexer2")) {
      L2->print();
    }
  }

  // final token
  dest.tokensMut.append(
    new Lexer2Token(L2_EOF, FileLocation() /*dummy*/, NULL /*file (for now)*/));
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
    printBisonTokenDecls(true /*spellings*/);
    return 0;
  }

  while (lexer2_gettoken() != L2_EOF) {
    yylval->print();
  }

  return 0;
}

#endif // TEST_LEXER2
