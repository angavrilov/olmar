# 1 "nonport.cpp"
 
 
 

# 1 "/usr/include/stdio.h" 1 3
 

















 







# 1 "/usr/include/features.h" 1 3
 




















 






























































 




















 





 



 







 
# 142 "/usr/include/features.h" 3


 









 








 



























# 208 "/usr/include/features.h" 3


































 



 


 








 




 
















 


# 1 "/usr/include/sys/cdefs.h" 1 3
 




















 




 






 



# 53 "/usr/include/sys/cdefs.h" 3














 





 




 









 







 










 






 









# 136 "/usr/include/sys/cdefs.h" 3


 






 








 








 








 










 







 




 






# 283 "/usr/include/features.h" 2 3



 








 




 

 








# 1 "/usr/include/gnu/stubs.h" 1 3
 



















# 311 "/usr/include/features.h" 2 3




# 27 "/usr/include/stdio.h" 2 3


extern "C" { 



# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


































typedef unsigned int size_t;






















 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 33 "/usr/include/stdio.h" 2 3


# 1 "/usr/include/bits/types.h" 1 3
 

















 









# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 29 "/usr/include/bits/types.h" 2 3


 
typedef unsigned char __u_char;
typedef unsigned short __u_short;
typedef unsigned int __u_int;
typedef unsigned long __u_long;




typedef struct
  {
    long int __val[2];
  } __quad_t;
typedef struct
  {
    __u_long __val[2];
  } __u_quad_t;

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;




typedef __quad_t *__qaddr_t;

typedef __u_quad_t __dev_t;		 
typedef __u_int __uid_t;		 
typedef __u_int __gid_t;		 
typedef __u_long __ino_t;		 
typedef __u_int __mode_t;		 
typedef __u_int __nlink_t; 		 
typedef long int __off_t;		 
typedef __quad_t __loff_t;		 
typedef int __pid_t;			 
typedef int __ssize_t;			 
typedef __u_long __rlim_t;		 
typedef __u_quad_t __rlim64_t;		 
typedef __u_int __id_t;			 

typedef struct
  {
    int __val[2];
  } __fsid_t;				 

 
typedef int __daddr_t;			 
typedef char *__caddr_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __swblk_t;		 

typedef long int __clock_t;

 
typedef int __clockid_t;

 
typedef int __timer_t;


 



typedef int __key_t;

 
typedef unsigned short int __ipc_pid_t;


 
typedef long int __blksize_t;

 

 
typedef long int __blkcnt_t;
typedef __quad_t __blkcnt64_t;

 
typedef __u_long __fsblkcnt_t;
typedef __u_quad_t __fsblkcnt64_t;

 
typedef __u_long __fsfilcnt_t;
typedef __u_quad_t __fsfilcnt64_t;

 
typedef __u_quad_t __ino64_t;

 
typedef __loff_t __off64_t;

 
typedef long int __t_scalar_t;
typedef unsigned long int __t_uscalar_t;

 
typedef int __intptr_t;

 
typedef unsigned int __socklen_t;


 

# 1 "/usr/include/bits/pthreadtypes.h" 1 3
 
 
 
 
 
 
 
 
 
 
 
 
 









# 1 "/usr/include/bits/sched.h" 1 3
 



















# 62 "/usr/include/bits/sched.h" 3





 
struct __sched_param
  {
    int __sched_priority;
  };


# 23 "/usr/include/bits/pthreadtypes.h" 2 3


 
struct _pthread_fastlock
{
  long int __status;    
  int __spinlock;       

};


 
typedef struct _pthread_descr_struct *_pthread_descr;




 
typedef struct __pthread_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
} pthread_attr_t;


 
typedef struct
{
  struct _pthread_fastlock __c_lock;  
  _pthread_descr __c_waiting;         
} pthread_cond_t;


 
typedef struct
{
  int __dummy;
} pthread_condattr_t;

 
typedef unsigned int pthread_key_t;


 
 

typedef struct
{
  int __m_reserved;                
  int __m_count;                   
  _pthread_descr __m_owner;        
  int __m_kind;                    
  struct _pthread_fastlock __m_lock;  
} pthread_mutex_t;


 
typedef struct
{
  int __mutexkind;
} pthread_mutexattr_t;


 
typedef int pthread_once_t;


# 117 "/usr/include/bits/pthreadtypes.h" 3


# 136 "/usr/include/bits/pthreadtypes.h" 3



 
typedef unsigned long int pthread_t;


# 143 "/usr/include/bits/types.h" 2 3




# 35 "/usr/include/stdio.h" 2 3








 
typedef struct _IO_FILE FILE;








 
typedef struct _IO_FILE __FILE;









# 1 "/usr/include/libio.h" 1 3
 




























# 1 "/usr/include/_G_config.h" 1 3
 





 






# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 





























 



















































typedef unsigned int  wint_t;




 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 14 "/usr/include/_G_config.h" 2 3










# 1 "/usr/include/wchar.h" 1 3
 

















 











# 46 "/usr/include/wchar.h" 3


# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 190 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3














 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 48 "/usr/include/wchar.h" 2 3


# 1 "/usr/include/bits/wchar.h" 1 3
 

























# 50 "/usr/include/wchar.h" 2 3


 













 
typedef struct
{
  int __count;
  union
  {
    wint_t __wch;
    char __wchb[4];
  } __value;		 
} __mbstate_t;




 

# 683 "/usr/include/wchar.h" 3



# 24 "/usr/include/_G_config.h" 2 3


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








# 1 "/usr/include/gconv.h" 1 3
 

















 








# 1 "/usr/include/wchar.h" 1 3
 

















 











# 46 "/usr/include/wchar.h" 3


# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 190 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3














 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 48 "/usr/include/wchar.h" 2 3




 











# 76 "/usr/include/wchar.h" 3




 

# 683 "/usr/include/wchar.h" 3



# 28 "/usr/include/gconv.h" 2 3


# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 30 "/usr/include/gconv.h" 2 3


 


 
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
			    const  unsigned char **, const  unsigned char *,
			    unsigned char **, size_t *, int, int);

 
typedef int (*__gconv_init_fct) (struct __gconv_step *);
typedef void (*__gconv_end_fct) (struct __gconv_step *);


 
typedef int (*__gconv_trans_fct) (struct __gconv_step *,
				  struct __gconv_step_data *, void *,
				  const  unsigned char *,
				  const  unsigned char **,
				  const  unsigned char *, unsigned char **,
				  size_t *);

 
typedef int (*__gconv_trans_context_fct) (void *, const  unsigned char *,
					  const  unsigned char *,
					  unsigned char *, unsigned char *);

 
typedef int (*__gconv_trans_query_fct) (const  char *, const  char ***,
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
  const  char *__modname;

  int __counter;

  char *__from_name;
  char *__to_name;

  __gconv_fct __fct;
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
    struct __gconv_step_data __data [1] ;
} *__gconv_t;


# 44 "/usr/include/_G_config.h" 2 3

typedef union
{
  struct __gconv_info __cd;
  struct
  {
    struct __gconv_info __cd;
    struct __gconv_step_data __data;
  } __combined;
} _G_iconv_t;

typedef int _G_int16_t  ;
typedef int _G_int32_t  ;
typedef unsigned int _G_uint16_t  ;
typedef unsigned int _G_uint32_t  ;




 


















 




 














# 30 "/usr/include/libio.h" 2 3

 

















 

# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stdarg.h" 1 3
 
































































 






typedef void *__gnuc_va_list;



 

# 122 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stdarg.h" 3




















# 209 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stdarg.h" 3




# 51 "/usr/include/libio.h" 2 3







# 70 "/usr/include/libio.h" 3


 

















# 101 "/usr/include/libio.h" 3











 

























 



















struct _IO_jump_t;  struct _IO_FILE;

 







typedef void _IO_lock_t;



 

struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;
   

   
  int _pos;
# 190 "/usr/include/libio.h" 3

};

 
enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};

# 257 "/usr/include/libio.h" 3


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
  int _blksize;
  __off_t   _old_offset;  


   
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

   

  _IO_lock_t *_lock;








  __off64_t   _offset;





  void *__pad1;
  void *__pad2;

  int _mode;
   
  char _unused2[15 * sizeof (int) - 2 * sizeof (void *)];

};





struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;











 

 

typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);

 





typedef __ssize_t __io_write_fn (void *__cookie, const  char *__buf,
				 size_t __n);

 





typedef int __io_seek_fn (void *__cookie, __off64_t   *__pos, int __w);

 
typedef int __io_close_fn (void *__cookie);


# 387 "/usr/include/libio.h" 3




extern "C" {


extern int __underflow (_IO_FILE *)  ;
extern int __uflow (_IO_FILE *)  ;
extern int __overflow (_IO_FILE *, int)  ;
extern wint_t   __wunderflow (_IO_FILE *)  ;
extern wint_t   __wuflow (_IO_FILE *)  ;
extern wint_t   __woverflow (_IO_FILE *, wint_t  )  ;
























extern int _IO_getc (_IO_FILE *__fp)  ;
extern int _IO_putc (int __c, _IO_FILE *__fp)  ;
extern int _IO_feof (_IO_FILE *__fp)  ;
extern int _IO_ferror (_IO_FILE *__fp)  ;

extern int _IO_peekc_locked (_IO_FILE *__fp)  ;

 



extern void _IO_flockfile (_IO_FILE *)  ;
extern void _IO_funlockfile (_IO_FILE *)  ;
extern int _IO_ftrylockfile (_IO_FILE *)  ;
















extern int _IO_vfscanf (_IO_FILE *  , const char *  ,
			__gnuc_va_list , int *  )  ;
extern int _IO_vfprintf (_IO_FILE *  , const char *  ,
			 __gnuc_va_list )  ;
extern __ssize_t   _IO_padn (_IO_FILE *, int, __ssize_t  )  ;
extern size_t   _IO_sgetn (_IO_FILE *, void *, size_t  )  ;

extern __off64_t   _IO_seekoff (_IO_FILE *, __off64_t  , int, int)  ;
extern __off64_t   _IO_seekpos (_IO_FILE *, __off64_t  , int)  ;

extern void _IO_free_backup_area (_IO_FILE *)  ;

# 507 "/usr/include/libio.h" 3



}



# 64 "/usr/include/stdio.h" 2 3


# 75 "/usr/include/stdio.h" 3


 

typedef _G_fpos_t fpos_t;







 





 





 






 







 




 








# 1 "/usr/include/bits/stdio_lim.h" 1 3
 








































