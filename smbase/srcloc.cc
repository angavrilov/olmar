// srcloc.cc
// code for srcloc.h

#include "srcloc.h"     // this module
#include "exc.h"        // throw_XOpen
#include "array.h"      // ArrayStack
#include "syserr.h"     // xsyserror
#include "trace.h"      // traceProgress

#include <stdio.h>      // FILE, etc.


// this parameter controls the frequency of Markers in 
// the marker index; higher frequency makes the index
// faster but use more space
enum { MARKER_FREQ = 100 };    // 100 is about a 10% overhead


// ------------------------- File -----------------------
void addLineLength(ArrayStack<unsigned char> &lengths, int len)
{
  while (len >= 255) {
    // add a long-line marker, which represents 254 chars of input
    lengths.push(255);
    len -= 254;
  }

  // add the short count at the end
  lengths.push((unsigned char)len);
}


SourceLocManager::File::File(char const *n, SourceLoc aStartLoc)
  : name(n),
    startLoc(aStartLoc),     // assigned by SourceLocManager

    // valid marker/col for the first char in the file
    marker(0, 1, 0),
    markerCol(1)
{
  // the buffering that FILE does is going to be wasted, but
  // portability is probably more important than squeezing
  // more performance out of the i/o calls
  FILE *fp = fopen(name, "r");
  if (!fp) {
    throw_XOpen(name);
  }

  // These are growable versions of the indexes.  They will be
  // discarded after the file has been read.  They keep track of their
  // own 'next index' values.  I chose to use a growable array instead
  // of making two passes over the file to reduce i/o.
  ArrayStack<unsigned char> lineLengths;
  ArrayStack<Marker> index;

  // put a marker at the start for uniformity
  index.push(Marker(0, 1, 0));

  // how many lines to go before I insert the next marker
  int indexDelay = MARKER_FREQ;

  // where I am in the file
  int charOffset = 0;
  int lineNum = 1;
  int lineLen = 0;       // length of current line, so far

  // read the file, computing information about line lengths
  enum { BUFLEN=8192 };
  char buf[BUFLEN];
  for (;;) {
    // read a buffer of data
    int len = fread(buf, 1, BUFLEN, fp);
    if (len < 0) {
      xsyserror("fread", name);
    }
    if (len==0) {
      break;
    }

    // the code that follows can be seen as abstracting the data
    // contained in buf[start] through buf[end-1] and adding that
    // information to the summary variables above
    char const *start = buf;      // beginning of unaccounted-for chars
    char const *p = buf;          // scan pointer
    char const *end = buf+len;    // end of unaccounted-for chars

    // loop over the lines in 'buf'
    while (start < end) {
      // scan to the next newline
      while (p<end && *p!='\n') {
        p++;
      }
      if (p==end) {
        break;
      }
      xassert(*p == '\n');

      // account for [start,p)
      charOffset += p-start;
      lineLen += p-start;
      start = p;

      // account for the newline at '*p'
      addLineLength(lineLengths, lineLen);
      charOffset++;
      lineNum++;
      lineLen = 0;

      p++;
      start++;

      if (--indexDelay == 0) {
        // insert a marker to remember this location
        index.push(Marker(charOffset, lineNum, lineLengths.length()));
        indexDelay = MARKER_FREQ;
      }
    }

    // move [start,p) into 'lineLen'
    charOffset += p-start;
    lineLen += p-start;
    start = p;
    xassert(start == end);
  }

  // handle the last line; in the usual case, where a newline is
  // the last character, the final line will have 0 length, but
  // we encode that anyway since it helps the decode phase below
  addLineLength(lineLengths, lineLen);
  charOffset += lineLen;

  // move computed information into 'this'
  this->numChars = charOffset;
  this->numLines = lineNum-1;
  if (numLines == 0) {
    // a file with no newlines
    this->avgCharsPerLine = numChars;
  }
  else {
    this->avgCharsPerLine = numChars / numLines;
  }

  this->lineLengthsSize = lineLengths.length();
  this->lineLengths = new unsigned char[lineLengthsSize];
  memcpy(this->lineLengths, lineLengths.getArray(), 
         lineLengthsSize * sizeof(this->lineLengths[0]));

  this->indexSize = index.length();
  this->index = new Marker[indexSize];
  memcpy(this->index, index.getArray(), 
         indexSize * sizeof(this->index[0]));
}


SourceLocManager::File::~File()
{
  delete[] lineLengths;
}


