// strutil.h
// various string utilities built upon the 'str' module
// Scott McPeak, July 2000

#ifndef __STRUTIL_H
#define __STRUTIL_H

#include "str.h"      // string

// direct string replacement, replacing instances of oldstr with newstr
// (newstr may be "")
string replace(char const *src, char const *oldstr, char const *newstr);

// works like unix "tr": the source string is translated character-by-character,
// with occurrances of 'srcchars' replaced by corresponding characters from
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
string basename(char const *src);

// given a directory name like "a/b/c", return "a/b"; if 'src' contains
// no slashes at all, return "."
string dirname(char const *src);


// return 'prefix', pluralized if n!=1
string plural(int n, char const *prefix);


#endif // __STRUTIL_H
