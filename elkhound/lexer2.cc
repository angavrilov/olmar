// lexer2.cc
// code for lexer2.h

#include "lexer2.h"      // this module
#include "trace.h"       // tracingSys
#include "strutil.h"     // encodeWithEscapes
#include "exc.h"         // xformat

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
  { N(L2_TYPE_NAME),            "TYPE_NAME", false },
  { N(L2_VARIABLE_NAME),        "VARIABLE_NAME", false },

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
  { N(L2_FALSE),                "false", true },
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
  { N(L2_TRUE),                 "true", true },
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
  
  // extensions for parsing gnu
  { N(L2___ATTRIBUTE__),        "__attribute__", true },
  { N(L2___FUNCTION__),         "__FUNCTION__", true },
  { N(L2___LABEL__),            "__label__", true },
  { N(L2___PRETTY_FUNCTION__),  "__PRETTY_FUNCTION__", true },
  { N(L2___TYPEOF__),           "__typeof__", true },
  
  // my own extension
  { N(L2_OWNER),                "owner", true },
  
  // additional tokens to help in specifying disambiguation
  { N(L2_PREFER_REDUCE),        "PREFER_REDUCE", true },
  { N(L2_PREFER_SHIFT),         "PREFER_SHIFT", true },
                                                           
  // theorem-prover extensions
  { N(L2_THMPRV_ASSERT),        "thmprv_assert", true },
  { N(L2_THMPRV_ASSUME),        "thmprv_assume", true },
  { N(L2_THMPRV_INVARIANT),     "thmprv_invariant", true },
  { N(L2_IMPLIES),              "==>", true },
  { N(L2_THMPRV_PRE),           "thmprv_pre", true },
  { N(L2_THMPRV_POST),          "thmprv_post", true },
  { N(L2_THMPRV_LET),           "thmprv_let", true },
  { N(L2_THMPRV_ATTR),          "thmprv_attr", true },
};

#undef N


// map of GNU keywords into ANSI keywords (I don't know and
// don't care about whatever differences there may be)
struct GNUKeywordMap {
  char const *spelling;
  Lexer2TokenType tokType;
} gnuKeywordMap[] = {
  { "__asm__", L2_ASM },
  { "__const__", L2_CONST },
  { "__label__", L2___LABEL__ },
  { "__inline__", L2_INLINE },
  { "__signed__", L2_SIGNED },
  { "__volatile__", L2_VOLATILE },
  { "__FUNCTION__", L2___FUNCTION__ },
  { "__PRETTY_FUNCTION__", L2___PRETTY_FUNCTION__ },

  // hack to make C++ keywords not appear to be keywords;
  // need to make this dependent on some flag somewhere ..
  { "bool", L2_NAME },
  { "catch", L2_NAME },
  { "class", L2_NAME },
  { "complex", L2_NAME },
  { "const_cast", L2_NAME },
  { "delete", L2_NAME },
  { "dynamic_cast", L2_NAME },
  { "explicit", L2_NAME },
  { "export", L2_NAME },
  { "friend", L2_NAME },
  { "mutable", L2_NAME },
  { "namespace", L2_NAME },
  { "new", L2_NAME },
  { "operator", L2_NAME },
  { "private", L2_NAME },
  { "protected", L2_NAME },
  { "public", L2_NAME },
  { "reinterpret_cast", L2_NAME },
  { "static_cast", L2_NAME },
  { "template", L2_NAME },
  { "this", L2_NAME },
  { "throw", L2_NAME },
  { "try", L2_NAME },
  { "typename", L2_NAME },
  { "using", L2_NAME },
  { "virtual", L2_NAME },

  // ones that GNU c has
  //{ "inline", L2_NAME },
};


