// strutil.cc            see license.txt for copyright and terms of use
// code for strutil.h

#include "strutil.h"     // this module
#include "exc.h"         // xformat
#include "autofile.h"    // AutoFILE

#include <ctype.h>       // isspace
#include <string.h>      // strstr, memcmp
#include <stdio.h>       // sprintf
#include <stdlib.h>      // strtoul
#include <time.h>        // time, asctime, localtime


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


string expandRanges(char const *chars)
{
  stringBuilder ret;
  
  while (*chars) {
    if (chars[1] == '-' && chars[2] != 0) {
      // range specification
      if (chars[0] > chars[2]) {
        xformat("range specification with wrong collation order");
      }

      for (char c = chars[0]; c <= chars[2]; c++) {
        ret << c;
      }
      chars += 3;
    }
    else {
      // simple character specification
      ret << chars[0];
      chars++;
    }
  }

  return ret;
}


string translate(char const *src, char const *srcchars, char const *destchars)
{
  // first, expand range notation in the specification sequences
  string srcSpec = expandRanges(srcchars);
  string destSpec = expandRanges(destchars);

  // build a translation map
  char map[256];
  int i;
  for (i=0; i<256; i++) {
    map[i] = i;
  }

  // excess characters from either string are ignored ("SysV" behavior)
  for (i=0; i < srcSpec.length() && i < destSpec.length(); i++) {
    map[(unsigned char)( srcSpec[i] )] = destSpec[i];
  }

  // run through 'src', applying 'map'
  string ret(strlen(src));
  char *dest = ret.pchar();
  while (*src) {
    *dest = map[(unsigned char)*src];
    dest++;
    src++;
  }
  *dest = 0;    // final nul terminator

  return ret;
}


// why is this necessary?
string stringToupper(char const *src)
  { return translate(src, "a-z", "A-Z"); }


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
  { '\'', '\''},  // single-quote
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


string encodeWithEscapes(char const *p)
{
  return encodeWithEscapes(p, strlen(p));
}


string quoted(char const *src)
{
  return stringc << "\""
                 << encodeWithEscapes(src, strlen(src))
                 << "\"";
}