void SourceLocManager::File::resetMarker()
{
  marker.charOffset = 0;
  marker.lineOffset = 1;
  marker.arrayOffset = 0;
  markerCol = 1;
}


// it's conceivable gcc is smart enough to recognize
// the induction variable, if I inline this..
inline void SourceLocManager::File::advanceMarker()
{
  int len = (int)lineLengths[marker.arrayOffset];
  if (len < 255) {
    // normal length line
    marker.charOffset += len+1;     // +1 for newline
    marker.lineOffset++;
    marker.arrayOffset++;
    markerCol = 1;
  }
  else {
    // fragment of a long line, representing 254 characters
    marker.charOffset += 254;
    marker.arrayOffset++;
    markerCol += 254;
  }
}


int SourceLocManager::File::lineToChar(int lineNum)
{
  if (lineNum == numLines+1) {
    // end of file location
    return numChars;
  }

  xassert(1 <= lineNum && lineNum <= numLines);

  // check to see if the marker is already close
  if (marker.lineOffset <= lineNum &&
                           lineNum < marker.lineOffset + MARKER_FREQ) {
    // use the marker as-is
  }
  else {
    // do a binary search on the index to find the marker whose
    // lineOffset is closest to 'lineNum' without going over
    int low = 0;              // lowest index under consideration
    int high = indexSize-1;   // highest index under consideration
    while (low < high) {
      // check the midpoint (round up to ensure progress when low+1 == high)
      int mid = (low+high+1)/2;
      if (index[mid].lineOffset > lineNum) {
        // too high
        high = mid-1;
      }
      else {
        // too low or just right
        low = mid;
      }
    }

    // copy this index marker into our primary marker
    marker = index[low];
    markerCol = 1;            // all index markers implicitly have column 1
  }

  xassert(marker.lineOffset <= lineNum);

  // move the marker down the array until it arrives at
  // the desired line
  while (marker.lineOffset < lineNum) {
    advanceMarker();
  }

  // make sure we never go beyond the end of the array
  xassert(marker.arrayOffset < lineLengthsSize);

  // if we didn't move the marker, we might not be in column 1
  return marker.charOffset - (markerCol-1);
}


void SourceLocManager::File::charToLineCol(int offset, int &line, int &col)
{
  if (offset == numChars) {
    // end of file location
    line = numLines+1;
    col = 1;
    return;
  }

  xassert(0 <= offset && offset < numChars);

  // check if the marker is close enough
  if (marker.charOffset <= offset &&
                           offset < marker.charOffset + MARKER_FREQ*avgCharsPerLine) {
    // use as-is
  }
  else {
    // binary search, like above
    int low = 0;
    int high = indexSize-1;
    while (low < high) {
      // check midpoint
      int mid = (low+high+1)/2;
      if (index[mid].charOffset > offset) {
        high = mid-1;
      }
      else {
        low = mid;
      }
    }

    // copy this marker
    marker = index[low];
    markerCol = 1;
  }

  xassert(marker.charOffset <= offset);

  // move the marker until it's within one spot of moving
  // beyond the offset
  while (marker.charOffset + lineLengths[marker.arrayOffset] < offset) {
    advanceMarker();
  }

  // make sure we never go beyond the end of the array
  xassert(marker.arrayOffset < lineLengthsSize);

  // read off line/col
  line = marker.lineOffset;
  col = markerCol + (offset - marker.charOffset);
}


// ----------------------- StaticLoc -------------------
SourceLocManager::StaticLoc::~StaticLoc()
{}


// ----------------------- SourceLocManager -------------------
SourceLocManager *sourceLocManager = NULL;


SourceLocManager::SourceLocManager()
  : files(),
    recent(NULL),
    statics(),
    nextLoc(toLoc(1)),
    nextStaticLoc(toLoc(0)),
    maxStaticLocs(100)
{
  if (!sourceLocManager) {
    sourceLocManager = this;
  }

  // slightly clever: treat SL_UNKNOWN as a static
  SourceLoc u = encodeStatic(StaticLoc("<noloc>", 0,1,1));
  xassert(u == SL_UNKNOWN);
  PRETEND_USED(u);     // silence warning in case xasserts are turned off
}

SourceLocManager::~SourceLocManager()
{
  if (sourceLocManager == this) {
    sourceLocManager = NULL;
  }
}


