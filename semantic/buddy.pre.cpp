/*
 * $Id$
 *
 * Author: Tjark Weber
 * (c) 2007 Radboud University
 *
 * This file was initially obtained through
 *
 *   g++ -E -o buddy.pre.cpp buddy.cpp
 *
 * but has been edited manually since then.
 *
 * Modifications & caveats to make this code compile with (the current version
 * of) the semantics compiler:
 *
 * - virtual functions are not allowed
 * - access modifiers (private/public/protected) are unsupported
 * - class inheritance/base classes are not allowed
 * - templates are not allowed
 * - assembler statements will lead to unverifiable PVS code
 * - global variables are not allowed
 * - nested class/enum declarations (inside another class) are not allowed
 */


# 1 "buddy.cpp"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "buddy.cpp"
# 11 "buddy.cpp"
# 1 "include/assert.h" 1
# 11 "include/assert.h"
       

# 1 "include/compiler.h" 1
# 11 "include/compiler.h"
       
# 14 "include/assert.h" 2
# 1 "include/stdio.h" 1
# 11 "include/stdio.h"
       


# 1 "include/console_screen.h" 1
# 11 "include/console_screen.h"
       

# 1 "include/console.h" 1
# 11 "include/console.h"
       

# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stdarg.h" 1 3 4
# 43 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stdarg.h" 3 4
// obscure built-in types are not supported by the semantics compiler
/*
typedef __builtin_va_list __gnuc_va_list;
*/
# 105 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stdarg.h" 3 4
/*
typedef __gnuc_va_list va_list;
*/
# 14 "include/console.h" 2

# 1 "include/types.h" 1
# 11 "include/types.h"
       

# 1 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 1 3 4
# 152 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 3 4
typedef int ptrdiff_t;
# 214 "/usr/lib/gcc/i386-redhat-linux/4.0.1/include/stddef.h" 3 4
typedef unsigned int size_t;
# 14 "include/types.h" 2

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

typedef unsigned long mword;
typedef unsigned long long Paddr;
# 16 "include/console.h" 2

/*
class Console
{
    private:
        virtual void
        putc (char c) = 0;

        void
        print_number (uint64 val, unsigned base, unsigned width, unsigned flags);

        inline void
        print_str (char const *str, unsigned width, unsigned precs);

        __attribute__((format (printf, (2),(0))))
        inline char const *
        print_format (char const *format, va_list& args);

    protected:
        bool initialized;

    public:
        Console() : initialized (false) {}

        __attribute__((format (printf, (2),(0))))
        void
        vprintf (char const *format, va_list args);
};
*/
# 14 "include/console_screen.h" 2
# 1 "include/memory.h" 1
# 11 "include/memory.h"
       
# 15 "include/console_screen.h" 2
# 1 "include/string.h" 1
# 11 "include/string.h"
       
# 21 "include/string.h"
extern "C"
__attribute__((always_inline)) __attribute__((nonnull))
inline void *
memcpy (void *d, void const *s, size_t n)
{
  // uninitialized variables are not supported by the semantics compiler
  /*
    mword dummy;
  */
    mword dummy = 0UL;
    asm volatile ("rep; movsb"
                  : "=D" (dummy), "+S" (s), "+c" (n)
                  : "0" (d)
                  : "memory");
    return d;
}

extern "C"
__attribute__((always_inline)) __attribute__((nonnull))
inline void *
memset (void *d, int c, size_t n)
{
  // uninitialized variables are not supported by the semantics compiler
  /*
    mword dummy;
  */
    mword dummy = 0UL;
    asm volatile ("rep; stosb"
                  : "=D" (dummy), "+c" (n)
                  : "0" (d), "a" (c)
                  : "memory");
    return d;
}

extern "C"
__attribute__((always_inline)) __attribute__((nonnull))
inline int
strcmp (char const *s1, char const *s2)
{
    while (*s1 && *s1 == *s2)
        s1++, s2++;

    return *s1 - *s2;
}

__attribute__((always_inline))
// return type int requires integral conversions that are not recorded properly
// by Elsa; hence the semantics compiler is not (yet) able to insert them
/*
inline int
*/
inline unsigned
bit_scan_reverse (unsigned val)
{
    if (__builtin_expect((!val), false))
        return -1;

    asm volatile ("bsr %1, %0" : "=r" (val) : "rm" (val));

    return val;
}
# 16 "include/console_screen.h" 2

