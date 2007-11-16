/*
 * $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * This file was initially obtained through
 *
 *   g++ -E -o ptab.pre.cpp ptab.cpp
 *
 * but has been edited manually since then.
 *
 * Modifications & caveats to make this code compile with (the current version
 * of) the semantics compiler:
 *
 * - function "panic" added
 * - function "trace" commented out
 */


void panic(...) {}


# 1 "ptab.cpp"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "ptab.cpp"
# 11 "ptab.cpp"
# 1 "assert.h" 1
# 11 "assert.h"
       

# 1 "compiler.h" 1
# 11 "compiler.h"
       
# 24 "compiler.h"
void __builtin_trap(void);
# 14 "assert.h" 2
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 314 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 315 "/usr/include/features.h" 2 3 4
# 337 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4



# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 5 "/usr/include/gnu/stubs.h" 2 3 4


# 1 "/usr/include/gnu/stubs-32.h" 1 3 4
# 8 "/usr/include/gnu/stubs.h" 2 3 4
# 338 "/usr/include/features.h" 2 3 4
# 29 "/usr/include/stdio.h" 2 3 4

extern "C" {



# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 214 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 3 4
typedef unsigned int size_t;
# 35 "/usr/include/stdio.h" 2 3 4

# 1 "/usr/include/bits/types.h" 1 3 4
# 28 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 29 "/usr/include/bits/types.h" 2 3 4


# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 32 "/usr/include/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;




__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;







__extension__ typedef long long int __quad_t;
__extension__ typedef unsigned long long int __u_quad_t;
# 134 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 135 "/usr/include/bits/types.h" 2 3 4


__extension__ typedef __u_quad_t __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef __u_quad_t __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef __quad_t __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef __u_quad_t __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;

__extension__ typedef int __daddr_t;
__extension__ typedef long int __swblk_t;
__extension__ typedef int __key_t;


__extension__ typedef int __clockid_t;


__extension__ typedef void * __timer_t;


__extension__ typedef long int __blksize_t;




__extension__ typedef long int __blkcnt_t;
__extension__ typedef __quad_t __blkcnt64_t;


__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef __u_quad_t __fsblkcnt64_t;


__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef __u_quad_t __fsfilcnt64_t;

__extension__ typedef int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


__extension__ typedef int __intptr_t;


__extension__ typedef unsigned int __socklen_t;
# 37 "/usr/include/stdio.h" 2 3 4









typedef struct _IO_FILE FILE;





# 62 "/usr/include/stdio.h" 3 4
typedef struct _IO_FILE __FILE;
# 72 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/libio.h" 1 3 4
# 32 "/usr/include/libio.h" 3 4
# 1 "/usr/include/_G_config.h" 1 3 4
# 14 "/usr/include/_G_config.h" 3 4
# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 355 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 3 4
typedef unsigned int wint_t;
# 15 "/usr/include/_G_config.h" 2 3 4
# 24 "/usr/include/_G_config.h" 3 4
# 1 "/usr/include/wchar.h" 1 3 4
# 48 "/usr/include/wchar.h" 3 4
# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 49 "/usr/include/wchar.h" 2 3 4

# 1 "/usr/include/bits/wchar.h" 1 3 4
# 51 "/usr/include/wchar.h" 2 3 4
# 76 "/usr/include/wchar.h" 3 4
typedef struct
{
  int __count;
  union
  {
    wint_t __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 25 "/usr/include/_G_config.h" 2 3 4

typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;
# 44 "/usr/include/_G_config.h" 3 4
# 1 "/usr/include/gconv.h" 1 3 4
# 28 "/usr/include/gconv.h" 3 4
# 1 "/usr/include/wchar.h" 1 3 4
# 48 "/usr/include/wchar.h" 3 4
# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 49 "/usr/include/wchar.h" 2 3 4
# 29 "/usr/include/gconv.h" 2 3 4


# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 32 "/usr/include/gconv.h" 2 3 4





enum
{
  __GCONV_OK = 0,
  __GCONV_NOCONV,
  __GCONV_NODB,
  __GCONV_NOMEM,

  __GCONV_EMPTY_INPUT,
  __GCONV_FULL_OUTPUT,
  __GCONV_ILLEGAL_INPUT,
  __GCONV_INCOMPLETE_INPUT,

  __GCONV_ILLEGAL_DESCRIPTOR,
  __GCONV_INTERNAL_ERROR
};



enum
{
  __GCONV_IS_LAST = 0x0001,
  __GCONV_IGNORE_ERRORS = 0x0002
};



struct __gconv_step;
struct __gconv_step_data;
struct __gconv_loaded_object;
struct __gconv_trans_data;



typedef int (*__gconv_fct) (struct __gconv_step *, struct __gconv_step_data *,
       __const unsigned char **, __const unsigned char *,
       unsigned char **, size_t *, int, int);


typedef wint_t (*__gconv_btowc_fct) (struct __gconv_step *, unsigned char);


typedef int (*__gconv_init_fct) (struct __gconv_step *);
typedef void (*__gconv_end_fct) (struct __gconv_step *);



typedef int (*__gconv_trans_fct) (struct __gconv_step *,
      struct __gconv_step_data *, void *,
      __const unsigned char *,
      __const unsigned char **,
      __const unsigned char *, unsigned char **,
      size_t *);


typedef int (*__gconv_trans_context_fct) (void *, __const unsigned char *,
       __const unsigned char *,
       unsigned char *, unsigned char *);


typedef int (*__gconv_trans_query_fct) (__const char *, __const char ***,
     size_t *);


typedef int (*__gconv_trans_init_fct) (void **, const char *);
typedef void (*__gconv_trans_end_fct) (void *);

struct __gconv_trans_data
{

  __gconv_trans_fct __trans_fct;
  __gconv_trans_context_fct __trans_context_fct;
  __gconv_trans_end_fct __trans_end_fct;
  void *__data;
  struct __gconv_trans_data *__next;
};



struct __gconv_step
{
  struct __gconv_loaded_object *__shlib_handle;
  __const char *__modname;

  int __counter;

  char *__from_name;
  char *__to_name;

  __gconv_fct __fct;
  __gconv_btowc_fct __btowc_fct;
  __gconv_init_fct __init_fct;
  __gconv_end_fct __end_fct;



  int __min_needed_from;
  int __max_needed_from;
  int __min_needed_to;
  int __max_needed_to;


  int __stateful;

  void *__data;
};



struct __gconv_step_data
{
  unsigned char *__outbuf;
  unsigned char *__outbufend;



  int __flags;



  int __invocation_counter;



  int __internal_use;

  __mbstate_t *__statep;
  __mbstate_t __state;



  struct __gconv_trans_data *__trans;
};



typedef struct __gconv_info
{
  size_t __nsteps;
  struct __gconv_step *__steps;
  __extension__ struct __gconv_step_data __data [];
} *__gconv_t;
# 45 "/usr/include/_G_config.h" 2 3 4
typedef union
{
  struct __gconv_info __cd;
  struct
  {
    struct __gconv_info __cd;
    struct __gconv_step_data __data;
  } __combined;
} _G_iconv_t;

typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));
# 33 "/usr/include/libio.h" 2 3 4
# 53 "/usr/include/libio.h" 3 4
# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stdarg.h" 1 3 4
# 43 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 54 "/usr/include/libio.h" 2 3 4
# 166 "/usr/include/libio.h" 3 4
struct _IO_jump_t; struct _IO_FILE;
# 176 "/usr/include/libio.h" 3 4
typedef void _IO_lock_t;





struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;



  int _pos;
# 199 "/usr/include/libio.h" 3 4
};


enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};
# 267 "/usr/include/libio.h" 3 4
struct _IO_FILE {
  int _flags;




  char* _IO_read_ptr;
  char* _IO_read_end;
  char* _IO_read_base;
  char* _IO_write_base;
  char* _IO_write_ptr;
  char* _IO_write_end;
  char* _IO_buf_base;
  char* _IO_buf_end;

  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;



  int _flags2;

  __off_t _old_offset;



  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];



  _IO_lock_t *_lock;
# 315 "/usr/include/libio.h" 3 4
  __off64_t _offset;





  void *__pad1;
  void *__pad2;

  int _mode;

  char _unused2[15 * sizeof (int) - 2 * sizeof (void *)];

};





struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;
# 354 "/usr/include/libio.h" 3 4
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);







typedef __ssize_t __io_write_fn (void *__cookie, __const char *__buf,
     size_t __n);







typedef int __io_seek_fn (void *__cookie, __off64_t *__pos, int __w);


typedef int __io_close_fn (void *__cookie);




typedef __io_read_fn cookie_read_function_t;
typedef __io_write_fn cookie_write_function_t;
typedef __io_seek_fn cookie_seek_function_t;
typedef __io_close_fn cookie_close_function_t;


typedef struct
{
  __io_read_fn *read;
  __io_write_fn *write;
  __io_seek_fn *seek;
  __io_close_fn *close;
} _IO_cookie_io_functions_t;
typedef _IO_cookie_io_functions_t cookie_io_functions_t;

struct _IO_cookie_file;


extern void _IO_cookie_init (struct _IO_cookie_file *__cfile, int __read_write,
        void *__cookie, _IO_cookie_io_functions_t __fns);




extern "C" {


extern int __underflow (_IO_FILE *) throw ();
extern int __uflow (_IO_FILE *) throw ();
extern int __overflow (_IO_FILE *, int) throw ();
extern wint_t __wunderflow (_IO_FILE *) throw ();
extern wint_t __wuflow (_IO_FILE *) throw ();
extern wint_t __woverflow (_IO_FILE *, wint_t) throw ();
# 444 "/usr/include/libio.h" 3 4
extern int _IO_getc (_IO_FILE *__fp) throw ();
extern int _IO_putc (int __c, _IO_FILE *__fp) throw ();
extern int _IO_feof (_IO_FILE *__fp) throw ();
extern int _IO_ferror (_IO_FILE *__fp) throw ();

extern int _IO_peekc_locked (_IO_FILE *__fp) throw ();





extern void _IO_flockfile (_IO_FILE *) throw ();
extern void _IO_funlockfile (_IO_FILE *) throw ();
extern int _IO_ftrylockfile (_IO_FILE *) throw ();
# 474 "/usr/include/libio.h" 3 4
extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
   __gnuc_va_list, int *__restrict) throw ();
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
    __gnuc_va_list) throw ();
extern __ssize_t _IO_padn (_IO_FILE *, int, __ssize_t) throw ();
extern size_t _IO_sgetn (_IO_FILE *, void *, size_t) throw ();

extern __off64_t _IO_seekoff (_IO_FILE *, __off64_t, int, int) throw ();
extern __off64_t _IO_seekpos (_IO_FILE *, __off64_t, int) throw ();

extern void _IO_free_backup_area (_IO_FILE *) throw ();
# 532 "/usr/include/libio.h" 3 4
}
# 73 "/usr/include/stdio.h" 2 3 4




typedef __gnuc_va_list va_list;
# 86 "/usr/include/stdio.h" 3 4