# 128 "/usr/include/stdio.h" 2 3



 
extern FILE *stdin;		 
extern FILE *stdout;		 
extern FILE *stderr;		 
 




 
extern int remove (const  char *__filename)  ;
 
extern int rename (const  char *__old, const  char *__new)  ;


 

extern FILE *tmpfile (void)  ;










 
extern char *tmpnam (char *__s)  ;


 

extern char *tmpnam_r (char *__s)  ;




 






extern char *tempnam (const  char *__dir, const  char *__pfx)
        ;



 
extern int fclose (FILE *__stream)  ;
 
extern int fflush (FILE *__stream)  ;


 
extern int fflush_unlocked (FILE *__stream)  ;









 
extern FILE *fopen (const  char *   __filename,
		    const  char *   __modes)  ;
 
extern FILE *freopen (const  char *   __filename,
		      const  char *   __modes,
		      FILE *   __stream)  ;
# 219 "/usr/include/stdio.h" 3










 
extern FILE *fdopen (int __fd, const  char *__modes)  ;


# 248 "/usr/include/stdio.h" 3



 

extern void setbuf (FILE *   __stream, char *   __buf)  ;
 


extern int setvbuf (FILE *   __stream, char *   __buf,
		    int __modes, size_t __n)  ;


 

extern void setbuffer (FILE *   __stream, char *   __buf,
		       size_t __size)  ;

 
extern void setlinebuf (FILE *__stream)  ;



 
extern int fprintf (FILE *   __stream,
		    const  char *   __format, ...)  ;
 
extern int printf (const  char *   __format, ...)  ;
 
extern int sprintf (char *   __s,
		    const  char *   __format, ...)  ;

 
extern int vfprintf (FILE *   __s, const  char *   __format,
		     __gnuc_va_list  __arg)  ;
 
extern int vprintf (const  char *   __format, __gnuc_va_list  __arg)
      ;
 
extern int vsprintf (char *   __s, const  char *   __format,
		     __gnuc_va_list  __arg)  ;


 
extern int snprintf (char *   __s, size_t __maxlen,
		     const  char *   __format, ...)
        ;

extern int vsnprintf (char *   __s, size_t __maxlen,
		      const  char *   __format, __gnuc_va_list  __arg)
        ;


# 320 "/usr/include/stdio.h" 3



 
extern int fscanf (FILE *   __stream,
		   const  char *   __format, ...)  ;
 
extern int scanf (const  char *   __format, ...)  ;
 
extern int sscanf (const  char *   __s,
		   const  char *   __format, ...)  ;

# 346 "/usr/include/stdio.h" 3



 
extern int fgetc (FILE *__stream)  ;
extern int getc (FILE *__stream)  ;

 
extern int getchar (void)  ;

 




 
extern int getc_unlocked (FILE *__stream)  ;
extern int getchar_unlocked (void)  ;



 
extern int fgetc_unlocked (FILE *__stream)  ;



 
extern int fputc (int __c, FILE *__stream)  ;
extern int putc (int __c, FILE *__stream)  ;

 
extern int putchar (int __c)  ;

 




 
extern int fputc_unlocked (int __c, FILE *__stream)  ;



 
extern int putc_unlocked (int __c, FILE *__stream)  ;
extern int putchar_unlocked (int __c)  ;




 
extern int getw (FILE *__stream)  ;

 
extern int putw (int __w, FILE *__stream)  ;



 
extern char *fgets (char *   __s, int __n, FILE *   __stream)
      ;







 

extern char *gets (char *__s)  ;


# 436 "/usr/include/stdio.h" 3



 
extern int fputs (const  char *   __s, FILE *   __stream)
      ;







 
extern int puts (const  char *__s)  ;


 
extern int ungetc (int __c, FILE *__stream)  ;


 
extern size_t fread (void *   __ptr, size_t __size,
		     size_t __n, FILE *   __stream)  ;
 
extern size_t fwrite (const  void *   __ptr, size_t __size,
		      size_t __n, FILE *   __s)  ;


 
extern size_t fread_unlocked (void *   __ptr, size_t __size,
			      size_t __n, FILE *   __stream)  ;
extern size_t fwrite_unlocked (const  void *   __ptr, size_t __size,
			       size_t __n, FILE *   __stream)  ;



 
extern int fseek (FILE *__stream, long int __off, int __whence)  ;
 
extern long int ftell (FILE *__stream)  ;
 
extern void rewind (FILE *__stream)  ;

 












 
extern int fgetpos (FILE *   __stream, fpos_t *   __pos)
      ;
 
extern int fsetpos (FILE *__stream, const  fpos_t *__pos)  ;
# 519 "/usr/include/stdio.h" 3










 
extern void clearerr (FILE *__stream)  ;
 
extern int feof (FILE *__stream)  ;
 
extern int ferror (FILE *__stream)  ;


 
extern void clearerr_unlocked (FILE *__stream)  ;
extern int feof_unlocked (FILE *__stream)  ;
extern int ferror_unlocked (FILE *__stream)  ;



 
extern void perror (const  char *__s)  ;

 


extern int sys_nerr;
extern const  char * const  sys_errlist[];








 
extern int fileno (FILE *__stream)  ;



 
extern int fileno_unlocked (FILE *__stream)  ;





 
extern FILE *popen (const  char *__command, const  char *__modes)  ;

 
extern int pclose (FILE *__stream)  ;




 
extern char *ctermid (char *__s)  ;









# 601 "/usr/include/stdio.h" 3




 

 
extern void flockfile (FILE *__stream)  ;

 

extern int ftrylockfile (FILE *__stream)  ;

 
extern void funlockfile (FILE *__stream)  ;










 





} 




# 5 "nonport.cpp" 2

# 1 "/usr/include/stdlib.h" 1 3
 

















 







 





# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 


# 269 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 33 "/usr/include/stdlib.h" 2 3


extern "C" { 




# 91 "/usr/include/stdlib.h" 3


 
typedef struct
  {
    int quot;			 
    int rem;			 
  } div_t;

 

typedef struct
  {
    long int quot;		 
    long int rem;		 
  } ldiv_t;



# 118 "/usr/include/stdlib.h" 3



 



 





 

extern size_t __ctype_get_mb_cur_max (void)  ;


 
extern double atof (const  char *__nptr)    ;
 
extern int atoi (const  char *__nptr)    ;
 
extern long int atol (const  char *__nptr)    ;







 
extern double strtod (const  char *   __nptr,
		      char **   __endptr)  ;










 
extern long int strtol (const  char *   __nptr,
			char **   __endptr, int __base)  ;
 
extern unsigned long int strtoul (const  char *   __nptr,
				  char **   __endptr, int __base)
      ;

# 180 "/usr/include/stdlib.h" 3


# 194 "/usr/include/stdlib.h" 3



# 244 "/usr/include/stdlib.h" 3



 


extern double __strtod_internal (const  char *   __nptr,
				 char **   __endptr, int __group)
      ;
extern float __strtof_internal (const  char *   __nptr,
				char **   __endptr, int __group)
      ;
extern long double __strtold_internal (const  char *   __nptr,
				       char **   __endptr,
				       int __group)  ;

extern long int __strtol_internal (const  char *   __nptr,
				   char **   __endptr,
				   int __base, int __group)  ;



extern unsigned long int __strtoul_internal (const  char *   __nptr,
					     char **   __endptr,
					     int __base, int __group)  ;


# 288 "/usr/include/stdlib.h" 3


# 378 "/usr/include/stdlib.h" 3




 


extern char *l64a (long int __n)  ;

 
extern long int a64l (const  char *__s)    ;


# 1 "/usr/include/sys/types.h" 1 3
 

















 








extern "C" { 




typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;


typedef __loff_t loff_t;



typedef __ino_t ino_t;











typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;





typedef __off_t off_t;











typedef __pid_t pid_t;




typedef __id_t id_t;




typedef __ssize_t ssize_t;




typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;



typedef __key_t key_t;









# 1 "/usr/include/time.h" 1 3
 

















 














# 51 "/usr/include/time.h" 3


# 61 "/usr/include/time.h" 3








 
typedef __time_t time_t;










 
typedef __clockid_t clockid_t;










 
typedef __timer_t timer_t;





# 112 "/usr/include/time.h" 3




# 364 "/usr/include/time.h" 3



# 126 "/usr/include/sys/types.h" 2 3


# 137 "/usr/include/sys/types.h" 3



# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 140 "/usr/include/sys/types.h" 2 3



 
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;


 



 


typedef	char int8_t;
typedef	short int int16_t;
typedef	int int32_t;





 
typedef	unsigned char u_int8_t;
typedef	unsigned short int u_int16_t;
typedef	unsigned int u_int32_t;




typedef int register_t;

# 200 "/usr/include/sys/types.h" 3





 
# 1 "/usr/include/endian.h" 1 3
 






















 











 
# 1 "/usr/include/bits/endian.h" 1 3
 






# 37 "/usr/include/endian.h" 2 3


 



















# 206 "/usr/include/sys/types.h" 2 3


 
# 1 "/usr/include/sys/select.h" 1 3
 


















 






 


 
# 1 "/usr/include/bits/select.h" 1 3
 






















# 57 "/usr/include/bits/select.h" 3


 













# 31 "/usr/include/sys/select.h" 2 3


 
# 1 "/usr/include/bits/sigset.h" 1 3
 





















typedef int __sig_atomic_t;

 


typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int))) ];
  } __sigset_t;




 





# 125 "/usr/include/bits/sigset.h" 3

# 34 "/usr/include/sys/select.h" 2 3




typedef __sigset_t sigset_t;


 

# 1 "/usr/include/time.h" 1 3
 

















 














# 51 "/usr/include/time.h" 3


# 61 "/usr/include/time.h" 3



# 72 "/usr/include/time.h" 3



# 84 "/usr/include/time.h" 3



# 96 "/usr/include/time.h" 3








 

struct timespec
  {
    long int tv_sec;		 
    long int tv_nsec;		 
  };





# 364 "/usr/include/time.h" 3



# 43 "/usr/include/sys/select.h" 2 3


# 1 "/usr/include/bits/time.h" 1 3
 


















 



# 57 "/usr/include/bits/time.h" 3








 

struct timeval
  {
    __time_t tv_sec;		 
    __suseconds_t tv_usec;	 
  };


# 45 "/usr/include/sys/select.h" 2 3



 
typedef long int __fd_mask;

 




 
typedef struct
  {
     





    __fd_mask __fds_bits[1024  / (8 * sizeof (__fd_mask)) ];


  } fd_set;

 



 