/*
class Console_screen : public Console
{
    private:
        unsigned _num_row;
        unsigned _row;
        unsigned _col;

        __attribute__((always_inline))
        inline void
        clear_all()
        {
            memset (reinterpret_cast<void *>(0xcfbff000), 0, 160 * _num_row);
        }

        __attribute__((always_inline))
        inline void
        clear_row (unsigned row)
        {
            memcpy (reinterpret_cast<void *>(0xcfbff000), reinterpret_cast<void *>(0xcfbff000 + 160), 160 * row);
            memset (reinterpret_cast<void *>(0xcfbff000 + 160 * row), 0, 160);
        }

        void
        putc (char c);

    public:
        enum Color
        {
            COLOR_BLACK = 0x0,
            COLOR_BLUE = 0x1,
            COLOR_GREEN = 0x2,
            COLOR_CYAN = 0x3,
            COLOR_RED = 0x4,
            COLOR_MAGENTA = 0x5,
            COLOR_YELLOW = 0x6,
            COLOR_WHITE = 0x7,
            COLOR_LIGHT_BLACK = 0x8,
            COLOR_LIGHT_BLUE = 0x9,
            COLOR_LIGHT_GREEN = 0xa,
            COLOR_LIGHT_CYAN = 0xb,
            COLOR_LIGHT_RED = 0xc,
            COLOR_LIGHT_MAGENTA = 0xd,
            COLOR_LIGHT_YELLOW = 0xe,
            COLOR_LIGHT_WHITE = 0xf
        };

        Console_screen() : _num_row (25), _row (0), _col (0) {}

        __attribute__((section (".init")))
        void
        init();

        __attribute__((always_inline))
        inline void
        put (unsigned row, unsigned col, Color color, char c)
        {
            *reinterpret_cast<unsigned short volatile *>(0xcfbff000 + row * 160 + col * 2) = color << 8 | c;
        }

        unsigned
        init_spinner (unsigned cpu);
};
*/
# 15 "include/stdio.h" 2
# 1 "include/console_serial.h" 1
# 11 "include/console_serial.h"
       



# 1 "include/io.h" 1
# 11 "include/io.h"
       



/*
class Io
{
    public:
        template <typename T> __attribute__((always_inline))
        static inline T
        in (unsigned port)
        {
            T val;
            asm volatile ("in %w1, %0" : "=a" (val) : "Nd" (port));
            return val;
        }

        template <typename T> __attribute__((always_inline))
        static inline void
        out (unsigned port, T val)
        {
            asm volatile ("out %0, %w1" : : "a" (val), "Nd" (port));
        }
};
*/
# 16 "include/console_serial.h" 2


/*
class Console_serial : public Console
{
    private:
        enum Port
        {
            RHR = 0,
            THR = 0,
            IER = 1,
            ISR = 2,
            FCR = 2,
            LCR = 3,
            MCR = 4,
            LSR = 5,
            MSR = 6,
            SPR = 7,
            DLR_LOW = 0,
            DLR_HIGH = 1,
        };

        enum
        {
            IER_RHR_INTR = 1u << 0,
            IER_THR_INTR = 1u << 1,
            IER_RLS_INTR = 1u << 2,
            IER_MOS_INTR = 1u << 3,

            FCR_FIFO_ENABLE = 1u << 0,
            FCR_RECV_FIFO_RESET = 1u << 1,
            FCR_TMIT_FIFO_RESET = 1u << 2,
            FCR_DMA_MODE = 1u << 3,
            FCR_RECV_TRIG_LOW = 1u << 6,
            FCR_RECV_TRIG_HIGH = 1u << 7,

            ISR_INTR_STATUS = 1u << 0,
            ISR_INTR_PRIO = 7u << 1,

            LCR_DATA_BITS_5 = 0u << 0,
            LCR_DATA_BITS_6 = 1u << 0,
            LCR_DATA_BITS_7 = 2u << 0,
            LCR_DATA_BITS_8 = 3u << 0,
            LCR_STOP_BITS_1 = 0u << 2,
            LCR_STOP_BITS_2 = 1u << 2,
            LCR_PARITY_ENABLE = 1u << 3,
            LCR_PARITY_EVEN = 1u << 4,
            LCR_PARITY_FORCE = 1u << 5,
            LCR_BREAK = 1u << 6,
            LCR_DLAB = 1u << 7,

            MCR_DTR = 1u << 0,
            MCR_RTS = 1u << 1,
            MCR_GPO_1 = 1u << 2,
            MCR_GPO_2 = 1u << 3,
            MCR_LOOP = 1u << 4,

            LSR_RECV_READY = 1u << 0,
            LSR_OVERRUN_ERROR = 1u << 1,
            LSR_PARITY_ERROR = 1u << 2,
            LSR_FRAMING_ERROR = 1u << 3,
            LSR_BREAK_INTR = 1u << 4,
            LSR_TMIT_HOLD_EMPTY = 1u << 5,
            LSR_TMIT_EMPTY = 1u << 6
        };

        unsigned base;

        __attribute__((always_inline))
        inline uint8
        in (Port port)
        {
            return Io::in<uint8>(base + port);
        }

        __attribute__((always_inline))
        inline void
        out (Port port, uint8 val)
        {
            Io::out (base + port, val);
        }

        void
        putc (char c);

    public:
        __attribute__((section (".init")))
        void
        init();
};
*/
# 16 "include/stdio.h" 2
# 1 "include/cpu.h" 1
# 11 "include/cpu.h"
       



