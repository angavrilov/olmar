// grampar.h
// declarations for bison-generated grammar parser

#ifndef __GRAMPAR_H
#define __GRAMPAR_H

// caller interface to Bison-generated parser; starts parsing
// stdin (?) and returns 0 for success and 1 for error
extern "C" int yyparse();


// Bison calls this to get each token; returns token code,
// or 0 for eof
extern "C" int yylex();

// and this on error
extern "C" void yyerror(char const *message);





#endif // __GRAMPAR_H