typedef _G_fpos_t fpos_t;





typedef _G_fpos64_t fpos64_t;
# 138 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 139 "/usr/include/stdio.h" 2 3 4



extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;









extern int remove (__const char *__filename) throw ();

extern int rename (__const char *__old, __const char *__new) throw ();









extern FILE *tmpfile (void);
# 176 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile64 (void);



extern char *tmpnam (char *__s) throw ();





extern char *tmpnam_r (char *__s) throw ();
# 198 "/usr/include/stdio.h" 3 4
extern char *tempnam (__const char *__dir, __const char *__pfx)
     throw () __attribute__ ((__malloc__));








extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 223 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 233 "/usr/include/stdio.h" 3 4
extern int fcloseall (void);









extern FILE *fopen (__const char *__restrict __filename,
      __const char *__restrict __modes);




extern FILE *freopen (__const char *__restrict __filename,
        __const char *__restrict __modes,
        FILE *__restrict __stream);
# 264 "/usr/include/stdio.h" 3 4


extern FILE *fopen64 (__const char *__restrict __filename,
        __const char *__restrict __modes);
extern FILE *freopen64 (__const char *__restrict __filename,
   __const char *__restrict __modes,
   FILE *__restrict __stream);




extern FILE *fdopen (int __fd, __const char *__modes) throw ();





extern FILE *fopencookie (void *__restrict __magic_cookie,
     __const char *__restrict __modes,
     _IO_cookie_io_functions_t __io_funcs) throw ();


extern FILE *fmemopen (void *__s, size_t __len, __const char *__modes) throw ();




extern FILE *open_memstream (char **__restrict __bufloc,
        size_t *__restrict __sizeloc) throw ();






extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) throw ();



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) throw ();





extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) throw ();


extern void setlinebuf (FILE *__stream) throw ();








extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...) throw ();





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg) throw ();





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     throw () __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     throw () __attribute__ ((__format__ (__printf__, 3, 0)));






extern int vasprintf (char **__restrict __ptr, __const char *__restrict __f,
        __gnuc_va_list __arg)
     throw () __attribute__ ((__format__ (__printf__, 2, 0)));
extern int __asprintf (char **__restrict __ptr,
         __const char *__restrict __fmt, ...)
     throw () __attribute__ ((__format__ (__printf__, 2, 3)));
extern int asprintf (char **__restrict __ptr,
       __const char *__restrict __fmt, ...)
     throw () __attribute__ ((__format__ (__printf__, 2, 3)));







extern int vdprintf (int __fd, __const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, __const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));








extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...) ;




extern int scanf (__const char *__restrict __format, ...) ;

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) throw () ;








extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) ;





extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (__const char *__restrict __s,
      __const char *__restrict __format, __gnuc_va_list __arg)
     throw () __attribute__ ((__format__ (__scanf__, 2, 0))) ;









extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);

# 456 "/usr/include/stdio.h" 3 4
extern int getc_unlocked (FILE *__stream);
extern int getchar_unlocked (void);
# 467 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream);











extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);

# 500 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream);







extern int putc_unlocked (int __c, FILE *__stream);
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream);


extern int putw (int __w, FILE *__stream);








extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     ;






extern char *gets (char *__s) ;

# 546 "/usr/include/stdio.h" 3 4
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream) ;
# 562 "/usr/include/stdio.h" 3 4
extern __ssize_t __getdelim (char **__restrict __lineptr,
          size_t *__restrict __n, int __delimiter,
          FILE *__restrict __stream) ;
extern __ssize_t getdelim (char **__restrict __lineptr,
        size_t *__restrict __n, int __delimiter,
        FILE *__restrict __stream) ;







extern __ssize_t getline (char **__restrict __lineptr,
       size_t *__restrict __n,
       FILE *__restrict __stream) ;








extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream) ;






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream) ;




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) ;

# 623 "/usr/include/stdio.h" 3 4
extern int fputs_unlocked (__const char *__restrict __s,
      FILE *__restrict __stream);
# 634 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream) ;
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream) ;








extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream) ;




extern void rewind (FILE *__stream);

# 670 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off_t __off, int __whence);




extern __off_t ftello (FILE *__stream) ;
# 689 "/usr/include/stdio.h" 3 4






extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos);




extern int fsetpos (FILE *__stream, __const fpos_t *__pos);
# 712 "/usr/include/stdio.h" 3 4



extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
extern __off64_t ftello64 (FILE *__stream) ;
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos);
extern int fsetpos64 (FILE *__stream, __const fpos64_t *__pos);




extern void clearerr (FILE *__stream) throw ();

extern int feof (FILE *__stream) throw ();

extern int ferror (FILE *__stream) throw ();




extern void clearerr_unlocked (FILE *__stream) throw ();
extern int feof_unlocked (FILE *__stream) throw ();
extern int ferror_unlocked (FILE *__stream) throw ();








extern void perror (__const char *__s);






# 1 "/usr/include/bits/sys_errlist.h" 1 3 4
# 27 "/usr/include/bits/sys_errlist.h" 3 4
extern int sys_nerr;
extern __const char *__const sys_errlist[];


extern int _sys_nerr;
extern __const char *__const _sys_errlist[];
# 751 "/usr/include/stdio.h" 2 3 4




extern int fileno (FILE *__stream) throw ();




extern int fileno_unlocked (FILE *__stream) throw ();
# 770 "/usr/include/stdio.h" 3 4
extern FILE *popen (__const char *__command, __const char *__modes);





extern int pclose (FILE *__stream);





extern char *ctermid (char *__s) throw ();