typedef __fd_mask fd_mask;

 




 






extern "C" { 

 




extern int select (int __nfds, fd_set *   __readfds,
		   fd_set *   __writefds,
		   fd_set *   __exceptfds,
		   struct timeval *   __timeout)  ;

# 110 "/usr/include/sys/select.h" 3


} 


# 209 "/usr/include/sys/types.h" 2 3


 
# 1 "/usr/include/sys/sysmacros.h" 1 3
 





















 









 
















# 212 "/usr/include/sys/types.h" 2 3









 


typedef __blkcnt_t blkcnt_t;	  



typedef __fsblkcnt_t fsblkcnt_t;  



typedef __fsfilcnt_t fsfilcnt_t;  


# 248 "/usr/include/sys/types.h" 3








} 


# 391 "/usr/include/stdlib.h" 2 3


 



 
extern long int random (void)  ;

 
extern void srandom (unsigned int __seed)  ;

 



extern char *initstate (unsigned int __seed, char *__statebuf,
			size_t __statelen)  ;

 

extern char *setstate (char *__statebuf)  ;



 



struct random_data
  {
    int32_t *fptr;		 
    int32_t *rptr;		 
    int32_t *state;		 
    int rand_type;		 
    int rand_deg;		 
    int rand_sep;		 
    int32_t *end_ptr;		 
  };

extern int random_r (struct random_data *   __buf,
		     int32_t *   __result)  ;

extern int srandom_r (unsigned int __seed, struct random_data *__buf)  ;

extern int initstate_r (unsigned int __seed, char *   __statebuf,
			size_t __statelen,
			struct random_data *   __buf)  ;

extern int setstate_r (char *   __statebuf,
		       struct random_data *   __buf)  ;




 
extern int rand (void)  ;
 
extern void srand (unsigned int __seed)  ;


 
extern int rand_r (unsigned int *__seed)  ;




 

 
extern double drand48 (void)  ;
extern double erand48 (unsigned short int __xsubi[3])  ;

 
extern long int lrand48 (void)  ;
extern long int nrand48 (unsigned short int __xsubi[3])  ;

 
extern long int mrand48 (void)  ;
extern long int jrand48 (unsigned short int __xsubi[3])  ;

 
extern void srand48 (long int __seedval)  ;
extern unsigned short int *seed48 (unsigned short int __seed16v[3])  ;
extern void lcong48 (unsigned short int __param[7])  ;


 


struct drand48_data
  {
    unsigned short int __x[3];	 
    unsigned short int __old_x[3];  
    unsigned short int __c;	 
    unsigned short int __init;	 
    unsigned long long int __a;	 
  };

 
extern int drand48_r (struct drand48_data *   __buffer,
		      double *   __result)  ;
extern int erand48_r (unsigned short int __xsubi[3],
		      struct drand48_data *   __buffer,
		      double *   __result)  ;

 
extern int lrand48_r (struct drand48_data *   __buffer,
		      long int *   __result)  ;
extern int nrand48_r (unsigned short int __xsubi[3],
		      struct drand48_data *   __buffer,
		      long int *   __result)  ;

 
extern int mrand48_r (struct drand48_data *   __buffer,
		      long int *   __result)  ;
extern int jrand48_r (unsigned short int __xsubi[3],
		      struct drand48_data *   __buffer,
		      long int *   __result)  ;

 
extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
      ;

extern int seed48_r (unsigned short int __seed16v[3],
		     struct drand48_data *__buffer)  ;

extern int lcong48_r (unsigned short int __param[7],
		      struct drand48_data *__buffer)  ;







 
extern void *malloc (size_t __size)    ;
 
extern void *calloc (size_t __nmemb, size_t __size)
        ;



 

extern void *realloc (void *__ptr, size_t __size)    ;
 
extern void free (void *__ptr)  ;


 
extern void cfree (void *__ptr)  ;



# 1 "/usr/include/alloca.h" 1 3
 























# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 25 "/usr/include/alloca.h" 2 3


extern "C" { 

 


 
extern void *alloca (size_t __size)  ;





} 


# 547 "/usr/include/stdlib.h" 2 3




 
extern void *valloc (size_t __size)    ;








 
extern void abort (void)    ;


 
extern int atexit (void (*__func) (void))  ;


 

extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
      ;


 


extern void exit (int __status)    ;








 
extern char *getenv (const  char *__name)  ;

 

extern char *__secure_getenv (const  char *__name)  ;


 
 

extern int putenv (char *__string)  ;



 

extern int setenv (const  char *__name, const  char *__value, int __replace)
      ;

 
extern int unsetenv (const  char *__name)  ;



 


extern int clearenv (void)  ;




 



extern char *mktemp (char *__template)  ;

 





extern int mkstemp (char *__template)  ;













 




extern char *mkdtemp (char *__template)  ;



 
extern int system (const  char *__command)  ;










 





extern char *realpath (const  char *   __name,
		       char *   __resolved)  ;



 


typedef int (*__compar_fn_t) (const  void *, const  void *);






 

extern void *bsearch (const  void *__key, const  void *__base,
		      size_t __nmemb, size_t __size, __compar_fn_t __compar);

 

extern void qsort (void *__base, size_t __nmemb, size_t __size,
		   __compar_fn_t __compar);


 
extern int abs (int __x)    ;
extern long int labs (long int __x)    ;






 

 
extern div_t div (int __numer, int __denom)
        ;
extern ldiv_t ldiv (long int __numer, long int __denom)
        ;








 


 


extern char *ecvt (double __value, int __ndigit, int *   __decpt,
		   int *   __sign)  ;

 


extern char *fcvt (double __value, int __ndigit, int *   __decpt,
		   int *   __sign)  ;

 


extern char *gcvt (double __value, int __ndigit, char *__buf)  ;



 
extern char *qecvt (long double __value, int __ndigit,
		    int *   __decpt, int *   __sign)  ;
extern char *qfcvt (long double __value, int __ndigit,
		    int *   __decpt, int *   __sign)  ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)  ;


 

extern int ecvt_r (double __value, int __ndigit, int *   __decpt,
		   int *   __sign, char *   __buf,
		   size_t __len)  ;
extern int fcvt_r (double __value, int __ndigit, int *   __decpt,
		   int *   __sign, char *   __buf,
		   size_t __len)  ;

extern int qecvt_r (long double __value, int __ndigit,
		    int *   __decpt, int *   __sign,
		    char *   __buf, size_t __len)  ;
extern int qfcvt_r (long double __value, int __ndigit,
		    int *   __decpt, int *   __sign,
		    char *   __buf, size_t __len)  ;




 

extern int mblen (const  char *__s, size_t __n)  ;
 

extern int mbtowc (wchar_t *   __pwc,
		   const  char *   __s, size_t __n)  ;
 

extern int wctomb (char *__s, wchar_t __wchar)  ;


 
extern size_t mbstowcs (wchar_t *    __pwcs,
			const  char *   __s, size_t __n)  ;
 
extern size_t wcstombs (char *   __s,
			const  wchar_t *   __pwcs, size_t __n)
      ;



 



extern int rpmatch (const  char *__response)  ;



# 812 "/usr/include/stdlib.h" 3









 






# 843 "/usr/include/stdlib.h" 3


# 853 "/usr/include/stdlib.h" 3



 


extern int getloadavg (double __loadavg[], int __nelem)  ;





} 


# 6 "nonport.cpp" 2

# 1 "/usr/include/string.h" 1 3
 

















 








extern "C" { 

 


# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 33 "/usr/include/string.h" 2 3



 
extern void *memcpy (void *   __dest,
		     const  void *   __src, size_t __n)  ;
 

extern void *memmove (void *__dest, const  void *__src, size_t __n)
      ;

 



extern void *memccpy (void *   __dest, const  void *   __src,
		      int __c, size_t __n)
      ;



 
extern void *memset (void *__s, int __c, size_t __n)  ;

 
extern int memcmp (const  void *__s1, const  void *__s2, size_t __n)
        ;

 
extern void *memchr (const  void *__s, int __c, size_t __n)
         ;

# 73 "/usr/include/string.h" 3



 
extern char *strcpy (char *   __dest, const  char *   __src)
      ;
 
extern char *strncpy (char *   __dest,
		      const  char *   __src, size_t __n)  ;

 
extern char *strcat (char *   __dest, const  char *   __src)
      ;
 
extern char *strncat (char *   __dest, const  char *   __src,
		      size_t __n)  ;

 
extern int strcmp (const  char *__s1, const  char *__s2)
        ;
 
extern int strncmp (const  char *__s1, const  char *__s2, size_t __n)
        ;

 
extern int strcoll (const  char *__s1, const  char *__s2)
        ;
 
extern size_t strxfrm (char *   __dest,
		       const  char *   __src, size_t __n)  ;

# 116 "/usr/include/string.h" 3



 
extern char *strdup (const  char *__s)    ;


 







# 152 "/usr/include/string.h" 3


 
extern char *strchr (const  char *__s, int __c)    ;
 
extern char *strrchr (const  char *__s, int __c)    ;







 

extern size_t strcspn (const  char *__s, const  char *__reject)
        ;
 

extern size_t strspn (const  char *__s, const  char *__accept)
        ;
 
extern char *strpbrk (const  char *__s, const  char *__accept)
        ;
 
extern char *strstr (const  char *__haystack, const  char *__needle)
        ;







 
extern char *strtok (char *   __s, const  char *   __delim)
      ;

 

extern char *__strtok_r (char *   __s,
			 const  char *   __delim,
			 char **   __save_ptr)  ;

extern char *strtok_r (char *   __s, const  char *   __delim,
		       char **   __save_ptr)  ;


# 214 "/usr/include/string.h" 3



 
extern size_t strlen (const  char *__s)    ;









 
extern char *strerror (int __errnum)  ;

 

extern char *strerror_r (int __errnum, char *__buf, size_t __buflen)  ;


 

extern void __bzero (void *__s, size_t __n)  ;


 
extern void bcopy (const  void *__src, void *__dest, size_t __n)  ;

 
extern void bzero (void *__s, size_t __n)  ;

 
extern int bcmp (const  void *__s1, const  void *__s2, size_t __n)
        ;

 
extern char *index (const  char *__s, int __c)    ;

 
extern char *rindex (const  char *__s, int __c)    ;

 

extern int ffs (int __i)    ;

 









 
extern int strcasecmp (const  char *__s1, const  char *__s2)
        ;

 
extern int strncasecmp (const  char *__s1, const  char *__s2, size_t __n)
        ;


# 289 "/usr/include/string.h" 3



 

extern char *strsep (char **   __stringp,
		     const  char *   __delim)  ;


# 332 "/usr/include/string.h" 3



# 361 "/usr/include/string.h" 3


} 


