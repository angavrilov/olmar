// lexerint.h
// LexerInterface, the interface the GLR parser uses
// to access the lexer's token stream

#ifndef LEXERINT_H
#define LEXERINT_H

#include "useract.h"      // SemanticValue
#include "fileloc.h"      // SourceLocation
#include "str.h"          // string

// This 'interface' is a collection of variables describing
// the current token.  I don't use a bunch of pure-virtual
// functions because of the cost of calling them; everything
// here will be in the inner loop of the parser.
class LexerInterface {
public:     // data
  // token classification; this is what the parser will use to
  // make parsing decisions; this code must correspond to something
  // declared in the 'terminals' section of the grammar; when this
  // is 0, it is the final (end-of-file) token
  int type;

  // semantic value; this is what will be passed to the reduction
  // actions when this token is on the right hand side of a rule
  SemanticValue sval;

  // source location of the token; this will only be used if the
  // parser has been compiled to automatically propagate it
  SourceLocation loc;            // line/col/file

public:     // funcs
  // retrieve the next token; the lexer should respond by filling in
  // the above fields with new values, to describe the next token; the
  // lexer indicates end of file by putting 0 into 'type'; when the
  // LexerInterface object is first passed to the parser, the above
  // fields should already be set correctly (i.e. the parser will make
  // its first call to 'nextToken' *after* processing the first token)
  virtual void nextToken()=0;

  // suppress g++'s (wrong) warning
  virtual ~LexerInterface() {}
  
  
  // The following functions are called to help create diagnostic
  // reports.  They should describe the current token (the one
  // which the above fields refer to) in more-or-less human-readable
  // terms.

  // describe the token; for tokens with multiple spellings (e.g.
  // identifiers), this should include the actual token spelling
  // if possible
  virtual string tokenDesc()=0;

  // describe the token, but using the given type instead of the
  // one which is already stored; this is for when the user's
  // action has reclassified the token
  virtual string tokenDescType(int newType)=0;

};

#endif // LEXERINT_H
