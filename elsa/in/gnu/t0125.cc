// dsw: from cqual/tests/linux/rtc.i; I think these are from the real
// linux kernel

typedef struct { volatile int counter; } atomic_t;
static __inline__ void atomic_add(int i, volatile atomic_t *v)
{
  __asm__ __volatile__("lock ; "  "addl %1,%0"
                       // two colons here
                       :"=m" ((*(volatile struct { int a[100]; } *) v ) )
                       :"ir" (i), "m" ((*(volatile struct { int a[100]; } *) v ) ));
}

typedef struct {
  volatile unsigned int lock;
} rwlock_t;
extern inline void read_lock(rwlock_t *rw)
{
  do {
    if (true) // don't have this: (__builtin_constant_p( rw ))
      asm volatile
        ("lock ; "
         "subl $1,%0\n\t"
         "js 2f\n"
         "1:\n"
         ".section .text.lock,\"ax\"\n"
         "2:\tpushl %%eax\n\t"
         "leal %0,%%eax\n\t"
         "call "
         "__read_lock_failed"
         "\n\t"
         "popl %%eax\n\t"
         "jmp 1b\n"
         ".previous"
         :"=m" ((*(__dummy_lock_t *)(   rw   )) ) // only one colon here
         ) ;
    else
      asm volatile
        ("lock ; "
         "subl $1,(%0)\n\t"
         "js 2f\n"
         "1:\n"
         ".section .text.lock,\"ax\"\n"
         "2:\tcall "
         "__read_lock_failed"
         "\n\t"
         "jmp 1b\n"
         ".previous"
         :
         :"a" (  rw  )
         : "memory"             // three colons here
         ) ;
  } while (0) ;
}

// this is another copy but retaining the double colon, which is a
// single token in C++
extern inline void read_lock2(rwlock_t *rw)
{
	do {
          if (true)//__builtin_constant_p( rw ))
            asm volatile("lock ; "  "subl $1,%0\n\t" "js 2f\n" "1:\n" ".section .text.lock,\"ax\"\n" "2:\tpushl %%eax\n\t" "leal %0,%%eax\n\t" "call "     "__read_lock_failed"   "\n\t" "popl %%eax\n\t" "jmp 1b\n" ".previous" :"=m" ((*(__dummy_lock_t *)(   rw   )) )) ; else asm volatile("lock ; "  "subl $1,(%0)\n\t" "js 2f\n" "1:\n" ".section .text.lock,\"ax\"\n" "2:\tcall "     "__read_lock_failed"   "\n\t" "jmp 1b\n" ".previous"
  ::"a" (  rw  )                // NOTE double colon!
  : "memory") ; 
        } while (0) ;
}

void oink() {
  // three-colons now works also!
  asm ("asdfasd" ::: "a"(rw) );
  // and four
  asm ("asdfasd" :::: "a"(rw) );
}