# 7 "nonport.cpp" 2


# 38 "nonport.cpp"

# 1 "/usr/include/sys/time.h" 1 3
 
























# 1 "/usr/include/time.h" 1 3
 

















 














# 51 "/usr/include/time.h" 3


# 61 "/usr/include/time.h" 3



# 72 "/usr/include/time.h" 3



# 84 "/usr/include/time.h" 3



# 96 "/usr/include/time.h" 3




# 112 "/usr/include/time.h" 3




# 364 "/usr/include/time.h" 3



# 26 "/usr/include/sys/time.h" 2 3


# 1 "/usr/include/bits/time.h" 1 3
 


















 



# 57 "/usr/include/bits/time.h" 3




# 72 "/usr/include/bits/time.h" 3


# 28 "/usr/include/sys/time.h" 2 3





typedef __suseconds_t suseconds_t;




extern "C" { 

# 50 "/usr/include/sys/time.h" 3




 

struct timezone
  {
    int tz_minuteswest;		 
    int tz_dsttime;		 
  };

typedef struct timezone *   __timezone_ptr_t;




 




extern int gettimeofday (struct timeval *   __tv,
			 __timezone_ptr_t __tz)  ;


 

extern int settimeofday (const  struct timeval *__tv,
			 const  struct timezone *__tz)  ;

 



extern int adjtime (const  struct timeval *__delta,
		    struct timeval *__olddelta)  ;



 
enum __itimer_which
  {
     
    ITIMER_REAL = 0,

     
    ITIMER_VIRTUAL = 1,

     

    ITIMER_PROF = 2

  };

 

struct itimerval
  {
     
    struct timeval it_interval;
     
    struct timeval it_value;
  };




typedef int __itimer_which_t;


 

extern int getitimer (__itimer_which_t __which,
		      struct itimerval *__value)  ;

 


extern int setitimer (__itimer_which_t __which,
		      const  struct itimerval *   __new,
		      struct itimerval *   __old)  ;

 

extern int utimes (const  char *__file, const  struct timeval __tvp[2])
      ;



 








# 158 "/usr/include/sys/time.h" 3

# 167 "/usr/include/sys/time.h" 3


} 


# 39 "nonport.cpp" 2


# 1 "/usr/include/fcntl.h" 1 3
 

















 








 
extern "C" { 

 

# 1 "/usr/include/bits/fcntl.h" 1 3
 

























 






















 











 


































 


 




 




 














# 134 "/usr/include/bits/fcntl.h" 3


struct flock
  {
    short int l_type;	 
    short int l_whence;	 

    __off_t l_start;	 
    __off_t l_len;	 




    __pid_t l_pid;	 
  };

# 159 "/usr/include/bits/fcntl.h" 3


 









 








# 33 "/usr/include/fcntl.h" 2 3


 






 








 






 

extern int fcntl (int __fd, int __cmd, ...)  ;

 



extern int open (const  char *__file, int __oflag, ...)  ;












 



extern int creat (const  char *__file, __mode_t __mode)  ;














 



 









extern int lockf (int __fd, int __cmd, __off_t __len)  ;













# 163 "/usr/include/fcntl.h" 3


} 


# 41 "nonport.cpp" 2

# 1 "/usr/include/unistd.h" 1 3
 

















 








extern "C" { 

 


 



 


 


 


 



 



 



 



 






 


 




 


 


 



 



 



















































































# 1 "/usr/include/bits/posix_opt.h" 1 3
 





















 


 


 


 


 


 


 


 


 


 


 


 


 



 


 


 


 


 


 



 


 


 


 


 


 


 


 


 




 


 


 


 


 


 


 


 


 


 


 


 



# 175 "/usr/include/unistd.h" 2 3


 




 





 










# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 199 "/usr/include/unistd.h" 2 3


# 236 "/usr/include/unistd.h" 3




typedef __intptr_t intptr_t;






typedef __socklen_t socklen_t;




 






 
extern int access (const  char *__name, int __type)  ;








 







 






 





extern __off_t lseek (int __fd, __off_t __offset, int __whence)  ;
# 300 "/usr/include/unistd.h" 3





 
extern int close (int __fd)  ;

 

extern ssize_t read (int __fd, void *__buf, size_t __nbytes)  ;

 
extern ssize_t write (int __fd, const  void *__buf, size_t __n)  ;

# 347 "/usr/include/unistd.h" 3


 



extern int pipe (int __pipedes[2])  ;

 






extern unsigned int alarm (unsigned int __seconds)  ;

 






extern unsigned int sleep (unsigned int __seconds)  ;


 



extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
      ;

 

extern int usleep (__useconds_t __useconds)  ;



 

extern int pause (void)  ;


 
extern int chown (const  char *__file, __uid_t __owner, __gid_t __group)
      ;


 
extern int fchown (int __fd, __uid_t __owner, __gid_t __group)  ;


 

extern int lchown (const  char *__file, __uid_t __owner, __gid_t __group)
      ;



 
extern int chdir (const  char *__path)  ;


 
extern int fchdir (int __fd)  ;


 






extern char *getcwd (char *__buf, size_t __size)  ;









 


extern char *getwd (char *__buf)  ;



 
extern int dup (int __fd)  ;

 
extern int dup2 (int __fd, int __fd2)  ;

 
extern char **__environ;





 

extern int execve (const  char *__path, char * const  __argv[],
		   char * const  __envp[])  ;









 
extern int execv (const  char *__path, char * const  __argv[])  ;

 

extern int execle (const  char *__path, const  char *__arg, ...)  ;

 

extern int execl (const  char *__path, const  char *__arg, ...)  ;

 

extern int execvp (const  char *__file, char * const  __argv[])  ;

 


extern int execlp (const  char *__file, const  char *__arg, ...)  ;



 
extern int nice (int __inc)  ;



 
extern void _exit (int __status)  ;


 


# 1 "/usr/include/bits/confname.h" 1 3
 






















 
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX

  };

 
enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,


     

    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV ,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,


     
    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG

  };




 
enum
  {
    _CS_PATH,			 


# 492 "/usr/include/bits/confname.h" 3


# 527 "/usr/include/bits/confname.h" 3

# 561 "/usr/include/bits/confname.h" 3


    _CS_V6_WIDTH_RESTRICTED_ENVS,

  };

# 500 "/usr/include/unistd.h" 2 3


 
extern long int pathconf (const  char *__path, int __name)  ;

 
extern long int fpathconf (int __fd, int __name)  ;

 
extern long int sysconf (int __name)    ;


 
extern size_t confstr (int __name, char *__buf, size_t __len)  ;



 
extern __pid_t getpid (void)  ;

 
extern __pid_t getppid (void)  ;

 


extern __pid_t getpgrp (void)  ;








 
extern __pid_t __getpgid (__pid_t __pid)  ;





 


extern int setpgid (__pid_t __pid, __pid_t __pgid)  ;


 











 

extern int setpgrp (void)  ;

# 574 "/usr/include/unistd.h" 3



 


extern __pid_t setsid (void)  ;






 
extern __uid_t getuid (void)  ;

 
extern __uid_t geteuid (void)  ;

 
extern __gid_t getgid (void)  ;

 
extern __gid_t getegid (void)  ;

 


extern int getgroups (int __size, __gid_t __list[])  ;






 



extern int setuid (__uid_t __uid)  ;


 

extern int setreuid (__uid_t __ruid, __uid_t __euid)  ;



 
extern int seteuid (__uid_t __uid)  ;


 



extern int setgid (__gid_t __gid)  ;


 

extern int setregid (__gid_t __rgid, __gid_t __egid)  ;



 
extern int setegid (__gid_t __gid)  ;



 


extern __pid_t fork (void)  ;


 



extern __pid_t vfork (void)  ;



 

extern char *ttyname (int __fd)  ;

 

extern int ttyname_r (int __fd, char *__buf, size_t __buflen)  ;

 

extern int isatty (int __fd)  ;



 

extern int ttyslot (void)  ;



 
extern int link (const  char *__from, const  char *__to)  ;


 
extern int symlink (const  char *__from, const  char *__to)  ;

 


extern int readlink (const  char *   __path, char *   __buf,
		     size_t __len)  ;


 
extern int unlink (const  char *__name)  ;

 
extern int rmdir (const  char *__path)  ;


 
extern __pid_t tcgetpgrp (int __fd)  ;

 
extern int tcsetpgrp (int __fd, __pid_t __pgrp_id)  ;


 
extern char *getlogin (void)  ;








 
extern int setlogin (const  char *__name)  ;




 



# 1 "/usr/include/getopt.h" 1 3
 
























 











extern "C" {


 





extern char *optarg;

 











extern int optind;

 


extern int opterr;

 

extern int optopt;

# 113 "/usr/include/getopt.h" 3



 

























 


extern int getopt (int __argc, char *const *__argv, const char *__shortopts);




# 162 "/usr/include/getopt.h" 3

# 171 "/usr/include/getopt.h" 3



}


 



# 726 "/usr/include/unistd.h" 2 3





 


extern int gethostname (char *__name, size_t __len)  ;




 

extern int sethostname (const  char *__name, size_t __len)  ;

 

extern int sethostid (long int __id)  ;


 


extern int getdomainname (char *__name, size_t __len)  ;
extern int setdomainname (const  char *__name, size_t __len)  ;


 


extern int vhangup (void)  ;

 
extern int revoke (const  char *__file)  ;


 




extern int profil (unsigned short int *__sample_buffer, size_t __size,
		   size_t __offset, unsigned int __scale)  ;


 


extern int acct (const  char *__name)  ;


 
extern char *getusershell (void)  ;
extern void endusershell (void)  ;  
extern void setusershell (void)  ;  


 


extern int daemon (int __nochdir, int __noclose)  ;




 

extern int chroot (const  char *__path)  ;

 

extern char *getpass (const  char *__prompt)  ;




 
extern int fsync (int __fd)  ;





 
extern long int gethostid (void)  ;

 
extern void sync (void)  ;


 

extern int getpagesize (void)     ;


 

extern int truncate (const  char *__file, __off_t __length)  ;
# 834 "/usr/include/unistd.h" 3





 