# 1 "include/vectors.h" 1
# 11 "include/vectors.h"
       
# 16 "include/cpu.h" 2




/*
class Cpu
{
    private:
        static char const * const vendor_string[];

        __attribute__((always_inline))
        static inline void
        check_features();

        __attribute__((always_inline))
        static inline void
        setup_sysenter();

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

        enum Cr0
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

        enum Cr4
        {
            CR4_DE = 1ul << 3,
            CR4_PSE = 1ul << 4,
            CR4_PAE = 1ul << 5,
            CR4_PGE = 1ul << 7,
            CR4_SMXE = 1ul << 14,
        };

        enum Eflags
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


        static unsigned id __attribute__((section (".cpulocal,\"w\",@nobits#"))) __attribute__((weak));
        static unsigned package __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned core __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned thread __attribute__((section (".cpulocal,\"w\",@nobits#")));

        static Vendor vendor __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned platform __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned family __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned model __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned stepping __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned brand __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned patch __attribute__((section (".cpulocal,\"w\",@nobits#")));

        static uint32 name[12] __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static uint32 features[4] __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static bool bsp __attribute__((section (".cpulocal,\"w\",@nobits#")));


        static unsigned spinner __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned lapic_timer_count __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned lapic_error_count __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned lapic_perfm_count __attribute__((section (".cpulocal,\"w\",@nobits#")));
        static unsigned irqs[48] __attribute__((section (".cpulocal,\"w\",@nobits#")));

        static void
        init();

        static void
        wakeup_ap();

        static void
        delay (unsigned us);

        __attribute__((always_inline))
        static inline bool
        feature (Feature feat)
        {
            return features[feat / 32] & 1u << feat % 32;
        }

        __attribute__((always_inline))
        static inline void
        set_feature (unsigned feat)
        {
            features[feat / 32] |= 1u << feat % 32;
        }

        __attribute__((always_inline))
        static inline void
        pause()
        {
            asm volatile ("pause");
        }

        __attribute__((always_inline))
        static inline uint64
        time()
        {
            uint64 val;
            asm volatile ("rdtsc" : "=A" (val));
            return val;
        }

        __attribute__((always_inline)) __attribute__((noreturn))
        static inline void
        shutdown()
        {
            for (;;)
                asm volatile ("cli; hlt");
        }

        __attribute__((always_inline))
        static inline void
        cpuid (unsigned leaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid"
                          : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                          : "a" (leaf));
        }

        __attribute__((always_inline))
        static inline void
        cpuid (unsigned leaf, unsigned subleaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid"
                          : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                          : "a" (leaf), "c" (subleaf));
        }

        __attribute__((always_inline))
        static inline mword
        get_cr0()
        {
            mword cr0;
            asm volatile ("mov %%cr0, %0" : "=r" (cr0));
            return cr0;
        }

        __attribute__((always_inline))
        static inline void
        set_cr0 (mword cr0)
        {
            asm volatile ("mov %0, %%cr0" : : "r" (cr0));
        }

        __attribute__((always_inline))
        static inline mword
        get_cr4()
        {
            mword cr4;
            asm volatile ("mov %%cr4, %0" : "=r" (cr4));
            return cr4;
        }

        __attribute__((always_inline))
        static inline void
        set_cr4 (mword cr4)
        {
            asm volatile ("mov %0, %%cr4" : : "r" (cr4));
        }

        __attribute__((always_inline))
        static inline void
        flush_cache()
        {
            asm volatile ("wbinvd" : : : "memory");
        }
};
*/
# 17 "include/stdio.h" 2
# 32 "include/stdio.h"
enum TRACE_ENUM {
  // anonymous enums are not supported by the semantics compiler
    TRACE_CPU = 1u << 0,
    TRACE_VMX = 1u << 1,
    TRACE_SMX = 1u << 2,
    TRACE_APIC = 1u << 3,
    TRACE_MEMORY = 1u << 4,
    TRACE_PTAB = 1u << 5,
    TRACE_TIMER = 1u << 6,
    TRACE_INTERRUPT = 1u << 7,
    TRACE_ACPI = 1u << 8,
    TRACE_KEYB = 1u << 9
};



