// gramlex.h
// GrammarLexer: a c++ lexer class for use with Flex's generated c++ scanner
// this lexer class is used both for parsing both AST and grammar descriptions;
// they differ in their .lex description, but their lexing state is the same

#ifndef __GRAMLEX_H
#define __GRAMLEX_H


// This included file is part of the Flex distribution.  It is
// installed in /usr/include on my Linux machine.  By including it, we
// get the declaration of the yyFlexLexer class.  Note that the file
// that flex generates, gramlex.yy.cc, also #includes this file.
// Perhaps also worth mentioning: I'm developing this with flex 2.5.4.
#include <FlexLexer.h>


// token code definitions
#define TOK_EOF 0             // better name
#define TOK_INCLUDE 1         // not seen by parser


// other includes
#include "str.h"              // string
#include "objlist.h"          // ObjList
#include "fileloc.h"          // SourceLocation
#include "embedded.h"         // EmbeddedLang


// this class just holds the lexer state so it is properly encapsulated
// (and therefore, among other things, re-entrant)
class GrammarLexer : public yyFlexLexer, public ReportError {
public:      // types
  enum Constants {
    lexBufferSize = 4096,          // size of new lex buffers
  };
                                          
  // return true if the given token code is one of those representing
  // embedded text
  typedef bool (*isEmbedTok)(int tokCode);

private:     // data
  // state of a file we were or are lexing
  struct FileState : public SourceLocation {
    istream *source;               // (owner?) source stream
    yy_buffer_state *bufstate;     // (owner?) flex's internal buffer state

  public:
    FileState(SourceFile *file, istream *source);
    ~FileState();

    FileState(FileState const &obj);
    FileState& operator= (FileState const &obj);
  };

  FileState fileState;             // state for file we're lexing now
  ObjList<FileState> fileStack;    // stack of files we will return to

  SourceLocation tokenStartLoc;    // location of start of current token

  // support for embedded code
  bool expectingEmbedded;          // true when certain punctuation triggers
                                   // embedded processing
  char embedFinish;                // which character ends the embedded section
  int embedMode;                   // TOK_FUNDECL_BODY or TOK_FUN_BODY
  EmbeddedLang *embedded;          // (owner) the processor
  isEmbedTok embedTokTest;         // for printing diagnostics

public:      // data
  // todo: can eliminate commentStartLine in favor of tokenStartLoc?
  int commentStartLine;            // for reporting unterminated C comments
  int integerLiteral;              // to store number literal value
  string stringLiteral;            // string in quotes, minus the quotes
  string includeFileName;          // name in an #include directive

  // defined in the base class, FlexLexer:
  //   const char *YYText();           // start of matched text
  //   int YYLeng();                   // number of matched characters

private:     // funcs
  // called when a newline is encountered
  void newLine() { fileState.newLine(); }

public:      // funcs
  // create a new lexer that will read from to named stream,
  // or stdin if it is NULL
  GrammarLexer(isEmbedTok embedTokTest,
               char const *fname = "<stdin>",
               istream * /*owner*/ source = NULL);

  // clean up
  ~GrammarLexer();

  // get current token as a string
  string curToken() const;
  int curLen() const { return const_cast<GrammarLexer*>(this)->YYLeng(); }

  // current token's embedded text
  string curFuncBody() const;
  string curDeclBody() const { return curFuncBody(); }    // implementation artifact
  string curDeclName() const;

  // read the next token and return its code; returns TOK_EOF for end of file;
  // this function is defined in flex's output source code; this one
  // *does* return TOK_INCLUDE
  virtual int yylex();

  // similar to yylex, but process TOK_INCLUDE internally
  int yylexInc();

  // info about location of current token
  char const *curFname() const { return tokenStartLoc.fname(); }
  int curLine() const { return tokenStartLoc.line; }
  int curCol() const { return tokenStartLoc.col; }
  SourceLocation const &curLoc() const { return tokenStartLoc; }
  string curLocStr() const;    // string with file/line/col

  // error reporting; called by the lexer code
  void err(char const *msg) { reportError(msg); }     // msg should not include a newline
  void errorUnterminatedComment();
  void errorMalformedInclude();
  void errorIllegalCharacter(char ch);

  // for processing includes
  void recursivelyProcess(char const *fname, istream * /*owner*/ source);
  void popRecursiveFile();
  bool hasPendingFiles() const;
  
  // ReportError funcs
  virtual void reportError(char const *msg);
  virtual void reportWarning(char const *msg);
};


#endif // __GRAMLEX_H