extern char *cuserid (char *__s);




struct obstack;


extern int obstack_printf (struct obstack *__restrict __obstack,
      __const char *__restrict __format, ...)
     throw () __attribute__ ((__format__ (__printf__, 2, 3)));
extern int obstack_vprintf (struct obstack *__restrict __obstack,
       __const char *__restrict __format,
       __gnuc_va_list __args)
     throw () __attribute__ ((__format__ (__printf__, 2, 0)));







extern void flockfile (FILE *__stream) throw ();



extern int ftrylockfile (FILE *__stream) throw () ;


extern void funlockfile (FILE *__stream) throw ();
# 837 "/usr/include/stdio.h" 3 4
}
# 15 "assert.h" 2
# 12 "ptab.cpp" 2
# 1 "buddy.h" 1
# 11 "buddy.h"
       


# 1 "memory.h" 1
# 11 "memory.h"
       
# 15 "buddy.h" 2
# 1 "spinlock.h" 1
# 11 "spinlock.h"
       


# 1 "types.h" 1
# 11 "types.h"
       

# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 152 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 3 4
typedef int ptrdiff_t;
# 14 "types.h" 2

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

typedef unsigned long mword;
typedef unsigned long long Paddr;
# 15 "spinlock.h" 2

class Spinlock
{
    private:
        uint8 _lock;

    public:
        __attribute__((always_inline))
        inline Spinlock() : _lock (1) {}

        __attribute__((always_inline))
        inline void lock()
        {
            asm volatile ("1:   lock; decb %0;  "
                          "     jns 3f;         "
                          "2:   pause;          "
                          "     cmpb $0, %0;    "
                          "     jle 2b;         "
                          "     jmp 1b;         "
                          "3:                   "
                          : "+m" (_lock) : : "memory");
        }

        __attribute__((always_inline))
        inline void unlock()
        {
            asm volatile ("movb $1, %0" : "=m" (_lock) : : "memory");
        }
};
# 16 "buddy.h" 2


class Buddy
{
    private:
        class Block
        {
            public:
                Block * prev;
                Block * next;
                unsigned short ord;
                unsigned short tag;

                enum {
                    Used = 0,
                    Free = 1
                };
        };

        Spinlock lock;
        signed long max_idx;
        signed long min_idx;
        mword p_base;
        mword l_base;
        unsigned order;
        Block * index;
        Block * head;

        __attribute__((always_inline))
        inline signed long block_to_index (Block *b)
        {
            return b - index;
        }

        __attribute__((always_inline))
        inline Block *index_to_block (signed long i)
        {
            return index + i;
        }

        __attribute__((always_inline))
        inline signed long page_to_index (mword l_addr)
        {
            return l_addr / 0x1000 - l_base / 0x1000;
        }

        __attribute__((always_inline))
        inline mword index_to_page (signed long i)
        {
            return l_base + i * 0x1000;
        }

        __attribute__((always_inline))
        inline mword virt_to_phys (mword virt)
        {
            return virt - l_base + p_base;
        }

        __attribute__((always_inline))
        inline mword phys_to_virt (mword phys)
        {
            return phys - p_base + l_base;
        }

    public:
        enum Fill
        {
            NOFILL,
            FILL_0,
            FILL_1
        };

        static Buddy allocator;

        __attribute__((section (".init")))
        Buddy (mword phys, mword virt, mword f_addr, size_t size);

        void *alloc (unsigned short ord, Fill fill = NOFILL);

        void free (mword addr);

        __attribute__((always_inline))
        static inline void *phys_to_ptr (Paddr phys)
        {
            return reinterpret_cast<void *>(allocator.phys_to_virt (static_cast<mword>(phys)));
        }

        __attribute__((always_inline))
        static inline mword ptr_to_phys (void *virt)
        {
            return allocator.virt_to_phys (reinterpret_cast<mword>(virt));
        }
};
# 13 "ptab.cpp" 2

# 1 "pd.h" 1
# 11 "pd.h"
       


# 1 "slab.h" 1
# 11 "slab.h"
       

# 1 "bits.h" 1
# 11 "bits.h"
       





__attribute__((always_inline))
inline int bit_scan_reverse (unsigned val)
{
    if (__builtin_expect((!val), false))
        return -1;

    asm volatile ("bsr %1, %0" : "=r" (val) : "rm" (val));

    return val;
}

__attribute__((always_inline))
inline uint64 div64 (uint64 n, uint32 d, uint32 *r)
{
    uint64 q;
    asm volatile ("divl %5;"
                  "xchg %1, %2;"
                  "divl %5;"
                  "xchg %1, %3;"
                  : "=A" (q),
                    "=r" (*r)
                  : "a" (static_cast<uint32>(n >> 32)),
                    "d" (0),
                    "1" (static_cast<uint32>(n)),
                    "rm" (d));
    return q;
}

__attribute__((always_inline))
static inline unsigned align_down (mword val, unsigned align)
{
    val &= ~(align - 1);
    return val;
}

__attribute__((always_inline))
static inline unsigned align_up (mword val, unsigned align)
{
    val += (align - 1);
    return align_down (val, align);
}
# 14 "slab.h" 2





class Slab;

class Slab_cache
{
    private:
        Spinlock lock;
        Slab * curr;
        Slab * head;




        void grow();

    public:
        unsigned size;
        unsigned buff;
        unsigned elem;

        Slab_cache (char const *name, size_t elem_size, unsigned elem_align);




        void *alloc();




        void free (void *ptr);




        void reap();
};

class Slab
{
    public:
        unsigned avail;
        Slab_cache * cache;
        Slab * prev;
        Slab * next;
        char * head;

