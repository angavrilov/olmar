// strutil.h            see license.txt for copyright and terms of use
// various string utilities built upon the 'str' module
// Scott McPeak, July 2000

#ifndef STRUTIL_H
#define STRUTIL_H

#include "str.h"      // string
#include <stdio.h>    // FILE

// direct string replacement, replacing instances of oldstr with newstr
// (newstr may be "")
string replace(char const *src, char const *oldstr, char const *newstr);

// works like unix "tr": the source string is translated character-by-character,
// with occurrences of 'srcchars' replaced by corresponding characters from
// 'destchars'; further, either set may use the "X-Y" notation to denote a
// range of characters from X to Y
string translate(char const *src, char const *srcchars, char const *destchars);

// a simple example of using translate; it was originally inline, but a bug
// in egcs made me move it out of line
string stringToupper(char const *src);
//  { return translate(src, "a-z", "A-Z"); }


// remove any whitespace at the beginning or end of the string
string trimWhitespace(char const *str);


// encode a block of bytes as a string with C backslash escape
// sequences (but without the opening or closing quotes)
string encodeWithEscapes(char const *src, int len);

// safe when the text has no NUL characters
string encodeWithEscapes(char const *src);

// adds the quotes too
string quoted(char const *src);


// decode an escaped string; throw xFormat if there is a problem
// with the escape syntax; if 'delim' is specified, it will also
// make sure there are no unescaped instances of that
void decodeEscapes(string &dest, int &destLen, char const *src,
                   char delim = 0, bool allowNewlines=false);

// given a string with quotes and escapes, yield just the string;
// works if there are no escaped NULs
string parseQuotedString(char const *text);


// this probably belongs in a dedicated module for time/date stuff..
// returns asctime(localtime(time))
string localTimeString();


// given a directory name like "a/b/c", return "c"
// renamed from 'basename' because of conflict with something in string.h
string sm_basename(char const *src);

// given a directory name like "a/b/c", return "a/b"; if 'src' contains
// no slashes at all, return "."
string dirname(char const *src);


// return 'prefix', pluralized if n!=1; for example
//   plural(1, "egg") yields "egg", but
//   plural(2, "egg") yields "eggs";
// it knows about a few irregular pluralizations (see the source),
// and the expectation is I'll add more irregularities as I need them
string plural(int n, char const *prefix);

// same as 'plural', but with the stringized version of the number:
//   pluraln(1, "egg") yields "1 egg", and
//   pluraln(2, "eggs") yields "2 eggs"
string pluraln(int n, char const *prefix);


// Sometimes it's useful to store a string value in a static buffer;
// most often this is so 'gdb' can see the result.  This function just
// copies its input into a static buffer (of unspecified length, but
// it checks bounds internally), and returns a pointer to that buffer.
char *copyToStaticBuffer(char const *src);


// true if the first part of 'str' matches 'prefix'
bool prefixEquals(char const *str, char const *prefix);

// and similar for last part
bool suffixEquals(char const *str, char const *suffix);


// read/write strings <-> files
void writeStringToFile(char const *str, char const *fname);
string readStringFromFile(char const *fname);


// read the next line from a FILE* (e.g. an AutoFILE); the
// newline is returned if it is present (you can use 'chomp'
// to remove it); returns false (and "") on EOF
bool readLine(string &dest, FILE *fp);


// like perl 'chomp': remove a final newline if there is one
string chomp(char const *src);


#endif // STRUTIL_H
