#ifndef __STDARG_H
#define __STDARG_H

typedef void *va_list;

void va_start(va_list, void *);

void */*THE TYPECHECKER SHOULD OVERRIDE THIS*/ __va_arg(va_list, int);
#define va_arg(__AP, __TYPE) __va_arg((__AP), sizeof (__TYPE))

void va_end(va_list);

#endif /* __STDARG_H */
