// paths.h
// AST routines for enumerating paths

#ifndef PATHS_H
#define PATHS_H

#include "c.ast.gen.h"      // C AST elements

class Env;         // cc_env.h

// instrument the AST of a function to enable printing (among other
// things) of paths; returns total # of paths in the function (in
// addition to storing that info in the AST)
int countPaths(Env &env, TF_func *func);

// print all paths in this function
void printPaths(TF_func const *func);

// count/print for statements
int countExprPaths(Statement const *stmt, bool isContinue);
void printExprPath(int index, Statement const *stmt, bool isContinue);

// count/print for inititializers
int countExprPaths(Initializer const *init);
void printExprPath(int index, Initializer const *init);

// count/print for expressions
int countPaths(Env &env, Expression *ths);
void printPath(int index, Expression const *ths);

#endif // PATHS_H
