// unixutil.c            see license.txt for copyright and terms of use
// code for unixutil.h

#include "unixutil.h"   // this module

#include <unistd.h>     // write
#include <assert.h>     // assert
#include <sys/time.h>   // struct timeval
#include <sys/types.h>  // select
#include <unistd.h>     // select
#include <stdio.h>      // perror


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


int canRead(int fd)
{
  fd_set set;
  struct timeval tv;
  int res;

  // check only 'fd'
  FD_ZERO(&set);
  FD_SET(fd, &set);

  // do not block at all
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  res = select(fd+1, &set, NULL, NULL, &tv);
  if (res == -1) {
    perror("select");     // not ideal...
    return 0;
  }
  
  return res;             // 0 or 1
}


// EOF
