// embedded.h
// interface to an embedded language processor

#ifndef EMBEDDED_H
#define EMBEDDED_H

#include "str.h"        // stringBuilder
#include "reporterr.h"  // ReportError

class EmbeddedLang {
public:
  // for reporting errors
  ReportError &err;

  // all text processed so far; it collects the
  // embedded code; clients will call 'handle' a
  // bunch of times and then expect to retrieve
  // the text from here
  stringBuilder text;

  // when true (set by the lexer), the 'text' is to
  // be interpreted as an expression, rather than a
  // complete function body; this affects what
  // getFuncBody() returns
  bool exprOnly;

  // when true the text is a declaration, so we have to
  // add a single semicolon
  bool isDeclaration;

public:
  EmbeddedLang(ReportError &err);
  virtual ~EmbeddedLang();    // silence warning

  // start from scratch
  virtual void reset() = 0;

  // process the given string of characters, as source text in
  // the embedded language; 'loc' is provided for printing error
  // messages, if desired
  virtual void handle(char const *str, int len) = 0;

  // return true if we're at a nesting level of zero
  // and not in a string, etc. -- characters at this
  // level have "usual" meaning
  virtual bool zeroNesting() const = 0;

  // return the body of the embedded function; should
  // always return a complete function body, even when
  // exprOnly is true (by adding to 'text' if necessary)
  virtual string getFuncBody() const = 0;

  // return the name of the declared function, assuming
  // that is the context in which 'text' was collected
  virtual string getDeclName() const = 0;
};

#endif // EMBEDDED_H