        __attribute__((always_inline))
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        __attribute__((always_inline))
        static inline void operator delete (void *ptr)
        {
            Buddy::allocator.free (reinterpret_cast<mword>(ptr));
        }

        Slab (Slab_cache *slab_cache);

        __attribute__((always_inline))
        inline bool full() const
        {
            return !avail;
        }

        __attribute__((always_inline))
        inline bool empty() const
        {
            return avail == cache->elem;
        }

        void enqueue();
        void dequeue();

        __attribute__((always_inline))
        inline void *alloc();

        __attribute__((always_inline))
        inline void free (void *ptr);
};
# 15 "pd.h" 2
# 1 "space_io.h" 1
# 11 "space_io.h"
       




class Exc_state;
class Space_mem;

class Space_io
{
    private:
        Space_mem *space_mem;

        __attribute__((always_inline))
        inline mword idx_to_virt (unsigned idx)
        {
            return 0xd0000000 + idx / 8;
        }

        __attribute__((always_inline))
        inline mword idx_to_bit (unsigned idx)
        {
            return idx % (8 * sizeof (mword));
        }

    public:
        __attribute__((always_inline))
        inline Space_io (Space_mem *space) : space_mem (space) {}

        void insert (unsigned idx);
        void remove (unsigned idx);

        static void page_fault (Exc_state *xs);
};
# 16 "pd.h" 2
# 1 "space_mem.h" 1
# 11 "space_mem.h"
       


# 1 "config.h" 1
# 11 "config.h"
       
# 15 "space_mem.h" 2
# 1 "cpu.h" 1
# 11 "cpu.h"
       



# 1 "vectors.h" 1
# 11 "vectors.h"
       
# 16 "cpu.h" 2

class Cpu
{
    private:
        static char const * const vendor_string[];

        __attribute__((always_inline))
        static inline void check_features();

        __attribute__((always_inline))
        static inline void setup_thermal();

        __attribute__((always_inline))
        static inline void setup_sysenter();

    public:
        enum Vendor
        {
            VENDOR_UNKNOWN,
            VENDOR_INTEL,
            VENDOR_AMD
        };

        enum Feature
        {
            FEAT_FPU = 0,
            FEAT_VME = 1,
            FEAT_DE = 2,
            FEAT_PSE = 3,
            FEAT_TSC = 4,
            FEAT_MSR = 5,
            FEAT_PAE = 6,
            FEAT_MCE = 7,
            FEAT_CX8 = 8,
            FEAT_APIC = 9,
            FEAT_SEP = 11,
            FEAT_MTRR = 12,
            FEAT_PGE = 13,
            FEAT_MCA = 14,
            FEAT_CMOV = 15,
            FEAT_PAT = 16,
            FEAT_PSE36 = 17,
            FEAT_PSN = 18,
            FEAT_CLFSH = 19,
            FEAT_DS = 21,
            FEAT_ACPI = 22,
            FEAT_MMX = 23,
            FEAT_FXSR = 24,
            FEAT_SSE = 25,
            FEAT_SSE2 = 26,
            FEAT_SS = 27,
            FEAT_HTT = 28,
            FEAT_TM = 29,
            FEAT_PBE = 31,
            FEAT_SSE3 = 32,
            FEAT_MONITOR = 35,
            FEAT_DSCPL = 36,
            FEAT_VMX = 37,
            FEAT_SMX = 38,
            FEAT_EST = 39,
            FEAT_TM2 = 40,
            FEAT_SSSE3 = 41,
            FEAT_CNXTID = 42,
            FEAT_CX16 = 45,
            FEAT_XTPR = 46,
            FEAT_PDCM = 47,
            FEAT_DCA = 50,
            FEAT_SSE4_1 = 51,
            FEAT_SSE4_2 = 52,
            FEAT_APIC_2 = 53,
            FEAT_POPCNT = 55,
            FEAT_SYSCALL = 75,
            FEAT_XD = 84,
            FEAT_MMX_EXT = 86,
            FEAT_FFXSR = 89,
            FEAT_RDTSCP = 91,
            FEAT_EM64T = 93,
            FEAT_3DNOW_EXT = 94,
            FEAT_3DNOW = 95,
            FEAT_LAHF_SAHF = 96,
            FEAT_CMP_LEGACY = 97,
            FEAT_SVM = 98,
            FEAT_EXT_APIC = 99,
            FEAT_LOCK_MOV_CR0 = 100
        };

        enum
        {
            CR0_PE = 1ul << 0,
            CR0_MP = 1ul << 1,
            CR0_NE = 1ul << 5,
            CR0_WP = 1ul << 16,
            CR0_AM = 1ul << 18,
            CR0_NW = 1ul << 29,
            CR0_CD = 1ul << 30,
            CR0_PG = 1ul << 31
        };

        enum
        {
            CR4_DE = 1ul << 3,
            CR4_PSE = 1ul << 4,
            CR4_PAE = 1ul << 5,
            CR4_PGE = 1ul << 7,
            CR4_SMXE = 1ul << 14,
        };

        enum
        {
            EFL_CF = 1ul << 0,
            EFL_PF = 1ul << 2,
            EFL_AF = 1ul << 4,
            EFL_ZF = 1ul << 6,
            EFL_SF = 1ul << 7,
            EFL_TF = 1ul << 8,
            EFL_IF = 1ul << 9,
            EFL_DF = 1ul << 10,
            EFL_OF = 1ul << 11,
            EFL_IOPL = 3ul << 12,
            EFL_NT = 1ul << 14,
            EFL_RF = 1ul << 16,
            EFL_VM = 1ul << 17,
            EFL_AC = 1ul << 18,
            EFL_VIF = 1ul << 19,
            EFL_VIP = 1ul << 20,
            EFL_ID = 1ul << 21
        };

