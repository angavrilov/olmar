// mysig.cc
// code for mysig.h

#include "mysig.h"      // this module

#include <string.h>     // strsignal
#include <unistd.h>     // sleep
#include <stdio.h>      // printf

// needed on Solaris; is __sun__ a good way to detect that?
#ifdef __sun__
  #include <siginfo.h>
#endif

#ifndef __CYGWIN__      // everything here is for *not* cygwin

void setHandler(int signum, SignalHandler handler)
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));       // clear any "extra" fields
  sa.sa_handler = handler;          // handler function
  sigemptyset(&sa.sa_mask);         // don't block other signals
  sa.sa_flags = SA_RESTART;         // automatically restart syscalls

  // install the handler
  if (0 > sigaction(signum, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }
}


// simple handler that just prints and re-raises
void printHandler(int signum)
{
  fprintf(stderr, "printHandler: I caught signal %d\n", signum);
  psignal(signum, "psignal message");

  //fprintf(stderr, "I'm just going to wait a while...\n");
  //sleep(60);

  // block the signal -- doesn't work for internally-generated
  // signals (i.e. raise)
  //sigset_t mask;
  //sigemptyset(&mask);
  //sigaddset(&mask, SIGINT);

  // reset the signal handler to its default handler
  setHandler(signum, SIG_DFL);

  // re-raise, which doesn't come back to this handler because
  // the signal is blocked while we're in here -- wrong; it
  // is blocked from external signals, but not from signals
  // generated internally...
  fprintf(stderr, "re-raising...\n");
  raise(signum);
}


jmp_buf sane_state;

// handler to do a longjmp
void jmpHandler(int signum)
{
  //fprintf(stderr, "jmpHandler: I caught signal %d\n", signum);
  psignal(signum, "jmpHandler: caught signal");

  // reset the signal handler to its default handler
  setHandler(signum, SIG_DFL);

  // do it
  //fprintf(stderr, "calling longjmp...\n");
  longjmp(sane_state, 1);
}


// ------------------ test code ------------------
#ifdef TEST_MYSIG

int main()
{
  if (setjmp(sane_state) == 0) {   // normal flow
    setHandler(SIGINT, printHandler);
    setHandler(SIGTERM, printHandler);
    setHandler(SIGUSR1, jmpHandler);
    setHandler(SIGSEGV, jmpHandler);

    //printf("I'm pid %d waiting to be killed...\n", getpid());
    //sleep(10);
    *((int*)0) = 0;    // segfault!
    printf("finished waiting\n");
    return 0;
  }

  else {         // from longjmp
    printf("came back from a longjmp\n");
    return 0;
  }
}

#endif // TEST_MYSIG


#else   // cygwin -- just stubs so it compiles
void setHandler(int, SignalHandler) {}
void printHandler(int) {}
jmp_buf sane_state;
void jmpHandler(int) {}
#endif