Lexer2TokenType lookupKeyword(char const *keyword)
{
  // works?
  STATIC_ASSERT(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);

  xassert(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);

  {loopi(TABLESIZE(gnuKeywordMap)) {
    if (0==strcmp(gnuKeywordMap[i].spelling, keyword)) {
      return gnuKeywordMap[i].tokType;
    }
  }}

  {loopi(L2_NUM_TYPES) {
    if (l2TokTypes[i].bisonSpelling &&
        0==strcmp(l2TokTypes[i].spelling, keyword)) {
      return l2TokTypes[i].tokType;
    }
  }}

  return L2_NAME;     // not found, so is user-defined name
}

char const *l2Tok2String(Lexer2TokenType type)
{
  xassert(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);
  xassert(type < L2_NUM_TYPES);

  return l2TokTypes[type].spelling;
}

char const *l2Tok2SexpString(Lexer2TokenType type)
{
  xassert(TABLESIZE(l2TokTypes) == L2_NUM_TYPES);
  xassert(type < L2_NUM_TYPES);

  // for sexps let's use the L2_XXX names; among other things,
  // that will prevent syntactic parentheses from being interpreted
  // by the sexp reader (which probably wouldn't be all that
  // terrible, but not what I want...)
  return l2TokTypes[type].typeName;
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


// list of token declarations for my parser
void printMyTokenDecls()
{
  printf("// this list automatically generated by lexer2\n"
         "\n"
         "// form:\n"
         "//   <code> : <name> [<alias>] ;\n"
         "\n");

  loopi(L2_NUM_TYPES) {
    Lexer2TokenTypeDesc const &desc = l2TokTypes[i];
    xassert(desc.tokType == i);    // check correspondence between index and tokType

    printf("  %3d : %-20s ", desc.tokType, desc.typeName);
    if (desc.bisonSpelling) {
      printf("\"%s\" ;\n", desc.spelling);       // string token alias
    }
    else {
      printf(";\n");                             // no alias
    }
  }
}


// ----------------------- Lexer2Token -------------------------------
Lexer2Token::Lexer2Token(Lexer2TokenType aType, SourceLocation const &aLoc)
  : type(aType),
    intValue(0),     // legal? apparently..
    loc(aLoc),
    sourceMacro(NULL)
{}

Lexer2Token::~Lexer2Token()
{}


string Lexer2Token::toString(bool asSexp) const
{
  return toStringType(asSexp, type);
}

string Lexer2Token::toStringType(bool asSexp, Lexer2TokenType type) const
{
  string tokType;
  if (!asSexp) {
    tokType = l2Tok2String(type);
  }
  else {
    tokType = l2Tok2SexpString(type);
  }

  // get the literal value, if any
  string litVal;
  switch (type) {
    case L2_NAME:
    case L2_TYPE_NAME:
    case L2_VARIABLE_NAME:
    case L2_STRING_LITERAL:
      litVal = stringc << strValue;
      break;

    case L2_INT_LITERAL:
      litVal = stringc << intValue;
      break;

    default:
      return tokType;
  }

  if (!asSexp) {
    return stringc << tokType << "(" << litVal << ")";
  }
  else {
    return stringc << "(" << tokType << " " << litVal << ")";
  }
}


string Lexer2Token::unparseString() const
{
  switch (type) {
    case L2_NAME:
      return string(strValue);

    case L2_STRING_LITERAL:
      return stringc << quoted(strValue);

    case L2_INT_LITERAL:
      return stringc << intValue;

    default:
      return l2Tok2String(type);     // operators and keywords
  }
}


void Lexer2Token::print() const
{
  printf("[L2] Token at %s: %s\n",
         loc.toString().pcharc(), toString().pcharc());
}


void quotedUnescape(string &dest, int &destLen, char const *src,
                    char delim, bool allowNewlines)
{
  // strip quotes or ticks
  decodeEscapes(dest, destLen, string(src+1, strlen(src)-2),
                delim, allowNewlines);
}


// ------------------------- lexer 2 itself ----------------------------
// jobs performed:
//  - distinctions are drawn, e.g. keywords and operators
//  - whitespace and comments are stripped
//  - meaning is extracted, e.g. for integer literals
// not performed yet:
//  - preprocessor actions are performed: inclusion and macro expansion
void lexer2_lex(Lexer2 &dest, Lexer1 const &src, char const *fname)
{
  // get the SourceFile
  SourceFile *sourceFile = sourceFileList.open(fname);

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
      // coalesce adjacent strings (this is not efficient code..)
      stringBuilder sb;
      sb << prevToken->strValue;

      string tempString;
      int tempLength;
      quotedUnescape(tempString, tempLength, L1->text, '"',
                     src.allowMultilineStrings);
      sb.append(tempString, tempLength);

      prevToken->strValue = dest.idTable.add(sb);
      continue;
    }

    // create the object for the yielded token; don't know the type
    // yet at this point, so I use L2_NAME as a placeholder
    Lexer2Token *L2 =
      new Lexer2Token(L2_NAME,
                      SourceLocation(L1->loc, sourceFile));

    try {
      switch (L1->type) {
        case L1_IDENTIFIER:
          L2->type = lookupKeyword(L1->text);      // keyword's type or L2_NAME
          if (L2->type == L2_NAME) {
            // save name's text
            L2->strValue = dest.idTable.add(L1->text);
          }
          break;

        case L1_INT_LITERAL:
          L2->type = L2_INT_LITERAL;
          L2->intValue = strtoul(L1->text, NULL /*endptr*/, 0 /*radix*/);
          break;

        case L1_FLOAT_LITERAL:
          L2->type = L2_FLOAT_LITERAL;
          L2->floatValue = new float(atof(L1->text));
          break;

        case L1_STRING_LITERAL: {
          L2->type = L2_STRING_LITERAL;
          string tmp;
          int tmpLen;
          quotedUnescape(tmp, tmpLen, L1->text, '"',
                         src.allowMultilineStrings);
          if (tmpLen != tmp.length()) {
            cout << "warning: literal string with embedded nulls not handled properly\n";
          }

          L2->strValue = dest.idTable.add(tmp);
          break;
        }

        case L1_CHAR_LITERAL: {
          L2->type = L2_CHAR_LITERAL;
          int tempLen;
          string temp;
          quotedUnescape(temp, tempLen, L1->text, '\'',
                         false /*allowNewlines*/);

          if (tempLen != 1) {
            xformat("character literal must have 1 char");
          }

          L2->charValue = temp[0];
          break;
        }

        case L1_OPERATOR:
          L2->type = lookupKeyword(L1->text);      // operator's type
          xassert(L2->type != L2_NAME);            // otherwise invalid operator text..
          break;

        default:
          xfailure("unknown L1 type");
      }
    }
    catch (xFormat &x) {
      cout << L1->loc.toString() << ": " << x.cond() << endl;
      continue;
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
    new Lexer2Token(L2_EOF, SourceLocation() /*dummy*/));
}