        static unsigned boot_lock asm ("boot_lock");
        static unsigned booted;
        static unsigned secure;

        static unsigned id __attribute__((weak));
        static unsigned package ;
        static unsigned core ;
        static unsigned thread ;

        static Vendor vendor ;
        static unsigned platform ;
        static unsigned family ;
        static unsigned model ;
        static unsigned stepping ;
        static unsigned brand ;
        static unsigned patch ;

        static uint32 name[12] ;
        static uint32 features[4] ;
        static bool bsp ;

        static unsigned spinner ;
        static unsigned lapic_timer_count ;
        static unsigned lapic_error_count ;
        static unsigned lapic_perfm_count ;
        static unsigned lapic_therm_count ;
        static unsigned irqs[48] ;

        static void init();
        static void wakeup_ap();
        static void delay (unsigned us);

        __attribute__((always_inline))
        static inline bool feature (Feature feat)
        {
            return features[feat / 32] & 1u << feat % 32;
        }

        __attribute__((always_inline))
        static inline void set_feature (unsigned feat)
        {
            features[feat / 32] |= 1u << feat % 32;
        }

        __attribute__((always_inline))
        static inline void pause()
        {
            asm volatile ("pause");
        }

        __attribute__((always_inline))
        static inline void halt()
        {
            asm volatile ("sti; hlt");
        }

        __attribute__((always_inline))
        static inline uint64 time()
        {
            uint64 val;
            asm volatile ("rdtsc" : "=A" (val));
            return val;
        }

        __attribute__((always_inline)) __attribute__((noreturn))
        static inline void shutdown()
        {
            for (;;)
                asm volatile ("cli; hlt");
        }

        __attribute__((always_inline))
        static inline void cpuid (unsigned leaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf));
        }

        __attribute__((always_inline))
        static inline void cpuid (unsigned leaf, unsigned subleaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf), "c" (subleaf));
        }

        __attribute__((always_inline))
        static inline mword get_cr0()
        {
            mword cr0;
            asm volatile ("mov %%cr0, %0" : "=r" (cr0));
            return cr0;
        }

        __attribute__((always_inline))
        static inline void set_cr0 (mword cr0)
        {
            asm volatile ("mov %0, %%cr0" : : "r" (cr0));
        }

        __attribute__((always_inline))
        static inline mword get_cr4()
        {
            mword cr4;
            asm volatile ("mov %%cr4, %0" : "=r" (cr4));
            return cr4;
        }

        __attribute__((always_inline))
        static inline void set_cr4 (mword cr4)
        {
            asm volatile ("mov %0, %%cr4" : : "r" (cr4));
        }

        __attribute__((always_inline))
        static inline void flush_cache()
        {
            asm volatile ("wbinvd" : : : "memory");
        }
};
# 16 "space_mem.h" 2
# 1 "ptab.h" 1
# 11 "ptab.h"
       




# 1 "paging.h" 1
# 11 "paging.h"
       




class Paging
{
    public:
        enum Error
        {
            ERROR_PROT = 1u << 0,
            ERROR_WRITE = 1u << 1,
            ERROR_USER = 1u << 2,
            ERROR_RESERVED = 1u << 3,
            ERROR_IFETCH = 1u << 4
        };

        enum Attribute
        {
            ATTR_NONE = 0,
            ATTR_PRESENT = 1ull << 0,
            ATTR_WRITABLE = 1ull << 1,
            ATTR_USER = 1ull << 2,
            ATTR_WRITE_THROUGH = 1ull << 3,
            ATTR_UNCACHEABLE = 1ull << 4,
            ATTR_ACCESSED = 1ull << 5,
            ATTR_DIRTY = 1ull << 6,
            ATTR_SUPERPAGE = 1ull << 7,
            ATTR_GLOBAL = 1ull << 8,
            ATTR_NOEXEC = 1ull << 63,

            ATTR_PTAB = ATTR_ACCESSED |
                                      ATTR_USER |
                                      ATTR_WRITABLE |
                                      ATTR_PRESENT,

            ATTR_LEAF = ATTR_DIRTY |
                                      ATTR_ACCESSED |
                                      ATTR_PRESENT,

            ATTR_INVERTED = ATTR_NOEXEC,

            ATTR_ALL = ATTR_NOEXEC | ((1ull << 12) - 1)
        };

        static unsigned const levels = 3;
        static unsigned const bits_per_level = 9;
        static Attribute ptab_bits[levels] asm ("ptab_bits");

        __attribute__((section (".init")))
        static void check_features();

        __attribute__((section (".init")))
        static void enable();

    protected:
        Paddr val;

        __attribute__((always_inline))
        inline Paddr addr() const
        {
            return val & ~ATTR_ALL;
        }

        __attribute__((always_inline))
        inline Attribute attr() const
        {
            return Attribute (val & ATTR_ALL);
        }

        __attribute__((always_inline))
        inline bool present() const
        {
            return val & ATTR_PRESENT;
        }

        __attribute__((always_inline))
        inline bool superpage() const
        {
            return val & ATTR_SUPERPAGE;
        }
};
# 17 "ptab.h" 2



class Ptab : public Paging
{
    private:
        static Slab_cache cache;
        static Paddr remap_addr;

