// stdarg.h
// variable-argument function support macros

// This file was written by dsw; I (sm) haven't looked at it,
// and I don't know what strategy is envisioned for handling
// these during type checking.

#ifndef __STDARG_H
#define __STDARG_H

typedef void *va_list;

// sm: this is for compatibility with glibc, which seems to assume
// it is using gcc's names
typedef va_list __gnuc_va_list;

void va_start(va_list, void *);

void */*THE TYPECHECKER SHOULD OVERRIDE THIS*/ __va_arg(va_list, int);
#define va_arg(__AP, __TYPE) __va_arg((__AP), sizeof (__TYPE))

void va_end(va_list);

#endif /* __STDARG_H */