// find it, or return NULL
SourceLocManager::File *SourceLocManager::findFile(char const *name)
{
  if (!this) {
    // it's quite common to forget to do this, and this function is 
    // almost always the one which segfaults in that case, so I'll
    // make the error message a bit nicer to save a trip through
    // the debugger
    xfailure("you have to create a SourceLocManager in your main() function");
  }

  if (recent && recent->name.equals(name)) {
    return recent;
  }

  FOREACH_OBJLIST_NC(File, files, iter) {
    if (iter.data()->name.equals(name)) {
      return recent = iter.data();
    }
  }

  return NULL;
}

// find it or make it
SourceLocManager::File *SourceLocManager::getFile(char const *name)
{
  File *f = findFile(name);
  if (!f) {
    // read the file
    f = new File(name, nextLoc);
    files.append(f);

    // bump 'nextLoc' according to how long that file was,
    // plus 1 so it can own the position equal to its length
    nextLoc = toLoc(f->startLoc + f->numChars + 1);
  }

  return recent = f;
}


SourceLoc SourceLocManager::encodeOffset(
  char const *filename, int charOffset)
{
  xassert(charOffset >= 0);

  File *f = getFile(filename);
  return toLoc(f->startLoc + charOffset);
}


SourceLoc SourceLocManager::encodeLineCol(
  char const *filename, int line, int col)
{
  xassert(line >= 1);
  xassert(col >= 1);

  File *f = getFile(filename);

  // map from a line number to a char offset
  int charOffset = f->lineToChar(line);
  return toLoc(toInt(f->startLoc) + charOffset + (col-1));
}


SourceLoc SourceLocManager::encodeStatic(StaticLoc const &obj)
{
  if (-toInt(nextStaticLoc) == maxStaticLocs) {
    // Each distinct static location should correspond to a single
    // place in the source code.  If one place in the source is creating
    // a given static location over and over, that's bad because it
    // quickly leads to poor performance when storing and decoding them.
    // Instead, make one and re-use it.
    //
    // If this message is being printed because the program is just
    // really big and has lots of distinct static locations, then you
    // can increase maxStaticLocs manually.
    fprintf(stderr,
      "Warning: You've created %d static locations, which is symptomatic\n"
      "of a bug.  See %s, line %d.\n",
      maxStaticLocs, __FILE__, __LINE__);
  }

  // save this location
  statics.append(new StaticLoc(obj));

  // return current index, yield next
  SourceLoc ret = nextStaticLoc;
  nextStaticLoc = toLoc(toInt(ret) - 1);
  return ret;
}


SourceLocManager::File *SourceLocManager::findFileWithLoc(SourceLoc loc)
{
  // check cache
  if (recent && recent->hasLoc(loc)) {
    return recent;
  }

  // iterative walk
  FOREACH_OBJLIST_NC(File, files, iter) {
    if (iter.data()->hasLoc(loc)) {
      return recent = iter.data();
    }
  }

  // the user gave me a value that I never made!
  xfailure("invalid source location");
  return NULL;    // silence warning
}


SourceLocManager::StaticLoc const *SourceLocManager::getStatic(SourceLoc loc)
{
  int index = -toInt(loc);
  return statics.nthC(index);
}


void SourceLocManager::decodeOffset(
  SourceLoc loc, char const *&filename, int &charOffset)
{
  // check for static
  if (isStatic(loc)) {
    StaticLoc const *s = getStatic(loc);
    filename = s->name.pcharc();
    charOffset = s->offset;
    return;
  }

  File *f = findFileWithLoc(loc);
  filename = f->name.pcharc();
  charOffset = toInt(loc) - toInt(f->startLoc);
}


void SourceLocManager::decodeLineCol(
  SourceLoc loc, char const *&filename, int &line, int &col)
{
  // check for static
  if (isStatic(loc)) {
    StaticLoc const *s = getStatic(loc);
    filename = s->name.pcharc();
    line = s->line;
    col = s->col;
    return;
  }

  File *f = findFileWithLoc(loc);
  filename = f->name.pcharc();
  int charOffset = toInt(loc) - toInt(f->startLoc);
  
  f->charToLineCol(charOffset, line, col);
}


char const *SourceLocManager::getFile(SourceLoc loc)
{
  char const *name;
  int ofs;
  decodeOffset(loc, name, ofs);
  return name;
}


int SourceLocManager::getOffset(SourceLoc loc)
{
  char const *name;
  int ofs;
  decodeOffset(loc, name, ofs);
  return ofs;
}


int SourceLocManager::getLine(SourceLoc loc)
{
  char const *name;
  int line, col;
  decodeLineCol(loc, name, line, col);
  return line;
}