// global variables (even const ones) are not supported by the semantics
// compiler
/*
unsigned const trace_mask = (TRACE_CPU |
			     TRACE_VMX |
			     TRACE_SMX |
			     TRACE_APIC |
			     TRACE_MEMORY |
			     
			     TRACE_TIMER |
			     
			     TRACE_ACPI |
			     TRACE_KEYB |
			     
			     0);
*/

// these function declarations are not supported by the semantics compiler yet
// ... which probably has more to do with the missing implementation than with
// the printf format attribute.
/*
__attribute__((format (printf, (1),(0))))
void
vprintf (char const *format, va_list args);

__attribute__((format (printf, (1),(2))))
void
printf (char const *format, ...);

__attribute__((format (printf, (1),(2)))) __attribute__((noreturn))
void
panic (char const *format, ...);
*/

/*
extern Console_screen screen;
extern Console_serial serial;
*/
# 15 "include/assert.h" 2
# 12 "buddy.cpp" 2
# 1 "include/buddy.h" 1
# 11 "include/buddy.h"
       


# 1 "include/spinlock.h" 1
# 11 "include/spinlock.h"
       




/*
class Spinlock
{
    private:
        uint8 _lock;

    public:
        __attribute__((always_inline))
        inline
        Spinlock() : _lock (1) {}

        __attribute__((always_inline))
        inline void
        lock()
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
        inline void
        unlock()
        {
            asm volatile ("movb $1, %0" : "=m" (_lock) : : "memory");
        }
};
*/
# 15 "include/buddy.h" 2

// moved here because the semantics compiler does not allow nested class
// declarations
        class Block
        {
            public:
                Block * prev;
                Block * next;
                unsigned short ord;
                unsigned short tag;
        };

// moved here because the semantics compiler does not allow nested enum
// declarations
                enum UsedFreeEnum {
                    Used = 0,
                    Free = 1
                };


class Buddy
{
    private:
/*
        Spinlock lock;
*/
        signed long max_idx;
        signed long min_idx;
        mword phys_addr;
        mword p_base;
        mword l_base;
        unsigned order;
        Block * index;
        Block * head;

  // static variables are not supported by the semantics compiler
  /*
  static unsigned const page_ord = 12;
  static unsigned const page_size = 1u << page_ord;
  static unsigned const page_mask = ~(page_size - 1);
  */
  unsigned page_ord;
  unsigned page_size;
  unsigned page_mask;

        __attribute__((always_inline))
        inline signed long
        block_to_index (Block *b)
        {
            return b - index;
        }

        __attribute__((always_inline))
        inline Block *
        index_to_block (signed long i)
        {
            return index + i;
        }

        __attribute__((always_inline))
        inline signed long
        page_to_index (mword l_addr)
        {
            return l_addr / page_size - l_base / page_size;
        }

        __attribute__((always_inline))
        inline mword
        index_to_page (signed long i)
        {
            return l_base + i * page_size;
        }

