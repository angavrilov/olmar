// paths.h
// AST routines for enumerating paths

#ifndef PATHS_H
#define PATHS_H

class TF_func;     // c.ast
                           
// print all paths in this function
void printPaths(TF_func const *func);

#endif // PATHS_H