int SourceLocManager::getCol(SourceLoc loc)
{
  char const *name;
  int line, col;
  decodeLineCol(loc, name, line, col);
  return col;
}


string SourceLocManager::getString(SourceLoc loc)
{
  char const *name;
  int line, col;
  decodeLineCol(loc, name, line, col);

  return stringc << name << ":" << line << ":" << col;
}


// -------------------------- test code ----------------------
#ifdef TEST_SRCLOC

#include "test.h"        // USUAL_MAIN

#include <stdlib.h>      // rand

SourceLocManager mgr;
int longestLen=0;

// given a location, decode it into line/col and then re-encode,
// and check that the new encoding matches the old
void testRoundTrip(SourceLoc loc)
{
  char const *fname;
  int line, col;
  mgr.decodeLineCol(loc, fname, line, col);

  if (col > longestLen) {
    longestLen = col;
  }

  SourceLoc loc2 = mgr.encodeLineCol(fname, line, col);
  
  xassert(loc == loc2);
}


// these are some long lines to test their handling
// 109: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 209: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 309: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 509: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 609: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 1210: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 2410: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 4810: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------
// 9610: 1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------1---------2---------3---------4---------5---------6---------7---------8---------9---------10--------


           
// location in SourceLoc and line/col
class BiLoc {
public:
  int line, col;
  SourceLoc loc;
};


// given a file, compute SourceLocs throughout it and verify
// that round-trip encoding works
void testFile(char const *fname)
{
  // find the file's length
  int len;
  {
    FILE *fp = fopen(fname, "r");
    if (!fp) {
      throw_XOpen(fname);
    }

    fseek(fp, 0, SEEK_END);
    len = (int)ftell(fp);

    fclose(fp);
  }

  // get locations for the start and end
  SourceLoc start = mgr.encodeOffset(fname, 0);
  SourceLoc end = mgr.encodeOffset(fname, len-1);

  // check expectations for start
  xassert(mgr.getLine(start) == 1);
  xassert(mgr.getCol(start) == 1);

  // test them
  testRoundTrip(start);
  testRoundTrip(end);

  // temporary
  //testRoundTrip((SourceLoc)11649);

  BiLoc *bi = new BiLoc[len];
  char const *dummy;

  // test all positions, forward sequential; also build the
  // map for the random test; note that 'len' is considered
  // a valid source location even though it corresponds to
  // the char just beyond the end
  int i;
  for (i=0; i<=len; i++) {
    SourceLoc loc = mgr.encodeOffset(fname, i);
    testRoundTrip(loc);

    bi[i].loc = loc;
    mgr.decodeLineCol(loc, dummy, bi[i].line, bi[i].col);
  }

  // backward sequential
  for (i=len; i>0; i--) {
    SourceLoc loc = mgr.encodeOffset(fname, i);
    testRoundTrip(loc);
  }

  // random access, both mapping directions
  for (i=0; i<=len; i++) {
    int j = rand()%(len+1);
    int dir = rand()%2;

    if (dir==0) {
      // test loc -> line/col map
      int line, col;
      mgr.decodeLineCol(bi[j].loc, dummy, line, col);
      xassert(line == bi[j].line);
      xassert(col == bi[j].col);
    }
    else {
      // test line/col -> loc map
      SourceLoc loc = mgr.encodeLineCol(fname, bi[j].line, bi[j].col);
      xassert(loc == bi[j].loc);
    }
  }
}


void entry(int argc, char **argv)
{
  traceAddSys("progress");
  traceProgress() << "begin" << endl;

  if (argc >= 2) {
    // set maxStaticLocs low to test the warning
    mgr.maxStaticLocs = 1;
  }

  // test my source code
  testFile("srcloc.cc");
  testFile("srcloc.h");

  // do it again, so at least one won't be the just-added file;
  // in fact do it many times so I can see results in a profiler
  for (int i=0; i<1; i++) {
    testFile("srcloc.cc");
    testFile("srcloc.h");
  }

  traceProgress() << "end" << endl;

  // protect against degeneracy by printing the length of
  // the longest line
  cout << "long line len: " << longestLen << endl;

  // test the statics
  cout << "invalid: " << toString(SL_UNKNOWN) << endl;
  cout << "here: " << toString(HERE_SOURCELOC) << endl;

  cout << "srcloc is ok\n";
}

ARGS_MAIN


#endif // TEST_SRCLOC
