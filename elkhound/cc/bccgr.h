// bccgr.h            see license.txt for copyright and terms of use
// decls shared between bccgr.cc and cc.gr.gen.y

#ifndef BCCGR_H
#define BCCGR_H

#include <stdlib.h>     // free

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
            
// functions called by Bison-parser
void yyerror(char const *msg);
int yylex();

// Bison-parser entry
int yyparse();
extern int yydebug;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BCCGR_H