    public:
        __attribute__((always_inline))
        static inline void *operator new (size_t, bool small = false)
        {
            if (small)
                return cache.alloc();

            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        __attribute__((always_inline))
        static inline Ptab *master()
        {
            extern Ptab _master_l;
            return &_master_l;
        }

        __attribute__((always_inline))
        static inline Ptab *current()
        {
            mword addr;
            asm volatile ("mov %%cr3, %0" : "=r" (addr));
            return static_cast<Ptab *>(Buddy::phys_to_ptr (addr));
        }

        __attribute__((always_inline))
        inline void make_current()
        {
            do { if (__builtin_expect((!(this)), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "this", "ptab.h", 54); } while (0);
            asm volatile ("mov %0, %%cr3" : : "r" (Buddy::ptr_to_phys (this)) : "memory");
        }

        void map (Paddr phys, mword virt, size_t size, Attribute attr = ATTR_NONE);

        void unmap (mword virt, size_t size);

        bool lookup (mword virt, Paddr &phys);

        void sync_local();

        size_t sync_master (mword virt);

        void sync_master_range (mword s_addr, mword e_addr);

        void *remap (Paddr phys);
};
# 17 "space_mem.h" 2


class Exc_state;

class Space_mem
{
    protected:
        Ptab *master;
        Ptab *percpu[8];

    public:

        Space_mem();


        Space_mem (Ptab *mst) : master (mst) {}

        __attribute__((always_inline))
        inline Ptab *mst_ptab() const
        {
            return master;
        }

        __attribute__((always_inline))
        inline Ptab *cpu_ptab() const
        {
            return percpu[Cpu::id];
        }

        __attribute__((always_inline))
        inline void mem_map_local (Paddr phys, mword virt, size_t size, Paging::Attribute attr)
        {
            percpu[Cpu::id]->map (phys, virt, size, attr);
        }

        __attribute__((always_inline))
        inline void insert (Paddr phys, mword virt, size_t size, Paging::Attribute attr)
        {
            master->map (phys, virt, size, attr);
        }

        __attribute__((always_inline))
        inline void remove (mword virt, size_t size)
        {
            master->unmap (virt, size);
        }

        __attribute__((always_inline))
        inline bool lookup (mword virt, Paddr &phys)
        {
            return master->lookup (virt, phys);
        }

        static void page_fault (Exc_state *xs);
};
# 17 "pd.h" 2
# 1 "space_obj.h" 1
# 11 "space_obj.h"
       

# 1 "capability.h" 1
# 11 "capability.h"
       





class Capability
{
    private:
        mword val;

    public:
        enum Type
        {
            INVALID,
            PD,
            WQ,
            EC,
            PT
        };

        Capability (Type t, void *p = 0) : val (reinterpret_cast<mword>(p) | t) {}

        __attribute__((always_inline))
        inline Type type() const
        {
            return static_cast<Type>(val & 7);
        }

        __attribute__((always_inline))
        inline void *ptr() const
        {
            return reinterpret_cast<void *>(val & ~7);
        }
};
# 14 "space_obj.h" 2



class Exc_state;
class Space_mem;

class Space_obj
{
    private:
        Space_mem *space_mem;

        __attribute__((always_inline))
        inline mword idx_to_virt (unsigned idx)
        {
            return 0xe0000000 + idx * sizeof (Capability);
        }

    public:
        __attribute__((always_inline))
        inline Space_obj (Space_mem *space) : space_mem (space) {}

        __attribute__((always_inline))
        inline Capability lookup (unsigned idx)
        {
            return *reinterpret_cast<Capability *>(idx_to_virt (idx));
        }

        bool insert (unsigned idx, Capability cap);
        void remove (unsigned idx);

        static void page_fault (Exc_state *xs);
};
# 18 "pd.h" 2


class Pd : public Space_mem, public Space_io, public Space_obj
{
    private:

        static Slab_cache cache;

    public:

        static Pd *current ;


        static Pd kern;


        __attribute__((always_inline))
        static inline void *operator new (size_t)
        {
            return cache.alloc();
        }


        __attribute__((always_inline))
        static inline void operator delete (void *ptr)
        {
            cache.free (ptr);
        }


        Pd (char const *name);


        Pd (Ptab *mst) : Space_mem (mst), Space_io (this), Space_obj (this) {}

        void make_current();

        __attribute__((always_inline))
        static inline void init_cpu_ptab (Ptab *ptab)
        {
            kern.percpu[Cpu::id] = ptab;
        }
};
# 15 "ptab.cpp" 2


# 1 "tlb.h" 1
# 11 "tlb.h"
       




class Tlb
{
    public:
        __attribute__((always_inline))
        static inline void flush()
        {
            mword cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        __attribute__((always_inline))
        static inline void flush (mword virt)
        {
            asm volatile ("invlpg %0" : : "m" (*reinterpret_cast<mword *>(virt)));
        }
};
# 18 "ptab.cpp" 2


Slab_cache Ptab::cache ("Ptab", 32, 32);
Paddr Ptab::remap_addr = ~0ull;







void Ptab::map (Paddr phys, mword virt, size_t size, Attribute attrib)
{
    while (size) {


      /*
        trace (TRACE_PTAB, "(M) %#010lx LIN:%#010lx PHY:%#010llx A:%#05llx S:%#x",
               Buddy::ptr_to_phys (this),
               virt,
               static_cast<uint64>(phys),
               static_cast<uint64>(attrib),
               size);
      */

        unsigned lev = levels;

        for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

            unsigned shift = --lev * bits_per_level + 12;
            pte += virt >> shift & ((1ul << bits_per_level) - 1);
            size_t mask = (1ul << shift) - 1;

            if (size > mask && !((phys | virt) & mask)) {

                Paddr a = attrib | ATTR_LEAF;

                if (lev)
                    a |= ATTR_SUPERPAGE;

                bool flush_tlb = false;

                if (pte->present()) {


                    if (lev && !pte->superpage())
                        panic ("Overmap PT with SP\n");


                    if (pte->addr() != phys || (pte->val ^ ATTR_INVERTED) & (pte->val ^ a) & ATTR_ALL)
                        flush_tlb = true;
                }


                pte->val = phys | (a & ptab_bits[lev]);

                if (flush_tlb)
                    Tlb::flush (virt);


		/*
                trace (TRACE_PTAB, "   -> L:%u S:%u PTE:%#010lx PHY:%#010llx A:%#05llx FRAME",
                       lev, shift,
                       Buddy::ptr_to_phys (pte),
                       static_cast<uint64>(pte->addr()),
                       static_cast<uint64>(pte->attr()));
		*/


                mask++;
                size -= mask;
                virt += mask;
                phys += mask;
                break;
            }

            if (!lev)
                panic ("Unsupported SIZE\n");


            if (!pte->present())
                pte->val = Buddy::ptr_to_phys (new Ptab) | (ATTR_PTAB & ptab_bits[lev]);

            else if (pte->superpage())
                panic ("Overmap SP with PT\n");


	    /*
            trace (TRACE_PTAB, "   -> L:%u S:%u PTE:%#010lx PHY:%#010llx A:%#05llx TABLE",
                   lev, shift,
                   Buddy::ptr_to_phys (pte),
                   static_cast<uint64>(pte->addr()),
                   static_cast<uint64>(pte->attr()));
	    */

        }
    }
}
# 121 "ptab.cpp"
void Ptab::unmap (mword virt, size_t size)
{
    while (size) {


      /*
        trace (TRACE_PTAB, "(U) %#010lx LIN:%#010lx SIZE:%#x",
               Buddy::ptr_to_phys (this), virt, size);
      */


        unsigned lev = levels;

        for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

            unsigned shift = --lev * bits_per_level + 12;
            pte += virt >> shift & ((1ul << bits_per_level) - 1);

            if (pte->present()) {


	      /*
                trace (TRACE_PTAB, "   -> L:%u S:%u PTE:%#010lx PHY:%#010llx A:%#05llx %s",
                       lev, shift,
                       Buddy::ptr_to_phys (pte),
                       static_cast<uint64>(pte->addr()),
                       static_cast<uint64>(pte->attr()),
                       lev && !pte->superpage() ? "TABLE" : "FRAME");
	      */


                if (lev && !pte->superpage())
                    continue;


                pte->val = 0;

                Tlb::flush (virt);
            }

            size_t mask = 1ul << shift;
            size -= mask;
            virt += mask;
            break;
        }
    }
}

bool Ptab::lookup (mword virt, Paddr &phys)
{
    unsigned lev = levels;

    for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        pte += virt >> shift & ((1ul << bits_per_level) - 1);

        if (!pte->present())
            return false;

        if (lev && !pte->superpage())
            continue;

        phys = pte->addr();

        return true;
    }
}