extern int ftruncate (int __fd, __off_t __length)  ;













 

extern int getdtablesize (void)  ;






 

extern int brk (void *__addr)  ;

 



extern void *sbrk (intptr_t __delta)  ;




 









extern long int syscall (long int __sysno, ...)  ;




# 919 "/usr/include/unistd.h" 3



# 933 "/usr/include/unistd.h" 3



 

extern int fdatasync (int __fildes)  ;



 

# 959 "/usr/include/unistd.h" 3



 








 

 









extern int pthread_atfork (void (*__prepare) (void),
			   void (*__parent) (void),
			   void (*__child) (void))  ;


} 


# 42 "nonport.cpp" 2

# 1 "/usr/include/errno.h" 1 3
 

















 





 






extern "C" { 

 

# 1 "/usr/include/bits/errno.h" 1 3
 























# 1 "/usr/include/linux/errno.h" 1 3



# 1 "/usr/include/asm/errno.h" 1 3




































































































































# 4 "/usr/include/linux/errno.h" 2 3


# 24 "/usr/include/linux/errno.h" 3



# 25 "/usr/include/bits/errno.h" 2 3


 


 




 
extern int errno;

 
extern int *__errno_location (void)    ;







 













# 36 "/usr/include/errno.h" 2 3





 

















} 



 










# 43 "nonport.cpp" 2

# 1 "/usr/include/pwd.h" 1 3
 

















 








extern "C" { 




# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 33 "/usr/include/pwd.h" 2 3


# 47 "/usr/include/pwd.h" 3


 
struct passwd
{
  char *pw_name;		 
  char *pw_passwd;		 
  __uid_t pw_uid;		 
  __gid_t pw_gid;		 
  char *pw_gecos;		 
  char *pw_dir;			 
  char *pw_shell;		 
};









 
extern void setpwent (void)  ;

 
extern void endpwent (void)  ;

 
extern struct passwd *getpwent (void)  ;



 
extern struct passwd *fgetpwent (FILE *__stream)  ;

 
extern int putpwent (const  struct passwd *   __p,
		     FILE *   __f)  ;


 
extern struct passwd *getpwuid (__uid_t __uid)  ;

 
extern struct passwd *getpwnam (const  char *__name)  ;




 




 








extern int getpwent_r (struct passwd *   __resultbuf,
		       char *   __buffer, size_t __buflen,
		       struct passwd **   __result)  ;


extern int getpwuid_r (__uid_t __uid,
		       struct passwd *   __resultbuf,
		       char *   __buffer, size_t __buflen,
		       struct passwd **   __result)  ;

extern int getpwnam_r (const  char *   __name,
		       struct passwd *   __resultbuf,
		       char *   __buffer, size_t __buflen,
		       struct passwd **   __result)  ;



 

extern int fgetpwent_r (FILE *   __stream,
			struct passwd *   __resultbuf,
			char *   __buffer, size_t __buflen,
			struct passwd **   __result)  ;











} 


# 44 "nonport.cpp" 2






# 1 "/usr/include/sys/stat.h" 1 3
 

















 










# 78 "/usr/include/sys/stat.h" 3


# 94 "/usr/include/sys/stat.h" 3


extern "C" { 

# 1 "/usr/include/bits/stat.h" 1 3
 





















 






 





struct stat
  {
    __dev_t st_dev;			 
    unsigned short int __pad1;

    __ino_t st_ino;			 



    __mode_t st_mode;			 
    __nlink_t st_nlink;			 
    __uid_t st_uid;			 
    __gid_t st_gid;			 
    __dev_t st_rdev;			 
    unsigned short int __pad2;

    __off_t st_size;			 



    __blksize_t st_blksize;		 


    __blkcnt_t st_blocks;		 



    __time_t st_atime;			 
    unsigned long int __unused1;
    __time_t st_mtime;			 
    unsigned long int __unused2;
    __time_t st_ctime;			 
    unsigned long int __unused3;

    unsigned long int __unused4;
    unsigned long int __unused5;



  };

# 102 "/usr/include/bits/stat.h" 3


 



 



 








 





 







# 98 "/usr/include/sys/stat.h" 2 3




















 























 










 





 






 











 





 




 









 
extern int stat (const  char *   __file,
		 struct stat *   __buf)  ;

 

extern int fstat (int __fd, struct stat *__buf)  ;
# 217 "/usr/include/sys/stat.h" 3









 

extern int lstat (const  char *   __file,
		  struct stat *   __buf)  ;
# 239 "/usr/include/sys/stat.h" 3







 


extern int chmod (const  char *__file, __mode_t __mode)  ;

 

extern int fchmod (int __fd, __mode_t __mode)  ;



 

extern __mode_t umask (__mode_t __mask)  ;







 
extern int mkdir (const  char *__path, __mode_t __mode)  ;

 



extern int mknod (const  char *__path, __mode_t __mode, __dev_t __dev)
      ;



 
extern int mkfifo (const  char *__path, __mode_t __mode)  ;

 





















 

extern int __fxstat (int __ver, int __fildes, struct stat *__stat_buf)  ;
extern int __xstat (int __ver, const  char *__filename,
		    struct stat *__stat_buf)  ;
extern int __lxstat (int __ver, const  char *__filename,
		     struct stat *__stat_buf)  ;
# 327 "/usr/include/sys/stat.h" 3










extern int __xmknod (int __ver, const  char *__path, __mode_t __mode,
		     __dev_t *__dev)  ;

# 393 "/usr/include/sys/stat.h" 3


} 



# 50 "nonport.cpp" 2

# 1 "/usr/include/time.h" 1 3
 

















 










extern "C" { 




 


# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 38 "/usr/include/time.h" 2 3


 

# 1 "/usr/include/bits/time.h" 1 3
 


















 







 


 





 


extern long int __sysconf (int);




 

 

 


 






# 73 "/usr/include/bits/time.h" 3

# 42 "/usr/include/time.h" 2 3


 













 
typedef __clock_t clock_t;




# 72 "/usr/include/time.h" 3



# 84 "/usr/include/time.h" 3



# 96 "/usr/include/time.h" 3




# 112 "/usr/include/time.h" 3





 
struct tm
{
  int tm_sec;			 
  int tm_min;			 
  int tm_hour;			 
  int tm_mday;			 
  int tm_mon;			 
  int tm_year;			 
  int tm_wday;			 
  int tm_yday;			 
  int tm_isdst;			 


  long int tm_gmtoff;		 
  const  char *tm_zone;	 




};



 
struct itimerspec
  {
    struct timespec it_interval;
    struct timespec it_value;
  };

 
struct sigevent;











 

extern clock_t clock (void)  ;

 
extern time_t time (time_t *__timer)  ;

 
extern double difftime (time_t __time1, time_t __time0)
        ;

 
extern time_t mktime (struct tm *__tp)  ;


 


extern size_t strftime (char *   __s, size_t __maxsize,
			const  char *   __format,
			const  struct tm *   __tp)  ;










 

extern struct tm *gmtime (const  time_t *__timer)  ;

 

extern struct tm *localtime (const  time_t *__timer)  ;


 

extern struct tm *gmtime_r (const  time_t *   __timer,
			    struct tm *   __tp)  ;

 

extern struct tm *localtime_r (const  time_t *   __timer,
			       struct tm *   __tp)  ;


 

extern char *asctime (const  struct tm *__tp)  ;

 
extern char *ctime (const  time_t *__timer)  ;


 

 

extern char *asctime_r (const  struct tm *   __tp,
			char *   __buf)  ;

 
extern char *ctime_r (const  time_t *   __timer,
		      char *   __buf)  ;



 
extern char *__tzname[2];	 
extern int __daylight;		 
extern long int __timezone;	 



 
extern char *tzname[2];

 

extern void tzset (void)  ;



extern int daylight;
extern long int timezone;



 

extern int stime (const  time_t *__when)  ;



 






 


 
extern time_t timegm (struct tm *__tp)  ;

 
extern time_t timelocal (struct tm *__tp)  ;

 
extern int dysize (int __year)     ;




 
extern int nanosleep (const  struct timespec *__requested_time,
		      struct timespec *__remaining)  ;


 
extern int clock_getres (clockid_t __clock_id, struct timespec *__res)  ;

 
extern int clock_gettime (clockid_t __clock_id, struct timespec *__tp)  ;

 
extern int clock_settime (clockid_t __clock_id, const  struct timespec *__tp)
      ;

# 305 "/usr/include/time.h" 3



 
extern int timer_create (clockid_t __clock_id,
			 struct sigevent *   __evp,
			 timer_t *   __timerid)  ;

 
extern int timer_delete (timer_t __timerid)  ;

 
extern int timer_settime (timer_t __timerid, int __flags,
			  const  struct itimerspec *   __value,
			  struct itimerspec *   __ovalue)  ;

 
extern int timer_gettime (timer_t __timerid, struct itimerspec *__value)
      ;

 
extern int timer_getoverrun (timer_t __timerid)  ;



# 349 "/usr/include/time.h" 3


# 359 "/usr/include/time.h" 3



} 




# 51 "nonport.cpp" 2

# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/../../../../include/g++-3/iostream.h" 1 3
 





























# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/../../../../include/g++-3/streambuf.h" 1 3
 





























   



extern "C" {

}
 

# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stdarg.h" 1 3
 
































































 










 



 

















void va_end (__gnuc_va_list);		 


 



 












 























 
 













# 175 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stdarg.h" 3


 




 

 

 

typedef __gnuc_va_list va_list;
























# 40 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/../../../../include/g++-3/streambuf.h" 2 3

























