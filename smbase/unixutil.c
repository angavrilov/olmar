// unixutil.c
// code for unixutil.h

#include "unixutil.h"   // this module

#include <unistd.h>     // write
#include <assert.h>     // assert

int writeAll(int fd, void *buf, int len)
{
  int written = 0;
  while (written > 0) {
    int result = write(fd, buf+written, len-written);
    if (result < 0) {
      return 0;    // failure
    }
    written += result;
  }
  assert(written == len);
  return 1;        // success
}


int readString(int fd, char *str, int len)
{
  int count = read(fd, str, len-1);
  if (count < 0) {
    return 0;      // failure
  }
  str[len]=0;
  
  // remove trailing newline if any
  if (len>0 && str[len-1] == '\n') {
    str[len-1] = 0;
  }

  return 1;
}
