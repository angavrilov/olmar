// trivlex.cc
// trivial lexer (returns each character as a token)
  
#include "lexer2.h"     // Lexer2
#include "syserr.h"     // xsyserror

#include <stdio.h>      // FILE stuff

void trivialLexer(char const *fname, Lexer2 &dest)
{
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    xsyserror("open", fname);
  }                    
  SourceFile *sfile = sourceFileList.open(fname);

  int ch;                
  int col=1;
  while ((ch = fgetc(fp)) != EOF) {
    // abuse Lexer2 to hold chars
    FileLocation floc(1, col++);
    SourceLocation sloc(floc, sfile);
    Lexer2Token *tok = new Lexer2Token((Lexer2TokenType)ch, sloc);
    
    // add it to list
    dest.addToken(tok);
  }
  dest.addEOFToken();
}
