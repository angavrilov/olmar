// strutil.cc
// code for strutil.h

#include "strutil.h"     // this module

#include <ctype.h>       // isspace
#include <string.h>      // strstr
#include <stdio.h>       // sprintf


// replace all instances of oldstr in src with newstr, return result
string replace(char const *src, char const *oldstr, char const *newstr)
{
  stringBuilder ret("");

  while (*src) {
    char const *next = strstr(src, oldstr);
    if (!next) {
      ret &= string(src);
      break;
    }

    // make a substring out of the characters between src and next
    string upto(src, next-src);

    // add it to running string
    ret &= upto;

    // add the replace-with string
    ret &= string(newstr);

    // move src to beyond replaced substring
    src += (next-src) + strlen(oldstr);
  }

  return ret;
}


string trimWhitespace(char const *str)
{
  // trim leading whitespace
  while (isspace(*str)) {
    str++;
  }

  // trim trailing whitespace
  char const *end = str + strlen(str);
  while (end > str &&
         isspace(end[-1])) {
    end--;
  }

  // return it
  return string(str, end-str);
}


string encodeWithEscapes(char const *p, int len)
{
  stringBuilder sb;

  // table of escape codes
  static struct {
    char actual;      // actual character in string
    char escape;      // char that follows backslash to produce 'actual'
  } const arr[] = {
    { '\0', '0' },  // nul
    { '\a', 'a' },  // bell
    { '\b', 'b' },  // backspace
    { '\f', 'f' },  // form feed
    { '\n', 'n' },  // newline
    { '\r', 'r' },  // carriage return
    { '\t', 't' },  // tab
    { '\v', 'v' },  // vertical tab
    { '\\', '\\'},  // backslash
    { '"',  '"' },  // double-quote
  };

  for (; len>0; len--, p++) {
    // look for an escape code
    unsigned i;
    for (i=0; i<TABLESIZE(arr); i++) {
      if (arr[i].actual == *p) {
        sb << '\\' << arr[i].escape;
        break;
      }
    }
    if (i<TABLESIZE(arr)) {
      continue;   // found it and printed it
    }

    // try itself
    if (isprint(*p)) {
      sb << *p;
      continue;
    }

    // use the most general notation
    char tmp[5];
    sprintf(tmp, "\\x%02X", *p);
    sb << tmp;
  }
  
  return sb;
}