void decodeEscapes(string &dest, int &destLen, char const *src,
                   char delim, bool allowNewlines)
{
  // place to collect the string characters
  stringBuilder sb;
  destLen = 0;

  while (*src != '\0') {
    if (*src == '\n' && !allowNewlines) {
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

    // see if it's a simple one-char backslash code;
    // start at 1 so we do *not* use the '\0' code since
    // that's actually a special case of \0123', and
    // interferes with the latter
    int i;
    for (i=1; i<TABLESIZE(escapes); i++) {
      if (escapes[i].escape == *src) {
        sb << escapes[i].actual;
        destLen++;
        src++;
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
    //
    // 7/29/04: now making backslash the identity transformation in
    // this case
    //
    // copy character as if it had not been backslashed
    sb << *src;
    destLen++;
    src++;
  }

  // copy to 'dest'
  dest.setlength(destLen);       // this sets the NUL
  if (destLen > 0) {
    memcpy(dest.pchar(), sb.pchar(), destLen);
  }
}


string parseQuotedString(char const *text)
{
  if (!( text[0] == '"' &&
         text[strlen(text)-1] == '"' )) {
    xformat(stringc << "quoted string is missing quotes: " << text);
  }

  // strip the quotes
  string noQuotes = string(text+1, strlen(text)-2);
  
  // decode escapes
  string ret;
  int dummyLen;
  decodeEscapes(ret, dummyLen, noQuotes, '"');
  return ret;
}


string localTimeString()
{
  time_t t = time(NULL);
  char const *p = asctime(localtime(&t));
  return string(p, strlen(p) - 1);     // strip final newline
}


string sm_basename(char const *src)
{
  char const *sl = strrchr(src, '/');   // locate last slash
  if (sl && sl[1] == 0) {
    // there is a slash, but it is the last character; ignore it
    // (this behavior is what /bin/basename does)
    return sm_basename(string(src, strlen(src)-1));
  }

  if (sl) {
    return string(sl+1);     // everything after the slash
  }
  else {
    return string(src);      // entire string if no slashes
  }
}

string dirname(char const *src)
{
  char const *sl = strrchr(src, '/');   // locate last slash
  if (sl == src) {
    // last slash is leading slash
    return string("/");
  }

  if (sl && sl[1] == 0) {
    // there is a slash, but it is the last character; ignore it
    // (this behavior is what /bin/dirname does)
    return dirname(string(src, strlen(src)-1));
  }

  if (sl) {
    return string(src, sl-src);     // everything before slash
  }
  else {
    return string(".");
  }
}


// I will expand this definition as I go to get more knowledge about
// English irregularities as I need it
string plural(int n, char const *prefix)
{
  if (n==1) {
    return string(prefix);
  }

  if (0==strcmp(prefix, "was")) {
    return string("were");
  }
  if (prefix[strlen(prefix)-1] == 'y') {
    return stringc << string(prefix, strlen(prefix)-1) << "ies";
  }
  else {
    return stringc << prefix << "s";
  }
}

string pluraln(int n, char const *prefix)
{
  return stringc << n << " " << plural(n, prefix);
}


char *copyToStaticBuffer(char const *s)
{           
  enum { SZ=200 };
  static char buf[SZ+1];

  int len = strlen(s);
  if (len > SZ) len=SZ;
  memcpy(buf, s, len);
  buf[len] = 0;

  return buf;
}


bool prefixEquals(char const *str, char const *prefix)
{
  int slen = strlen(str);
  int plen = strlen(prefix);
  return slen >= plen &&
         0==memcmp(str, prefix, plen);
}

bool suffixEquals(char const *str, char const *suffix)
{
  int slen = strlen(str);
  int ulen = strlen(suffix);    // sUffix
  return slen >= ulen &&
         0==memcmp(str+slen-ulen, suffix, ulen);
}


void writeStringToFile(char const *str, char const *fname)
{
  AutoFILE fp(fname, "w");

  if (fputs(str, fp) < 0) {
    xbase("fputs: EOF");
  }
}


string readStringFromFile(char const *fname)
{
  AutoFILE fp(fname, "r");

  stringBuilder sb;

  char buf[4096];
  for (;;) {
    int len = fread(buf, 1, 4096, fp);
    if (len < 0) {
      xbase("fread failed");
    }
    if (len == 0) {
      break;
    }

    sb.append(buf, len);
  }

  return sb;
}


bool readLine(string &dest, FILE *fp)
{
  char buf[80];

  if (!fgets(buf, 80, fp)) {
    return false;
  }

  if (buf[strlen(buf)-1] == '\n') {
    // read a newline, we got the whole line
    dest = buf;
    return true;
  }

  // only got part of the string; need to iteratively construct
  stringBuilder sb;
  while (buf[strlen(buf)-1] != '\n') {
    sb << buf;
    if (!fgets(buf, 80, fp)) {
      // found eof after partial; return partial *without* eof
      // indication, since we did in fact read something
      break;
    }
  }

  dest = sb;
  return true;
}


string chomp(char const *src)
{
  if (src && src[strlen(src)-1] == '\n') {
    return string(src, strlen(src)-1);
  }
  else {
    return string(src);
  }
}


// ----------------------- test code -----------------------------
#ifdef TEST_STRUTIL

#include "test.h"      // USUAL_MAIN
#include <stdio.h>     // printf

void expRangeVector(char const *in, char const *out)
{
  printf("expRangeVector(%s, %s)\n", in, out);
  string result = expandRanges(in);
  xassert(result.equals(out));
}

void trVector(char const *in, char const *srcSpec, char const *destSpec, char const *out)
{
  printf("trVector(%s, %s, %s, %s)\n", in, srcSpec, destSpec, out);
  string result = translate(in, srcSpec, destSpec);
  xassert(result.equals(out));
}

void decodeVector(char const *in, char const *out, int outLen)
{
  printf("decodeVector: \"%s\"\n", in);
  string dest;
  int destLen;
  decodeEscapes(dest, destLen, in, '\0' /*delim, ignored*/, false /*allowNewlines*/);
  xassert(destLen == outLen);
  xassert(0==memcmp(out, dest.pcharc(), destLen));
}

void basenameVector(char const *in, char const *out)
{
  printf("basenameVector(%s, %s)\n", in, out);
  string result = sm_basename(in);
  xassert(result.equals(out));
}

void dirnameVector(char const *in, char const *out)
{
  printf("dirnameVector(%s, %s)\n", in, out);
  string result = dirname(in);
  xassert(result.equals(out));
}

void pluralVector(int n, char const *in, char const *out)
{
  printf("pluralVector(%d, %s, %s)\n", n, in, out);
  string result = plural(n, in);
  xassert(result.equals(out));
}


void entry()
{
  expRangeVector("abcd", "abcd");
  expRangeVector("a", "a");
  expRangeVector("a-k", "abcdefghijk");
  expRangeVector("0-9E-Qz", "0123456789EFGHIJKLMNOPQz");

  trVector("foo", "a-z", "A-Z", "FOO");
  trVector("foo BaR", "a-z", "A-Z", "FOO BAR");
  trVector("foo BaR", "m-z", "M-Z", "fOO BaR");

  decodeVector("\\r\\n", "\r\n", 2);
  decodeVector("abc\\0def", "abc\0def", 7);
  decodeVector("\\033", "\033", 1);
  decodeVector("\\x33", "\x33", 1);
  decodeVector("\\?", "?", 1);

  basenameVector("a/b/c", "c");
  basenameVector("abc", "abc");
  basenameVector("/", "");
  basenameVector("a/b/c/", "c");

  dirnameVector("a/b/c", "a/b");
  dirnameVector("a/b/c/", "a/b");
  dirnameVector("/a", "/");
  dirnameVector("abc", ".");
  dirnameVector("/", "/");

  pluralVector(1, "path", "path");
  pluralVector(2, "path", "paths");
  pluralVector(1, "fly", "fly");
  pluralVector(2, "fly", "flies");
  pluralVector(2, "was", "were");
}


USUAL_MAIN

#endif // TEST_STRUTIL