extern "C++" {
class istream;  
class ostream; class streambuf;

 




typedef __off64_t   streamoff;
typedef __off64_t   streampos;




typedef __ssize_t   streamsize;

typedef unsigned long __fmtflags;
typedef unsigned char __iostate;

struct _ios_fields
{  
    streambuf *_strbuf;
    ostream* _tie;
    int _width;
    __fmtflags _flags;
    wchar_t   _fill;
    __iostate _state;
    __iostate _exceptions;
    int _precision;

    void *_arrays;  
};















# 124 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/../../../../include/g++-3/streambuf.h" 3


class ios : public _ios_fields {
  ios& operator=(ios&);   
  ios (const ios&);  
  public:
    typedef __fmtflags fmtflags;
    typedef int iostate;
    typedef int openmode;
    typedef __ssize_t   streamsize;
    enum io_state {
	goodbit = 0 ,
	eofbit = 1 ,
	failbit = 2 ,
	badbit = 4  };
    enum open_mode {
	in = 1 ,
	out = 2 ,
	ate = 4 ,
	app = 8 ,
	trunc = 16 ,
	nocreate = 32 ,
	noreplace = 64 ,
	bin = 128 ,  
	binary = 128  };
    enum seek_dir { beg, cur, end};
    typedef enum seek_dir seekdir;
     
    enum { skipws= 01 ,
	   left= 02 , right= 04 , internal= 010 ,
	   dec= 020 , oct= 040 , hex= 0100 ,
	   showbase= 0200 , showpoint= 0400 ,
	   uppercase= 01000 , showpos= 02000 ,
	   scientific= 04000 , fixed= 010000 ,
	   unitbuf= 020000 , stdio= 040000 



	   };
    enum {  
	basefield=dec+oct+hex,
	floatfield = scientific+fixed,
	adjustfield = left+right+internal
    };

# 177 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/../../../../include/g++-3/streambuf.h" 3


    ostream* tie() const { return _tie; }
    ostream* tie(ostream* val) { ostream* save=_tie; _tie=val; return save; }

     
    wchar_t   fill() const { return _fill; }
    wchar_t   fill(wchar_t   newf)
	{wchar_t   oldf = _fill; _fill = newf; return oldf;}
    fmtflags flags() const { return _flags; }
    fmtflags flags(fmtflags new_val) {
	fmtflags old_val = _flags; _flags = new_val; return old_val; }
    int precision() const { return _precision; }
    int precision(int newp) {
	unsigned short oldp = _precision; _precision = (unsigned short)newp;
	return oldp; }
    fmtflags setf(fmtflags val) {
	fmtflags oldbits = _flags;
	_flags |= val; return oldbits; }
    fmtflags setf(fmtflags val, fmtflags mask) {
	fmtflags oldbits = _flags;
	_flags = (_flags & ~mask) | (val & mask); return oldbits; }
    fmtflags unsetf(fmtflags mask) {
	fmtflags oldbits = _flags;
	_flags &= ~mask; return oldbits; }
    int width() const { return _width; }
    int width(int val) { int save = _width; _width = val; return save; }




    void _throw_failure() const { }

    void clear(iostate state = 0) {
	_state = _strbuf ? state : state|badbit;
	if (_state & _exceptions) _throw_failure(); }
    void set(iostate flag) { _state |= flag;
	if (_state & _exceptions) _throw_failure(); }
    void setstate(iostate flag) { _state |= flag;  
	if (_state & _exceptions) _throw_failure(); }
    int good() const { return _state == 0; }
    int eof() const { return _state & ios::eofbit; }
    int fail() const { return _state & (ios::badbit|ios::failbit); }
    int bad() const { return _state & ios::badbit; }
    iostate rdstate() const { return _state; }
    operator void*() const { return fail() ? (void*)0 : (void*)(-1); }
    int operator!() const { return fail(); }
    iostate exceptions() const { return _exceptions; }
    void exceptions(iostate enable) {
	_exceptions = enable;
	if (_state & _exceptions) _throw_failure(); }

    streambuf* rdbuf() const { return _strbuf; }
    streambuf* rdbuf(streambuf *_s) {
      streambuf *_old = _strbuf; _strbuf = _s; clear (); return _old; }

    static int sync_with_stdio(int on);
    static void sync_with_stdio() { sync_with_stdio(1); }
    static fmtflags bitalloc();
    static int xalloc();
    void*& pword(int);
    void* pword(int) const;
    long& iword(int);
    long iword(int) const;









     
    class Init {
    public:
      Init () { }
    };

  protected:
    inline ios(streambuf* sb = 0, ostream* tie_to = 0);
    inline virtual ~ios();
    inline void init(streambuf* sb, ostream* tie = 0);
};




typedef ios::seek_dir _seek_dir;


 
 
 
 
 

 
 
class streammarker : private _IO_marker {
    friend class streambuf;
    void set_offset(int offset) { _pos = offset; }
  public:
    streammarker(streambuf *sb);
    ~streammarker();
    int saving() { return  1; }
    int delta(streammarker&);
    int delta();
};

struct streambuf : public _IO_FILE {  
    friend class ios;
    friend class istream;
    friend class ostream;
    friend class streammarker;
    const void *&_vtable() { return *(const void**)((_IO_FILE*)this + 1); }
  protected:
    static streambuf* _list_all;  
    _IO_FILE*& xchain() { return _chain; }
    void _un_link();
    void _link_in();
    char* gptr() const
      { return _flags  & 0x100  ? _IO_save_base : _IO_read_ptr; }
    char* pptr() const { return _IO_write_ptr; }
    char* egptr() const
      { return _flags  & 0x100  ? _IO_save_end : _IO_read_end; }
    char* epptr() const { return _IO_write_end; }
    char* pbase() const { return _IO_write_base; }
    char* eback() const
      { return _flags  & 0x100  ? _IO_save_base : _IO_read_base;}
    char* base() const { return _IO_buf_base; }
    char* ebuf() const { return _IO_buf_end; }
    int blen() const { return _IO_buf_end - _IO_buf_base; }
    void xput_char(char c) { *_IO_write_ptr++ = c; }
    int xflags() { return _flags ; }
    int xflags(int f) {int fl = _flags ; _flags  = f; return fl;}
    void xsetflags(int f) { _flags  |= f; }
    void xsetflags(int f, int mask)
      { _flags  = (_flags  & ~mask) | (f & mask); }
    void gbump(int n)
      { _flags  & 0x100  ? (_IO_save_base+=n):(_IO_read_ptr+=n);}
    void pbump(int n) { _IO_write_ptr += n; }
    void setb(char* b, char* eb, int a=0);
    void setp(char* p, char* ep)
      { _IO_write_base=_IO_write_ptr=p; _IO_write_end=ep; }
    void setg(char* eb, char* g, char *eg) {
      if (_flags  & 0x100 ) _IO_free_backup_area(this); 
      _IO_read_base = eb; _IO_read_ptr = g; _IO_read_end = eg; }
    char *shortbuf() { return _shortbuf; }

    int in_backup() { return _flags & 0x100 ; }
     
    char *Gbase() { return in_backup() ? _IO_save_base : _IO_read_base; }
     
    char *eGptr() { return in_backup() ? _IO_save_end : _IO_read_end; }
     
    char *Bbase() { return in_backup() ? _IO_read_base : _IO_save_base; }
    char *Bptr() { return _IO_backup_base; }
     
    char *eBptr() { return in_backup() ? _IO_read_end : _IO_save_end; }
    char *Nbase() { return _IO_save_base; }
    char *eNptr() { return _IO_save_end; }
    int have_backup() { return _IO_save_base != ((void *)0) ; }
    int have_markers() { return _markers != ((void *)0) ; }
    void free_backup_area();
    void unsave_markers();  
    int put_mode() { return _flags & 0x800 ; }
    int switch_to_get_mode();
    
    streambuf(int flags=0);
  public:
    static int flush_all();
    static void flush_all_linebuffered();  
    virtual ~streambuf();
    virtual int overflow(int c = (-1) );  
    virtual int underflow();  
    virtual int uflow();  
    virtual int pbackfail(int c);
 
    virtual streamsize xsputn(const char* s, streamsize n);
    virtual streamsize xsgetn(char* s, streamsize n);
    virtual streampos seekoff(streamoff, _seek_dir, int mode=ios::in|ios::out);
    virtual streampos seekpos(streampos pos, int mode = ios::in|ios::out);

    streampos pubseekoff(streamoff o, _seek_dir d, int mode=ios::in|ios::out)
      { return _IO_seekoff (this, o, d, mode); }
    streampos pubseekpos(streampos pos, int mode = ios::in|ios::out)
      { return _IO_seekpos (this, pos, mode); }
    streampos sseekoff(streamoff, _seek_dir, int mode=ios::in|ios::out);
    streampos sseekpos(streampos pos, int mode = ios::in|ios::out);
    virtual streambuf* setbuf(char* p, int len);
    virtual int sync();
    virtual int doallocate();

    int seekmark(streammarker& mark, int delta = 0);
    int sputbackc(char c);
    int sungetc();
    int unbuffered() { return _flags & 2  ? 1 : 0; }
    int linebuffered() { return _flags & 0x200  ? 1 : 0; }
    void unbuffered(int i)
	{ if (i) _flags |= 2 ; else _flags &= ~2 ; }
    void linebuffered(int i)
	{ if (i) _flags |= 0x200 ; else _flags &= ~0x200 ; }
    int allocate() {  
	if (base() || unbuffered()) return 0;
	else return doallocate(); }
     
    void allocbuf() { if (base() == ((void *)0) ) doallocbuf(); }
    void doallocbuf();
    int in_avail() { return _IO_read_end - _IO_read_ptr; }
    int out_waiting() { return _IO_write_ptr - _IO_write_base; }
    streamsize sputn(const char* s, streamsize n) { return xsputn(s, n); }
    streamsize padn(char pad, streamsize n) { return _IO_padn(this, pad, n); }
    streamsize sgetn(char* s, streamsize n) { return _IO_sgetn(this, s, n); }
    int ignore(int);
    int get_column();
    int set_column(int);
    long sgetline(char* buf, size_t   n, char delim, int putback_delim);
    int sputc(int c) { return _IO_putc(c, this); }
    int sbumpc() { return _IO_getc(this); }
    int sgetc() { return ((  this  )->_IO_read_ptr >= (  this  )->_IO_read_end && __underflow (  this  ) == (-1)  ? (-1)  : *(unsigned char *) (  this  )->_IO_read_ptr)  ; }
    int snextc() {
	if (_IO_read_ptr >= _IO_read_end && __underflow(this) == (-1) )
	  return (-1) ;
	else return _IO_read_ptr++, sgetc(); }
    void stossc() { if (_IO_read_ptr < _IO_read_end) _IO_read_ptr++; }
    int vscan(char const *fmt0, __gnuc_va_list  ap, ios* stream = ((void *)0) );
    int scan(char const *fmt0 ...);
    int vform(char const *fmt0, __gnuc_va_list  ap);
    int form(char const *fmt0 ...);




    virtual streamsize sys_read(char* buf, streamsize size);
    virtual streamsize sys_write(const char*, streamsize);
    virtual streampos sys_seek(streamoff, _seek_dir);
    virtual int sys_close();
    virtual int sys_stat(void*);  

    virtual int showmanyc();
    virtual void imbue(void *);

};

 
 

class filebuf : public streambuf {
  protected:
    void init();
  public:
    static const int openprot;  
    filebuf();
    filebuf(int fd);
    filebuf(int fd, char* p, int len);



    ~filebuf();
    filebuf* attach(int fd);
    filebuf* open(const char *filename, const char *mode);
    filebuf* open(const char *filename, ios::openmode mode, int prot = 0664);
    virtual int underflow();
    virtual int overflow(int c = (-1) );
    int is_open() const { return _fileno >= 0; }
    int fd() const { return is_open() ? _fileno : (-1) ; }
    filebuf* close();
    virtual int doallocate();
    virtual streampos seekoff(streamoff, _seek_dir, int mode=ios::in|ios::out);
    virtual streambuf* setbuf(char* p, int len);
    streamsize xsputn(const char* s, streamsize n);
    streamsize xsgetn(char* s, streamsize n);
    virtual int sync();
  protected:  
 
    int is_reading() { return eback() != egptr(); }
    char* cur_ptr() { return is_reading() ?  gptr() : pptr(); }
     
    char* file_ptr() { return eGptr(); }
     
    virtual streamsize sys_read(char* buf, streamsize size);
    virtual streampos sys_seek(streamoff, _seek_dir);
    virtual streamsize sys_write(const char*, streamsize);
    virtual int sys_stat(void*);  
    virtual int sys_close();




};

inline void ios::init(streambuf* sb, ostream* tie_to) {
		_state = sb ? ios::goodbit : ios::badbit; _exceptions=0;
		_strbuf=sb; _tie = tie_to; _width=0; _fill=' ';

		_flags=ios::skipws|ios::dec;



		_precision=6; _arrays = 0; }

inline ios::ios(streambuf* sb, ostream* tie_to) { init(sb, tie_to); }

inline ios::~ios() {



     
     
    operator delete[] (_arrays);
}
}  

