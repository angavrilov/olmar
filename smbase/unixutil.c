// unixutil.c
// code for unixutil.h

#include "unixutil.h"   // this module

#include <unistd.h>     // write
#include <assert.h>     // assert

int writeAll(int fd, void const *buf, int len)
{
  int written = 0;
  while (written < len) {
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
  str[count]=0;

  // remove trailing newlines (or NULs), if any
  while (count>0 && (str[count-1] == '\n' || str[count-1] == 0)) {
    count--;
    str[count] = 0;
  }

  return 1;
}