// --------------------- Lexer2 ------------------
Lexer2::Lexer2()
  : myIdTable(new StringTable()),
    idTable(*myIdTable),      // hope this works..
    tokens(),
    tokensMut(tokens)
{}

Lexer2::Lexer2(StringTable &extTable)
  : myIdTable(NULL),
    idTable(extTable),
    tokens(),
    tokensMut(tokens)
{}

Lexer2::~Lexer2()
{
  if (myIdTable) {
    delete myIdTable;
  }
}


SourceLocation Lexer2::startLoc() const
{
  if (tokens.isNotEmpty()) {
    return tokens.firstC()->loc;
  }
  else {
    return SourceLocation();    // empty
  }
}


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
    lexer2_lex(*lexer2, *lexer1, "<stdin>");

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

int main(int argc, char **argv)
{
  if (argc > 1 && 0==strcmp(argv[1], "bison")) {
    printBisonTokenDecls(true /*spellings*/);
    return 0;
  }
  
  if (argc > 1 && 0==strcmp(argv[1], "myparser")) {
    printMyTokenDecls();
    return 0;
  }

  if (argc > 1) {
    printf("unknown argument: %s\n", argv[1]);
    return 1;
  }

  while (lexer2_gettoken() != L2_EOF) {
    yylval->print();
  }

  return 0;
}

#endif // TEST_LEXER2
