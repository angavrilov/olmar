
// $Id: builtin-t.h,v 1.3 2006/11/08 12:44:42 tews Exp $

// header file declaring minimal types needed for GCC builtins.

typedef struct _IO_FILE FILE;
typedef unsigned long size_t;
typedef int ssize_t;
typedef __builtin_va_list va_list;
typedef void *__malloc_ptr_t;
struct tm;
#define complex         _Complex