        __attribute__((always_inline))
        inline mword
        page_to_frame (mword l_addr)
        {
            return l_addr - l_base + p_base;
        }

        __attribute__((always_inline))
        inline mword
        frame_to_page (mword p_addr)
        {
            return p_addr - p_base + l_base;
        }

    public:
  // static variables are not supported by the semantics compiler
  /*
        static Buddy allocator;
  */

        __attribute__((section (".init")))
        Buddy (mword p_addr, mword l_addr, mword f_addr, size_t size);

        void *
        alloc (unsigned ord, bool zero = false);

        void
        free (mword addr);

        __attribute__((always_inline))
	// static variables are not supported by the semantics compiler
	  inline /*static*/ mword
        phys()
        {
	  // static variables are not supported by the semantics compiler
	  return /*allocator.*/phys_addr;
        }

        __attribute__((always_inline))
	// static variables are not supported by the semantics compiler
	  inline /*static*/ void *
        phys_to_ptr (mword p_addr)
        {
	  // static variables are not supported by the semantics compiler
	  return reinterpret_cast<void *>(/*allocator.*/frame_to_page (p_addr));
        }

        __attribute__((always_inline))
	// static variables are not supported by the semantics compiler
	  inline /*static*/ mword
        ptr_to_phys (void *l_addr)
        {
	  // static variables are not supported by the semantics compiler
	  return /*allocator.*/page_to_frame (reinterpret_cast<mword>(l_addr));
        }
};
# 13 "buddy.cpp" 2
# 1 "include/initprio.h" 1
# 11 "include/initprio.h"
       
# 14 "buddy.cpp" 2
# 1 "include/lock_guard.h" 1
# 11 "include/lock_guard.h"
       



/*
template <typename T>
class Lock_guard
{
    private:
        T &_lock;

    public:
        __attribute__((always_inline)) inline
        Lock_guard (T &l) : _lock (l)
        {
            _lock.lock();
        }

        __attribute__((always_inline)) inline
        ~Lock_guard()
        {
            _lock.unlock();
        }
};
*/
# 15 "buddy.cpp" 2


// extern variables are not supported by the semantics compiler
/*
extern char _mempool_p, _mempool_l, _mempool_f;
*/



// initialization of static/global vars is not supported yet
/*
__attribute__((init_priority((((100 + 1) + 1)))))
Buddy Buddy::allocator (reinterpret_cast<mword>(&_mempool_p),
                        reinterpret_cast<mword>(&_mempool_l),
                        reinterpret_cast<mword>(&_mempool_f),
                        0x00800000);
*/


// added for the semantics compiler
int main() {
  char _mempool_p = '\0';
  char _mempool_l = '\0';
  char  _mempool_f = '\0';

  Buddy allocator(reinterpret_cast<mword>(&_mempool_p),
		  reinterpret_cast<mword>(&_mempool_l),
		  reinterpret_cast<mword>(&_mempool_f),
		  0x00800000U);

  return 0;
}



Buddy::Buddy (mword p_addr, mword l_addr, mword f_addr, size_t size)
{

  // moved here because static variables are not supported by the semantics
  // compiler
  page_ord  = 12;
  page_size = 1u << page_ord;
  page_mask = ~(page_size - 1);


    phys_addr = p_addr;


    unsigned bit = bit_scan_reverse (size);


    unsigned mask = (1u << bit) - 1;
    p_base = (p_addr + mask) & ~mask;
    l_base = p_base - p_addr + l_addr;


    order = bit - page_ord + 1;

    // copied here because global variables are not supported by the semantics
    // compiler; made int instead of unsigned to avoid integral conversion
    int const trace_mask = (TRACE_CPU |
				 TRACE_VMX |
				 TRACE_SMX |
				 TRACE_APIC |
				 TRACE_MEMORY |
				 
				 TRACE_TIMER |
				 
				 TRACE_ACPI |
				 TRACE_KEYB |
				 
				 0);

    // TODO: printf was commented out above
    /*
    do { register mword __esp asm ("esp"); if (__builtin_expect(((trace_mask & TRACE_MEMORY) == TRACE_MEMORY), false)) printf ("[%d] " "POOL: %#010lx-%#010lx O:%u" "\n", ((__esp - 1) & ~0xfff) == 0xcfffe000 ? ~0u [was Cpu::id] : ~0u, p_addr, p_addr + size, order); } while (0);
    */




    size -= order * sizeof *head;
    head = reinterpret_cast<Block *>(l_addr + size);


    size -= size / (page_size + sizeof *index) * sizeof *index;
    size &= page_mask;
    min_idx = l_addr / page_size - l_base / page_size;
    max_idx = (l_addr + size) / page_size - l_base / page_size;
    index = reinterpret_cast<Block *>(l_addr + size) - min_idx;

    unsigned i = 0U;
    for (/*unsigned i = 0*/; i < order; i++)
        head[i].next = head[i].prev = head + i;

    mword mw = f_addr;
    /*
    for (mword i = f_addr; i < l_addr + size; i += page_size)
        free (i);
    */
    for (; mw < l_addr + size; mw += page_size)
        free (mw);
}







