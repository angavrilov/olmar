// rdtsc.h
// interface to rdtsc.c

#ifdef __cplusplus
extern "C" {
#endif


void rdtsc(unsigned *lowp, unsigned *highp);


#ifdef __GNUC__
unsigned long long rdtsc_ll();
#endif


#ifdef __cplusplus
}
#endif

