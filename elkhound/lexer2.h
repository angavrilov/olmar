// lexer2.h
// 2nd phase lexical analyzer; see lexer2.txt

#ifndef __LEXER2_H
#define __LEXER2_H

#include "lexer1.h"       // Lexer1
#include "fileloc.h"      // SourceLocation
#include "strtable.h"     // StringRef, StringTable
#include "useract.h"      // SemanticValue

// this enumeration defines the terminal symbols that the parser
// deals with
enum Lexer2TokenType {
  // I've avoided collapsing these onto fewer lines because
  // it makes systematic modification (search & replace)
  // more difficult

  // end of file
  L2_EOF=0,

  // non-keyword name
  L2_NAME,

  // classified name (for e.g. cdecl2)
  L2_TYPE_NAME,
  L2_VARIABLE_NAME,

  // literals
  L2_INT_LITERAL,
  L2_FLOAT_LITERAL,
  L2_STRING_LITERAL,
  L2_CHAR_LITERAL,

  // keywords
  L2_ASM,
  L2_AUTO,
  L2_BREAK,
  L2_BOOL,
  L2_CASE,
  L2_CATCH,
  L2_CDECL,
  L2_CHAR,
  L2_CLASS,
  L2_CONST,
  L2_CONST_CAST,
  L2_CONTINUE,
  L2_DEFAULT,
  L2_DELETE,
  L2_DO,
  L2_DOUBLE,
  L2_DYNAMIC_CAST,
  L2_ELSE,
  L2_ENUM,
  L2_EXPLICIT,
  L2_EXTERN,
  L2_FLOAT,
  L2_FOR,
  L2_FRIEND,
  L2_GOTO,
  L2_IF,
  L2_INLINE,
  L2_INT,
  L2_LONG,
  L2_MUTABLE,
  L2_NEW,
  L2_OPERATOR,
  L2_PASCAL,
  L2_PRIVATE,
  L2_PROTECTED,
  L2_PUBLIC,
  L2_REGISTER,
  L2_REINTERPRET_CAST,
  L2_RETURN,
  L2_SHORT,
  L2_SIGNED,
  L2_SIZEOF,
  L2_STATIC,
  L2_STATIC_CAST,
  L2_STRUCT,
  L2_SWITCH,
  L2_TEMPLATE,
  L2_THIS,
  L2_THROW,
  L2_TRY,
  L2_TYPEDEF,
  L2_TYPEID,
  L2_UNION,
  L2_UNSIGNED,
  L2_VIRTUAL,
  L2_VOID,
  L2_VOLATILE,
  L2_WCHAR_T,
  L2_WHILE,

  // operators
  L2_LPAREN,
  L2_RPAREN,
  L2_LBRACKET,
  L2_RBRACKET,
  L2_ARROW,
  L2_COLONCOLON,
  L2_DOT,
  L2_BANG,
  L2_TILDE,
  L2_PLUS,
  L2_MINUS,
  L2_PLUSPLUS,
  L2_MINUSMINUS,
  L2_AND,
  L2_STAR,
  L2_DOTSTAR,
  L2_ARROWSTAR,
  L2_SLASH,
  L2_PERCENT,
  L2_LEFTSHIFT,
  L2_RIGHTSHIFT,
  L2_LESSTHAN,
  L2_LESSEQ,
  L2_GREATERTHAN,
  L2_GREATEREQ,
  L2_EQUALEQUAL,
  L2_NOTEQUAL,
  L2_XOR,
  L2_OR,
  L2_ANDAND,
  L2_OROR,
  L2_QUESTION,
  L2_COLON,
  L2_EQUAL,
  L2_STAREQUAL,
  L2_SLASHEQUAL,
  L2_PERCENTEQUAL,
  L2_PLUSEQUAL,
  L2_MINUSEQUAL,
  L2_ANDEQUAL,
  L2_XOREQUAL,
  L2_OREQUAL,
  L2_LEFTSHIFTEQUAL,
  L2_RIGHTSHIFTEQUAL,
  L2_COMMA,
  L2_ELLIPSIS,
  L2_SEMICOLON,
  L2_LBRACE,
  L2_RBRACE,

  // GNU extensions
  L2___ATTRIBUTE__,
  L2___FUNCTION__,
  L2___LABEL__,
  L2___PRETTY_FUNCTION__,
  L2___TYPEOF__,

  // my extensions
  L2_OWNER,

  L2_NUM_TYPES
};

// yield name as above from the int value
char const *l2Tok2String(Lexer2TokenType type);


// represent a unit of input to the parser
class Lexer2Token {
public:
  // kind of token
  Lexer2TokenType type;

  // semantic value; 'sval' is essentially an owner until the parser
  // reads it, and then it's a serf; so in fact it's treated as a serf
  // throughout, with corresponding leak potential
  union {
    int intValue;                // for L2_INT_LITERALs
    
    // this is an owner pointer.. I'll fix this when I overhaul L2
    float *floatValue;           // for L2_FLOAT_LITERALs

    char charValue;              // for L2_CHAR_LITERALs
    StringRef strValue;          // for L2_NAMEs and L2_STRING_LITERALs; refers to Lexer2::idTable
    SemanticValue sval;          // union with above means we can extract from this
  };

  // TODO: handle strings with embedded nulls

  // where token appears, or where macro reference which produced it appears
  SourceLocation loc;            // line/col/file

  // macro definition that produced this token, or NULL
  Lexer1Token *sourceMacro;      // (serf)

public:
  Lexer2Token(Lexer2TokenType type, SourceLocation const &loc);
  ~Lexer2Token();

  // debugging
  void print() const;
  string toString(bool asSexp=false) const;
  string unparseString() const;  // return the source text that generated this token
};


// lexing state
class Lexer2 {
public:
  // storage of all the identifiers we encounter
  StringTable idTable;

  // output token stream
  ObjList<Lexer2Token> tokens;

  // for appending
  ObjListMutator<Lexer2Token> tokensMut;

public:
  Lexer2();
  ~Lexer2();
};


// interface to 2nd phase lexical analysis
// (will change; for now I'm only going to process single files)
void lexer2_lex(Lexer2 &dest, Lexer1 const &src, char const *fname);


// parser's interface to lexer2 (experimental)
extern Lexer2Token const *yylval;      // semantic value for returned token, or NULL for L2_EOF
extern "C" {
  Lexer2TokenType lexer2_gettoken();
}


#endif // __LEXER2_H
