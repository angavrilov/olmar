// unixutil.h
// some utilities on top of unix functions

#ifndef UNIXUTIL_H
#define UNIXUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// write entire contents of buffer to 'fd', returning 0 on failure
int writeAll(int fd, void const *buf, int len);
                                           
// read(2) some data into a buffer of 'len' bytes; null-terminate
// those bytes, and strip any trailing newline; return 0 on failure
int readString(int fd, char *str, int len);


#ifdef __cplusplus
}
#endif

#endif // UNIXUTIL_H
