/* gprintf.h
 * generalized printf interface
 * http://www.efgh.com/software/gprintf.htm */

#ifndef GPRINTF_H
#define GPRINTF_H

#include <stdarg.h>      /* va_list */

/* This is called once for each output character.  It returns >=0 for
 * success or <0 for failure, in which case that code will end up as
 * the return value from general_printf.  'extra' is user-defined
 * context, and is passed the same value as the 'extra' arg to
 * general_printf.  'ch' is of course the character to output. */
typedef int (*Gprintf_output_function)(void *extra, int ch);

/* Interpret 'format' and 'args' as printf does, but calling
 * 'output' for each rendered character.  Returns the # of characters
 * output (not including final NUL), or <0 for failure (same code
 * that 'output' returns if it fails). */
int general_vprintf(Gprintf_output_function output,
                    void *extra, const char *format, va_list args);

#endif /* GPRINTF_H */