# 31 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/../../../../include/g++-3/iostream.h" 2 3


extern "C++" {
class istream; class ostream;
typedef ios& (*__manip)(ios&);
typedef istream& (*__imanip)(istream&);
typedef ostream& (*__omanip)(ostream&);

extern istream& ws(istream& ins);
extern ostream& flush(ostream& outs);
extern ostream& endl(ostream& outs);
extern ostream& ends(ostream& outs);

class ostream : virtual public ios
{
     
    void do_osfx();
  public:
    ostream() { }
    ostream(streambuf* sb, ostream* tied= ((void *)0) );
    int opfx() {
	if (!good()) return 0;
	else { if (_tie) _tie->flush();  ; return 1;} }
    void osfx() {  ;
		  if (flags() & (ios::unitbuf|ios::stdio))
		      do_osfx(); }
    ostream& flush();
    ostream& put(char c) { _strbuf->sputc(c); return *this; }





    ostream& write(const char *s, streamsize n);
    ostream& write(const unsigned char *s, streamsize n)
      { return write((const char*)s, n);}
    ostream& write(const signed char *s, streamsize n)
      { return write((const char*)s, n);}
    ostream& write(const void *s, streamsize n)
      { return write((const char*)s, n);}
    ostream& seekp(streampos);
    ostream& seekp(streamoff, _seek_dir);
    streampos tellp();
    ostream& form(const char *format ...);
    ostream& vform(const char *format, __gnuc_va_list  args);

    ostream& operator<<(char c);
    ostream& operator<<(unsigned char c) { return (*this) << (char)c; }
    ostream& operator<<(signed char c) { return (*this) << (char)c; }
    ostream& operator<<(const char *s);
    ostream& operator<<(const unsigned char *s)
	{ return (*this) << (const char*)s; }
    ostream& operator<<(const signed char *s)
	{ return (*this) << (const char*)s; }
    ostream& operator<<(const void *p);
    ostream& operator<<(int n);
    ostream& operator<<(unsigned int n);
    ostream& operator<<(long n);
    ostream& operator<<(unsigned long n);




    ostream& operator<<(short n) {return operator<<((int)n);}
    ostream& operator<<(unsigned short n) {return operator<<((unsigned int)n);}

    ostream& operator<<(bool b) { return operator<<((int)b); }

    ostream& operator<<(double n);
    ostream& operator<<(float n) { return operator<<((double)n); }

    ostream& operator<<(long double n);



    ostream& operator<<(__omanip func) { return (*func)(*this); }
    ostream& operator<<(__manip func) {(*func)(*this); return *this;}
    ostream& operator<<(streambuf*);



};

class istream : virtual public ios
{
     
protected:
    size_t   _gcount;

    int _skip_ws();
  public:
    istream(): _gcount (0) { }
    istream(streambuf* sb, ostream*tied= ((void *)0) );
    istream& get(char* ptr, int len, char delim = '\n');
    istream& get(unsigned char* ptr, int len, char delim = '\n')
	{ return get((char*)ptr, len, delim); }
    istream& get(char& c);
    istream& get(unsigned char& c) { return get((char&)c); }
    istream& getline(char* ptr, int len, char delim = '\n');
    istream& getline(unsigned char* ptr, int len, char delim = '\n')
	{ return getline((char*)ptr, len, delim); }
    istream& get(signed char& c)  { return get((char&)c); }
    istream& get(signed char* ptr, int len, char delim = '\n')
	{ return get((char*)ptr, len, delim); }
    istream& getline(signed char* ptr, int len, char delim = '\n')
	{ return getline((char*)ptr, len, delim); }
    istream& read(char *ptr, streamsize n);
    istream& read(unsigned char *ptr, streamsize n)
      { return read((char*)ptr, n); }
    istream& read(signed char *ptr, streamsize n)
      { return read((char*)ptr, n); }
    istream& read(void *ptr, streamsize n)
      { return read((char*)ptr, n); }
    istream& get(streambuf& sb, char delim = '\n');
    istream& gets(char **s, char delim = '\n');
    int ipfx(int need = 0) {
	if (!good()) { set(ios::failbit); return 0; }
	else {
	   ;
	  if (_tie && (need == 0 || rdbuf()->in_avail() < need)) _tie->flush();
	  if (!need && (flags() & ios::skipws)) return _skip_ws();
	  else return 1;
	}
    }
    int ipfx0() {  
	if (!good()) { set(ios::failbit); return 0; }
	else {
	   ;
	  if (_tie) _tie->flush();
	  if (flags() & ios::skipws) return _skip_ws();
	  else return 1;
	}
    }
    int ipfx1() {  
	if (!good()) { set(ios::failbit); return 0; }
	else {
	   ;
	  if (_tie && rdbuf()->in_avail() == 0) _tie->flush();
	  return 1;
	}
    }
    void isfx() {  ; }
    int get() { if (!ipfx1()) return (-1) ;
		else { int ch = _strbuf->sbumpc();
		       if (ch == (-1) ) set(ios::eofbit);
		       isfx();
		       return ch;
		     } }
    int peek();
    size_t   gcount() { return _gcount; }
    istream& ignore(int n=1, int delim = (-1) );
    int sync ();
    istream& seekg(streampos);
    istream& seekg(streamoff, _seek_dir);
    streampos tellg();
    istream& putback(char ch) {
	if (good() && _strbuf->sputbackc(ch) == (-1) ) clear(ios::badbit);
	return *this;}
    istream& unget() {
	if (good() && _strbuf->sungetc() == (-1) ) clear(ios::badbit);
	return *this;}
    istream& scan(const char *format ...);
    istream& vscan(const char *format, __gnuc_va_list  args);






    istream& operator>>(char*);
    istream& operator>>(unsigned char* p) { return operator>>((char*)p); }
    istream& operator>>(signed char*p) { return operator>>((char*)p); }
    istream& operator>>(char& c);
    istream& operator>>(unsigned char& c) {return operator>>((char&)c);}
    istream& operator>>(signed char& c) {return operator>>((char&)c);}
    istream& operator>>(int&);
    istream& operator>>(long&);




    istream& operator>>(short&);
    istream& operator>>(unsigned int&);
    istream& operator>>(unsigned long&);
    istream& operator>>(unsigned short&);

    istream& operator>>(bool&);

    istream& operator>>(float&);
    istream& operator>>(double&);
    istream& operator>>(long double&);
    istream& operator>>( __manip func) {(*func)(*this); return *this;}
    istream& operator>>(__imanip func) { return (*func)(*this); }
    istream& operator>>(streambuf*);
};

class iostream : public istream, public ostream
{
  public:
    iostream() { }
    iostream(streambuf* sb, ostream*tied= ((void *)0) );
};

class _IO_istream_withassign : public istream {
public:
  _IO_istream_withassign& operator=(istream&);
  _IO_istream_withassign& operator=(_IO_istream_withassign& rhs)
    { return operator= (static_cast<istream&> (rhs)); }
};

class _IO_ostream_withassign : public ostream {
public:
  _IO_ostream_withassign& operator=(ostream&);
  _IO_ostream_withassign& operator=(_IO_ostream_withassign& rhs)
    { return operator= (static_cast<ostream&> (rhs)); }
};

extern _IO_istream_withassign cin;
 
extern _IO_ostream_withassign cout, cerr;

extern _IO_ostream_withassign clog



;

extern istream& lock(istream& ins);
extern istream& unlock(istream& ins);
extern ostream& lock(ostream& outs);
extern ostream& unlock(ostream& outs);

struct Iostream_init { } ;   

inline ios& dec(ios& i)
{ i.setf(ios::dec, ios::dec|ios::hex|ios::oct); return i; }
inline ios& hex(ios& i)
{ i.setf(ios::hex, ios::dec|ios::hex|ios::oct); return i; }
inline ios& oct(ios& i)
{ i.setf(ios::oct, ios::dec|ios::hex|ios::oct); return i; }
}  


# 52 "nonport.cpp" 2



  
# 1 "/usr/include/dirent.h" 1 3
 

















 








extern "C" { 



# 45 "/usr/include/dirent.h" 3


 














# 1 "/usr/include/bits/dirent.h" 1 3
 





















struct dirent
  {

    __ino_t d_ino;
    __off_t d_off;




    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];		 
  };

# 46 "/usr/include/bits/dirent.h" 3








# 62 "/usr/include/dirent.h" 2 3






 




























 
enum
  {
    DT_UNKNOWN = 0,

    DT_FIFO = 1,

    DT_CHR = 2,

    DT_DIR = 4,

    DT_BLK = 6,

    DT_REG = 8,

    DT_LNK = 10,

    DT_SOCK = 12,

    DT_WHT = 14

  };

 





 