void Ptab::sync_local()
{
    unsigned lev = levels;
    Ptab *pte, *mst;

    for (pte = this, mst = Pd::kern.cpu_ptab();;
         pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Ptab *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        unsigned slot = 0xcfe00000 >> shift & ((1ul << bits_per_level) - 1);
        size_t size = 1ul << shift;

        mst += slot;
        do { if (__builtin_expect((!(mst->present())), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "mst->present()", "ptab.cpp", 200); } while (0);

        pte += slot;
        do { if (__builtin_expect((!(!pte->present())), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "!pte->present()", "ptab.cpp", 203); } while (0);

        if (size <= 0xd0000000 - 0xcfe00000) {
            *pte = *mst;
            break;
        }

        pte->val = Buddy::ptr_to_phys (new Ptab) | (ATTR_PTAB & ptab_bits[lev]);
    }
}

size_t Ptab::sync_master (mword virt)
{
    unsigned lev = levels;
    Ptab *pte, *mst;

    for (pte = this, mst = Ptab::master();;
         pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Ptab *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        unsigned slot = virt >> shift & ((1ul << bits_per_level) - 1);
        size_t size = 1ul << shift;

        mst += slot;
        pte += slot;

        if (mst->present()) {

            if (slot == (0xcfe00000 >> shift & ((1ul << bits_per_level) - 1))) {
                do { if (__builtin_expect((!(pte->present())), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "pte->present()", "ptab.cpp", 233); } while (0);
                continue;
            }

            *pte = *mst;


	    /*
            trace (TRACE_PTAB, "(S)   L:%u S:%u PTE:%#010lx MST:%#010lx %#010lx %#010llx %#x",
                   lev, shift,
                   Buddy::ptr_to_phys (pte),
                   Buddy::ptr_to_phys (mst),
                   virt & ~(size - 1),
                   static_cast<uint64>(pte->addr()),
                   size);
	    */

        }

        return size;
    }
}

void Ptab::sync_master_range (mword s_addr, mword e_addr)
{
    while (s_addr < e_addr) {
        size_t size = sync_master (s_addr);
        s_addr = (s_addr & ~(size - 1)) + size;
    }
}

void *Ptab::remap (Paddr phys)
{
    unsigned offset = static_cast<unsigned>(phys & ((1u << 22) - 1));

    phys &= ~offset;

    if (phys != remap_addr) {
        unmap (0xff000000, 0xff800000 - 0xff000000);
        map (phys, 0xff000000, 0xff800000 - 0xff000000, ATTR_WRITABLE);
        remap_addr = phys;
    }

    return reinterpret_cast<void *>(0xff000000 + offset);
}
