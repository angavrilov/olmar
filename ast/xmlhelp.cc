// xmlhelp.cc
// support routines for XML reading/writing

#include "xmlhelp.h"            // this module
#include <stdlib.h>             // atof, atol
#include <ctype.h>              // isprint, isdigit, isxdigit
#include <stdio.h>              // sprintf
#include "strtokp.h"            // StrtokParse
#include "exc.h"                // xformat

string toXml_bool(bool b) {
  if (b) return "true";
  else return "false";
}

void fromXml_bool(bool &b, rostring str) {
  b = streq(str, "true");
}


string toXml_int(int i) {
  return stringc << i;
}

void fromXml_int(int &i, rostring str) {
  long i0 = strtol(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_long(long i) {
  return stringc << i;
}

void fromXml_long(long &i, rostring str) {
  long i0 = strtol(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_unsigned_int(unsigned int i) {
  return stringc << i;
}

void fromXml_unsigned_int(unsigned int &i, rostring str) {
  unsigned long i0 = strtoul(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_unsigned_long(unsigned long i) {
  return stringc << i;
}

void fromXml_unsigned_long(unsigned long &i, rostring str) {
  unsigned long i0 = strtoul(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_double(double x) {
  return stringc << x;
}

void fromXml_double(double &x, rostring str) {
  x = atof(str.c_str());
}


string toXml_SourceLoc(SourceLoc loc) {
  // NOTE: the nohashline here is very important; never change it
  return sourceLocManager->getString_nohashline(loc);
}

void fromXml_SourceLoc(SourceLoc &loc, rostring str) {
  // the file format is filename:line:column
  StrtokParse tok(str, ":");
  if ((int)tok != 3) {
    // FIX: this is a parsing error but I don't want to throw an
    // exception out of this library function
    loc = SL_UNKNOWN;
    return;
  }
  char const *file = tok[0];
  if (streq(file, "<noloc>")) {
    loc = SL_UNKNOWN;
    return;
  }
  if (streq(file, "<init>")) {
    loc = SL_INIT;
    return;
  }
  int line = atoi(tok[1]);
  int col  = atoi(tok[2]);
  loc = sourceLocManager->encodeLineCol(file, line, col);
}


//  void xmlPrintPointer(ostream &os, char const *label, void const *p)
//  {
//    if (!p) {
//      // sm: previously, the code for this function just inserted 'p'
//      // as a 'void const *', but that is nonportable, as gcc-3 inserts
//      // "0" while gcc-2 emits "(nil)"
//      os << label << "0";
//    }
//    else {
//      // sm: I question whether this is portable, but it may not matter
//      // since null pointers are the only ones that are treated
//      // specially (as far as I can tell)
//      os << label << p;
//    }
//  }

string xmlPrintPointer(char const *label, void const *p)
{
  stringBuilder sb;
  if (!p) {
    // sm: previously, the code for this function just inserted 'p'
    // as a 'void const *', but that is nonportable, as gcc-3 inserts
    // "0" while gcc-2 emits "(nil)"
    sb << label << "0";
  }
  else {
    // sm: I question whether this is portable, but it may not matter
    // since null pointers are the only ones that are treated
    // specially (as far as I can tell)
    sb << label;
    sb << stringBuilder::Hex(reinterpret_cast<long unsigned>(p));
  }
  return sb;
}


// named escape codes
#define lt_CODE "lt;"
#define gt_CODE "gt;"
#define amp_CODE "amp;"
#define quot_CODE "quot;"
int const lt_codelen   = strlen(lt_CODE);
int const gt_codelen   = strlen(gt_CODE);
int const amp_codelen  = strlen(amp_CODE);
int const quot_codelen = strlen(quot_CODE);

string xmlAttrQuote(rostring src) {
  return stringc << "\""
                 << xmlAttrEncode(src)
                 << "\"";
}

string xmlAttrEncode(rostring src) {
  return xmlAttrEncode(toCStr(src), strlen(src));
}

string xmlAttrEncode(char const *p, int len) {
  stringBuilder sb;

  for(int i=0; i<len; ++i) {
    unsigned char c = p[i];

    // escape those special to xml attributes;
    // http://www.w3.org/TR/2004/REC-xml-20040204/#NT-AttValue
    switch (c) {
    default: break;             // try below
    case '<': sb << "&" lt_CODE;   continue;
    case '>': sb << "&" gt_CODE;   continue; // this one not strictly required here
    case '&': sb << "&" amp_CODE;  continue;
    case '"': sb << "&" quot_CODE; continue;
    }

    // try itself
    if (isprint(c)) {
      sb << c;
      continue;
    }

    // use the most general notation
    char tmp[7];
    // dsw: the sillyness of XML knows no bounds: it is actually more
    // efficient to use the decimal encoding since sometimes you only
    // need 4 or 5 characters, whereas with hex you are guaranteed to
    // need 6, however the uniformity makes it easier to decode hex.
    // Why not make the shorter encoding also the default so that it
    // really is shorter in the big picture?
    sprintf(tmp, "&#x%02X;", c);
    sb << tmp;
  }

  return sb;
}

// dsw: based on smbase/strutil.cc/parseQuotedString();
string xmlAttrDeQuote(rostring text) {
  if (!( text[0] == '"' &&
         text[strlen(text)-1] == '"' )) {
    xformat(stringc << "quoted string is missing quotes: " << text);
  }

  // strip the quotes
  string noQuotes = substring(toCStr(text)+1, strlen(text)-2);

  // decode escapes
  ArrayStack<char> buf;
  xmlAttrDecode(buf, noQuotes, '"');
  buf.push(0 /*NUL*/);

  // return string contents up to first NUL, which isn't necessarily
  // the same as the one just pushed; by invoking this function, the
  // caller is accepting responsibility for this condition
  return string(buf.getArray());
}

void xmlAttrDecode(ArrayStack<char> &dest, rostring origSrc, char delim)
{
  char const *src = toCStr(origSrc);
  xmlAttrDecode(dest, src, delim);
}

void xmlAttrDecode(ArrayStack<char> &dest, char const *src, char delim)
{
  while (*src != '\0') {
    // check for newlines
    if (*src == '\n') {
      xformat("unescaped newline (unterminated string)");
    }

    // check for the delimiter
    if (*src == delim) {
      xformat(stringc << "unescaped delimiter (" << delim << ")");
    }

    // check for normal characters
    if (*src != '&') {
      // normal character
      dest.push(*src);
      src++;
      continue;
    }
    src++;                      // advance past amperstand

    // checked for named escape codes
#define DO_ESCAPE(NAME, CHAR) \
    if (strncmp(NAME ##_CODE, src, NAME ##_codelen) == 0) { \
      dest.push(CHAR); \
      src += NAME ##_codelen; \
      continue; \
    }
    DO_ESCAPE(lt,   '<');
    DO_ESCAPE(gt,   '>');
    DO_ESCAPE(amp,  '&');
    DO_ESCAPE(quot, '"');
#undef DO_ESCAPE

    // check for numerical escapes
    if (*src != '#') {
      xformat(stringc << "use of an unimplemented or illegal amperstand escape (" << *src << ")");
    }
    ++src;

    // process decimal and hex escapes: decimal '&#DDD;' where D is a
    // decimal digit or hexadecimal '&#xHH;' where H is a hex digit.
    if (!(*src == 'x' || isdigit(*src))) {
      xformat(stringc << "illegal charcter after '&#' (" << *src << ")");
    }

    // are we doing hex or decimal processing?
    //
    // http://www.w3.org/TR/2004/REC-xml-20040204/#NT-CharRef "If the
    // character reference begins with "&#x", the digits and letters
    // up to the terminating ; provide a hexadecimal representation of
    // the character's code point in ISO/IEC 10646. If it begins just
    // with "&#", the digits up to the terminating ; provide a decimal
    // representation of the character's code point."
    bool hex = (*src == 'x');
    if (hex) {
      src++;

      // strtoul is willing to skip leading whitespace, so I need
      // to catch it myself
      if (!isxdigit(*src)) {
        // dsw: NOTE: in the non-hex case, the leading digit has
        // already been seen
        xformat("non-hex digit following '&#x' escape");
      }
      xassert(isxdigit(*src));
    } else {
      xassert(isdigit(*src));
    }

    // parse the digit
    char const *endptr;
    unsigned long val = strtoul(src, (char**)&endptr, hex? 16 : 10);
    if (src == endptr) {
      // this can't happen with the octal escapes because
      // there is always at least one valid digit
      xformat("invalid '&#' escape");
    }

    // keep it
    dest.push((char)(unsigned char)val);    // possible truncation..
    src = endptr;
  }
}
