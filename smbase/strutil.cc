// strutil.cc
// code for strutil.h

#include "strutil.h"     // this module
#include "exc.h"         // xformat

#include <ctype.h>       // isspace
#include <string.h>      // strstr
#include <stdio.h>       // sprintf
#include <stdlib.h>      // strtoul


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


// table of escape codes
static struct Escape {
  char actual;      // actual character in string
  char escape;      // char that follows backslash to produce 'actual'
} const escapes[] = {
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


string encodeWithEscapes(char const *p, int len)
{
  stringBuilder sb;

  for (; len>0; len--, p++) {
    // look for an escape code
    unsigned i;
    for (i=0; i<TABLESIZE(escapes); i++) {
      if (escapes[i].actual == *p) {
        sb << '\\' << escapes[i].escape;
        break;
      }
    }
    if (i<TABLESIZE(escapes)) {
      continue;   // found it and printed it
    }

    // try itself
    if (isprint(*p)) {
      sb << *p;
      continue;
    }

    // use the most general notation
    char tmp[5];
    sprintf(tmp, "\\x%02X", (unsigned char)(*p));
    sb << tmp;
  }
  
  return sb;
}


void decodeEscapes(string &dest, int &destLen, char const *src,
                   char delim)
{
  // place to collect the string characters
  stringBuilder sb;
  destLen = 0;

  while (*src != '\0') {
    if (*src == '\n') {
      xformat("unescaped newline (unterminated string)");
    }
    if (*src == delim) {
      xformat(stringc << "unescaped delimiter (" << delim << ")");
    }

    if (*src != '\\') {
      // easy case
      sb << *src;
      destLen++;
      src++;
      continue;
    }          
          
    // advance past backslash
    src++;

    // see if it's a simple one-char backslash code
    int i;
    for (i=0; i<TABLESIZE(escapes); i++) {
      if (escapes[i].escape == *src) {
        sb << escapes[i].actual;
        destLen++;
        break;
      }
    }
    if (i < TABLESIZE(escapes)) {
      continue;
    }
    
    if (*src == '\0') {
      xformat("backslash at end of string");
    }

    if (*src == '\n') {
      // escaped newline; advance to first non-whitespace
      src++;
      while (*src==' ' || *src=='\t') {
        src++;
      }
      continue;
    }

    if (*src == 'x' || isdigit(*src)) {
      // hexadecimal or octal char (it's unclear to me exactly how to
      // parse these since it's supposedly legal to have e.g. "\x1234"
      // mean a one-char string.. whatever)
      bool hex = (*src == 'x');
      if (hex) {
        src++;

        // strtoul is willing to skip leading whitespace
        if (isspace(*src)) {
          xformat("whitespace following hex (\\x) escape");
        }
      }

      char const *endptr;
      unsigned long val = strtoul(src, (char**)&endptr, hex? 16 : 8);
      if (src == endptr) {
        // this can't happen with the octal escapes because
        // there is always at least one valid digit
        xformat("invalid hex (\\x) escape");
      }

      sb << (unsigned char)val;    // possible truncation..
      destLen++;
      src = endptr;
      continue;
    }

    // everything not explicitly covered will be considered
    // an error (for now), even though the C++ spec says
    // it should be treated as if the backslash were not there
    xformat("unknown escape code");
  }

  // copy to 'dest'
  dest.setlength(destLen);
  memcpy(dest.pchar(), sb.pchar(), destLen+1);   // copy NUL too
}
