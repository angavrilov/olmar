// rdtsc.c
// simple test of the 'rdtsc' (read time-stamp count) instruction
// NOTE: this entire file assumes we're compiling to a 32-bit machine
// (i.e. 'unsigned' is a 32-bit quantity)

// see also:
//   http://cedar.intel.com/software/idap/media/pdf/rdtscpm1.pdf
//     html: http://www.google.com/search?q=cache:N43gwOb7_PUC:cedar.intel.com/software/idap/media/pdf/rdtscpm1.pdf+rdtsc&hl=en
//   http://www.x86-64.org/lists/bugs/msg00621.html


#include <stdio.h>    // printf

#ifdef RDTSC_SOURCE
// this function yields a 64-bit cycle count, writing into
// two variables passed by address
void rdtsc(unsigned *lowp, unsigned *highp)
{
  unsigned int low, high;

  // this is gcc inline assembler syntax; it says that the
  // instruction writes to EAX ("=a") and EDX ("=d"), but that
  // I would like it to then copy those values into 'low' and
  // 'high', respectively
  asm volatile ("rdtsc" : "=a" (low), "=d" (high));

  // now copy into the variables passed by address
  *lowp = low;
  *highp = high;
}

#else // RDTSC_SOURCE

// this is the binary instructions that gcc-2.95.3 (optimization -O2)
// produces for the above code; it will work regardless of the current
// compiler's assembler syntax (even if the current compiler doesn't
// have *any* inline assembler)
char const rdtsc_instructions[] = {
  0x55,                                 // push   %ebp
  0x89, 0xe5,                           // mov    %esp,%ebp
  0x53,                                 // push   %ebx
  0x8b, 0x4d, 0x08,                     // mov    0x8(%ebp),%ecx
  0x8b, 0x5d, 0x0c,                     // mov    0xc(%ebp),%ebx
  0x0f, 0x31,                           // rdtsc
  0x89, 0x01,                           // mov    %eax,(%ecx)
  0x89, 0x13,                           // mov    %edx,(%ebx)
  0x5b,                                 // pop    %ebx
  0xc9,                                 // leave
  0xc3,                                 // ret
  0x90,                                 // nop
};

// now declare a function pointer (global variable) which
// points at the code instructions above; thus, you can
// call the 'rdtsc' routine as if it were a regular function
void (*rdtsc)(unsigned *lowp, unsigned *highp) =
  (void (*)(unsigned*, unsigned*))rdtsc_instructions;

#endif // RDTSC_SOURCE


#ifdef __GNUC__
// this uses gcc's "long long" to represent the 64-bit
// quantity a little more easily
unsigned long long rdtsc_ll()
{
  unsigned int low, high;
  unsigned long long ret;

  rdtsc(&low, &high);

  ret = high;
  ret <<= 32;
  ret += low;

  return ret;
}
#endif // __GNUC__


#ifdef RDTSC_MAIN
int main()
{
  #ifdef __GNUC__
    unsigned long long v = rdtsc_ll();
    printf("rdtsc: %lld\n", v);
  #endif // __GNUC__

  // this segment should work on any compiler, by virtue
  // of only using 32-bit quantities
  {
    unsigned low, high;
    rdtsc(&low, &high);
    printf("rdtsc high=%u, low=%u\n", high, low);
  }

  return 0;
}
#endif // RDTSC_MAIN