typedef struct __dirstream DIR;

 

extern DIR *opendir (const  char *__name)  ;

 

extern int closedir (DIR *__dirp)  ;

 







extern struct dirent *readdir (DIR *__dirp)  ;













 


extern int readdir_r (DIR *   __dirp,
		      struct dirent *   __entry,
		      struct dirent **   __result)  ;
# 176 "/usr/include/dirent.h" 3









 
extern void rewinddir (DIR *__dirp)  ;




 
extern void seekdir (DIR *__dirp, long int __pos)  ;

 
extern long int telldir (DIR *__dirp)  ;




 
extern int dirfd (DIR *__dirp)  ;






 
# 1 "/usr/include/bits/posix1_lim.h" 1 3
 

















 









 

 


 


 


 


 


 


 


 



 


 


 


 


 



 


 


 


 


 


 


 


 


 


 


 


 



 


 


 


 


 



 
# 1 "/usr/include/bits/local_lim.h" 1 3
 


















 














 
# 1 "/usr/include/linux/limits.h" 1 3



















# 36 "/usr/include/bits/local_lim.h" 2 3


 




 




 





 

 


 

 


 

 


 



 


 

# 126 "/usr/include/bits/posix1_lim.h" 2 3








 







# 209 "/usr/include/dirent.h" 2 3


 








# 1 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-slackware-linux/2.95.3/include/stddef.h" 3




 





















# 220 "/usr/include/dirent.h" 2 3


 




extern int scandir (const  char *   __dir,
		    struct dirent ***   __namelist,
		    int (*__selector) (const  struct dirent *),
		    int (*__cmp) (const  void *, const  void *))  ;
# 242 "/usr/include/dirent.h" 3











 

extern int alphasort (const  void *__e1, const  void *__e2)
        ;
# 266 "/usr/include/dirent.h" 3







# 293 "/usr/include/dirent.h" 3


 




extern __ssize_t getdirentries (int __fd, char *   __buf,
				size_t __nbytes,
				__off_t *   __basep)  ;
# 313 "/usr/include/dirent.h" 3










} 


# 55 "nonport.cpp" 2



# 1 "nonport.h" 1
 
 
 
 




# 1 "typ.h" 1
 
 
 




 
typedef unsigned char byte;
typedef signed char signed_byte;


 
 


 





 







 


       
template <class T>
inline T min(T const &a, T const &b)
{
  return a<b? a:b;
}

template <class T>
inline T max(T const &a, T const &b)
{
  return a>b? a:b;
}


# 58 "typ.h"



 
 
 
 
 



 



 





 
 







 
 
template <class T>
inline T div_up(T const &x, T const &y)
{ return (x + y - 1) / y; }


 














  
 
 
 
 




# 9 "nonport.h" 2



 
 
 
 
 
typedef void (*NonportFailFunc)(char const *syscallName, char const *context);
   
   
extern NonportFailFunc nonportFail;

 
void defaultNonportFail(char const *syscallName, char const *context);



 
void setRawMode(bool raw);

 
 
char getConsoleChar();


 
long getMilliseconds();


 
 
bool limitFileAccess(char const *fname);


 
 
 
 
int getProcessId();


 
 
 
 
 
bool createDirectory(char const *dirname);

 
 
bool changeDirectory(char const *dirname);

 
 
bool getCurrentDirectory(char *dirname, int dirnameLen);


 
typedef bool (*PerFileFunc)(char const *name, void *extra);
   
   
   
void applyToCwdContents(PerFileFunc func, void *extra= ((void *)0) );

 
void applyToDirContents(char const *dirName,
                        PerFileFunc func, void *extra= ((void *)0) );


 
bool isDirectory(char const *path);


 
bool removeFile(char const *fname);


 
void getCurrentDate(int &month, int &day, int &year);
   
   
   
   


 
void portableSleep(unsigned seconds);


 
void getCurrentUsername(char *buffer, int buflen);


 
void readNonechoString(char *buffer, int buflen, char const *prompt);


 
bool fileOrDirectoryExists(char const *name);


 
 
 
 
 
bool ensurePath(char const *filename, bool isDirectory);


 
 
bool hasSystemCryptoRandom();

 
 
 
unsigned getSystemCryptoRandom();





# 58 "nonport.cpp" 2



NonportFailFunc nonportFail = defaultNonportFail;

void defaultNonportFail(char const *, char const *)
{}

 
inline void fail(char const *call, char const *ctx= ((void *)0) )
{
  nonportFail(call, ctx);
}


void setRawMode(bool raw)
{




    int res;
    if (raw) {
       
      res = system("stty -echo raw");
    }
    else {
       
      res = system("stty echo -raw");
    }

    if (res != 0) {
       
    }

}


 
 
char getConsoleChar()
{





     
    int ch = getchar();
    if (ch == (-1) ) {
      fail("getchar", "getConsoleChar");
    }
    return ch;

}


 
 
long getMilliseconds()
{





     
     
     
    struct timeval tv;
    gettimeofday(&tv, ((void *)0) );
       
       

     
     
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;

}


bool limitFileAccess(char const *fname)
{
   
  if (chmod(fname, 0600) != 0) {
    fail("chmod", fname);
    return false;
  }
  else {
    return true;
  }
}


bool createDirectory(char const *dirname)
{
  int res;




     
    res = mkdir(dirname, 0400   | 0200   | 0100  );


  if (res != 0) {
    fail("mkdir", dirname);
    return false;
  }
  else {
    return true;
  }
}


bool changeDirectory(char const *dirname)
{
  if (0!=chdir(dirname)) {
    fail("chdir", dirname);
    return false;
  }
  else {
    return true;
  }
}


bool getCurrentDirectory(char *dirname, int len)
{
  bool ok = getcwd(dirname, len) != ((void *)0) ;
  if (!ok) {
    fail("getcwd");
  }
  return ok;
}


bool removeFile(char const *fname)
{
  bool ok = unlink(fname) == 0;
  if (!ok) {
    fail("unlink", fname);
  }
  return ok;
}


 
 
 
void getCurrentDate(int &month, int &day, int &year)
{
   
   
  
  tzset();
  

   
  time_t unixTime;
  time(&unixTime);

   
  struct tm *t = localtime(&unixTime);

   
  month = t->tm_mon + 1;
  day = t->tm_mday;
  year = t->tm_year + 1900;     
}


void portableSleep(unsigned seconds)
{
   
  sleep(seconds);
}


void getCurrentUsername(char *buf, int buflen)
{
  







    




    char const *temp;
    struct passwd *pw = getpwuid(geteuid());
    if (!pw) {
      fail("getpwuid(geteuid())");
      temp = "(unknown)";
    }
    else {
      temp = pw->pw_name;
    }

    strncpy(buf, temp, buflen);
  
}


 
static void nonechoLoop(char *buf, int len)
{
  int cursor = 0;
  for(;;) {
    char ch = getConsoleChar();
    switch (ch) {
      case '\r':     
        buf[cursor] = 0;
        return;

      case '\b':     
        if (cursor > 0) {
          cursor--;
        }
        break;

      default:
        buf[cursor++] = ch;
        if (cursor >= len-1) {
           
          buf[len-1] = 0;
          return;
        }
        break;
    }
  }
}


void readNonechoString(char *buf, int len, char const *prompt)
{
  cout << prompt;
  cout.flush();

  setRawMode(true);

  try {
    nonechoLoop(buf, len);
  }
  catch (...) {
    setRawMode(false);     
    throw;
  }

  setRawMode(false);

  cout << "\n";
  cout.flush();
}


void applyToCwdContents(PerFileFunc func, void *extra)
{
  applyToDirContents(".", func, extra);
}


void applyToDirContents(char const *dirName,
                        PerFileFunc func, void *extra)
{
   
   
   
   
   
   
  
# 355 "nonport.cpp"

    DIR *dir = opendir(dirName);
    if (!dir) {
      fail("opendir", dirName);
      return;
    }

    for(;;) {
       
      struct dirent *ent = readdir(dir);
      if (!ent) {
	break;      
      }

      if (!func(ent->d_name, extra)) {
	break;      
      }
    }

    if (closedir(dir) != 0) {
      fail("closedir", dirName);
    }
  
}


bool isDirectory(char const *path)
{
  struct stat st;
  if (0!=stat(path, &st)) {
    fail("stat", path);
    return false;
  }
  


    return ((( ( st.st_mode ) ) & 0170000 ) == (  0040000  ))  ;
  
}


bool fileOrDirectoryExists(char const *path)
{
  struct stat st;
  if (0!=stat(path, &st)) {
    return false;      
  }
  else {
    return true;
  }
}


 
 
bool ensurePath(char const *filename, bool isDirectory)
{
   
  int len = strlen(filename);
  char *temp = new char[len+1];
  strcpy(temp, filename);

  if (isDirectory) {
    len++;     
  }

   
   
  for (int i=1; i < len; i++) {
    if (strchr("/" , temp[i])) {
       
      temp[i] = '\0';
      if (!fileOrDirectoryExists(temp)) {
         
        if (!createDirectory(temp)) {
          delete[] temp;
          return false;
        }
      }
      temp[i] = '/' ;       
    }
  }

   
  delete[] temp;
  return true;
}

 
 
bool hsrcHelper()
{
  
     
    int fd = open("/dev/random", 00 );
    if (fd < 0) {
      return false;
    }

     
    if (close(fd) < 0) {
      perror("close");
      return false;       
    }

    return true;

  


}

bool hasSystemCryptoRandom()
{
  static bool cached = false;
  static bool cachedAnswer;

  if (!cached) {
    cachedAnswer = hsrcHelper();
    cached = true;
  }
  
  return cachedAnswer;
}


 
 
 
 
 
 
unsigned getSystemCryptoRandom()
{
  
     
    int fd = open("/dev/random", 00 );
    if (!fd) {
      perror("open");
      exit(2);
    }

     
    union {
      unsigned ret;
      char c[4];
    };
    int got = 0;

    while (got < 4) {
      int ct = read(fd, c+got, 4-got);
      if (ct < 0) {
       	perror("read");
       	exit(2);
      }
      if (ct == 0) {
        fprintf(stderr , "got 0 bytes from /dev/random.. it's supposed to block!!\n");
        exit(2);
      }
      got += ct;
    }

    if (close(fd) < 0) {
      perror("close");
      exit(2);
    }

    return ret;

  




}


int getProcessId()
{
  
    return getpid();

  



}


 
# 705 "nonport.cpp"



 
# 753 "nonport.cpp"