void *
Buddy::alloc (unsigned ord, bool zero)
{
/*
    Lock_guard <Spinlock> guard (lock);
*/

    for (unsigned j = ord; j < order; j++) {

        if (head[j].next == head + j)
            continue;

        Block *block = head[j].next;
        block->prev->next = block->next;
        block->next->prev = block->prev;
        block->ord = ord;
	// Used made global for the semantics compiler
        block->tag = /*Block::*/Used;

        while (j-- != ord) {
            Block *buddy = block + (1ul << j);
            buddy->prev = buddy->next = head + j;
            buddy->ord = j;
	    // Free made global for the semantics compiler
            buddy->tag = /*Block::*/Free;
            buddy->tag = Free;
            head[j].next = head[j].prev = buddy;
        }

        mword l_addr = index_to_page (block_to_index (block));


	// TODO: panic was commented out above
	/*
        do { if (__builtin_expect((!((page_to_frame (l_addr) & ((1ul << (block->ord + page_ord)) - 1)) == 0)), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "(page_to_frame (l_addr) & ((1ul << (block->ord + page_ord)) - 1)) == 0", "buddy.cpp", 101); } while (0);
	*/

        if (zero)
	  // integral conversion not supported by the semantics compiler yet
	  memset (reinterpret_cast<void *>(l_addr), 0, 1/*ul*/ << (block->ord + page_ord));

        return reinterpret_cast<void *>(l_addr);
    }

    // TODO: panic was commented out above
    /*
    panic ("Out of memory");
    */
}





void
Buddy::free (mword l_addr)
{
    signed long idx = page_to_index (l_addr);


    // TODO: panic was commented out above
    /*
    do { if (__builtin_expect((!(idx >= min_idx && idx < max_idx)), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "idx >= min_idx && idx < max_idx", "buddy.cpp", 122); } while (0);
    */

    Block *block = index_to_block (idx);


    // TODO: panic was commented out above
    /*
    do { if (__builtin_expect((!(block->tag == Block::Used)), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "block->tag == Block::Used", "buddy.cpp", 127); } while (0);
    */

    // TODO: panic was commented out above
    /*
    do { if (__builtin_expect((!((page_to_frame (l_addr) & ((1ul << (block->ord + page_ord)) - 1)) == 0)), false)) panic ("Assertion \"%s\" failed at %s:%d\n", "(page_to_frame (l_addr) & ((1ul << (block->ord + page_ord)) - 1)) == 0", "buddy.cpp", 130); } while (0);
    */

/*
    Lock_guard <Spinlock> guard (lock);
*/

    // uninitialized variables are not supported
    /*
    unsigned ord;
    */
    unsigned ord = 0U;
    for (ord = block->ord; ord < order - 1; ord++) {


        signed long block_idx = block_to_index (block);
        signed long buddy_idx = block_idx ^ (1ul << ord);


        if (buddy_idx < min_idx || buddy_idx >= max_idx)
            break;

        Block *buddy = index_to_block (buddy_idx);


	// Used made global for the semantics compiler
        if (buddy->tag == /*Block::*/Used || buddy->ord != ord)
            break;


        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;


        if (buddy < block)
            block = buddy;
    }

    block->ord = ord;
    // Free made global for the semantics compiler
    block->tag = /*Block::*/Free;

    Block *h = head + ord;
    block->prev = h;
    block->next = h->next;
    block->next->prev = h->next = block;
}
