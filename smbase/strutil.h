// strutil.h
// various string utilities built upon the 'str' module
// Scott McPeak, July 2000

#ifndef __STRUTIL_H
#define __STRUTIL_H

#include "str.h"      // string

// direct string replacement, replacing instances of oldstr with newstr
// (newstr may be "")
string replace(char const *src, char const *oldstr, char const *newstr);


// remove any whitespace at the beginning or end of the string
string trimWhitespace(char const *str);


// encode a block of bytes as a string with C backslash escape
// sequences (but without the opening or closing quotes)
string encodeWithEscapes(char const *src, int len);

// safe when the text has no NUL characters; adds the quotes too
string quoted(char const *src);


// decode an escaped string; throw xFormat if there is a problem
// with the escape syntax; if 'delim' is specified, it will also
// make sure there are no unescaped instances of that
void decodeEscapes(string &dest, int &destLen, char const *src,
                   char delim = 0);

// given a string with quotes and escapes, yield just the string;
// works if there are no escaped NULs
string parseQuotedString(char const *text);



#endif // __STRUTIL_H
