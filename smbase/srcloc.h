// srcloc.h            see license.txt for copyright and terms of use
// source location information, efficiently represented as one word

// The fundamental assumption in this module is that source location
// information is frequently created, stored and passed around, but
// infrequently decoded into human-readable form.  Therefore the
// module uses a single word to store the information, and appeals
// to several index structures when decoding is necessary.
//
// Since decoding, when it happens, also usually has high locality,
// the data structures include caches to make accesses to nearby
// locations fast.
//
// No attempt is made to fold creation of SourceLocs into other
// file-processing activities, such as traditional lexical analysis.
// The complexity of doing that would be substantial, with little
// gain in efficiency, due to the large buffer caches in modern
// OSes.

#ifndef SRCLOC_H
#define SRCLOC_H

#include "str.h"      // string
#include "objlist.h"  // ObjList


// This is a source location.  It's interpreted as an integer
// specifying the byte offset within a hypothetical file created by
// concatenating all the sources together.  Its type is 'enum' so I
// can overload functions to accept SourceLoc without confusion.
// I assume the compiler will use a machine word for this (and check
// that assumption in the .cc file).
enum SourceLoc { SL_UNKNOWN=0 };


// This class manages all the data associated with creating and
// interpreting SourceLocs.  It's expected to be a singleton in the
// program, though within this module that assumption is confined to
// the 'toString' function at the end.
class SourceLocManager {
private:     // types
  // a triple which identifies a line boundary in a file (it's
  // implicit which file it is) with respect to all of the relevant
  // spaces
  class Marker {
  public:
    // character offset, starting with 0
    int charOffset;

    // line offset, starting with 1
    int lineOffset;

    // offset into the 'lineLengths' array; this is not simply
    // lineOffset-1 because of the possible presence of lines with
    // length longer than 254 chars
    int arrayOffset;

  public:
    Marker() {}      // for creation in arrays
    Marker(int c, int L, int a)
      : charOffset(c), lineOffset(L), arrayOffset(a) {}
    Marker(Marker const &obj)
      : DMEMB(charOffset), DMEMB(lineOffset), DMEMB(arrayOffset) {}
  };

  // describes a file we know about
  class File {
  public:    // data
    // file name; we consider two files to be the same if and only
    // if their names are equal, i.e. there is no checking done to
    // see if their names happen to be aliases in the filesystem
    string name;

    // start offset in the SourceLoc space
    SourceLoc startLoc;
    
    // number of chars in the file
    int numChars;

    // number of lines in the file
    int numLines;

    // an array of line lengths; to handle lines longer than 255
    // chars, we use runs of '255' chars to (in unary) encode
    // multiples of 254 (one less than 255) chars, plus the final
    // short count to give the total length
    unsigned char *lineLengths;      // (owner)

    // # of elements in 'lineLengths'
    int lineLengthsSize;

    // this marker and offset can name an arbitrary point
    // in the array, including those that are not at the
    // start of a line; we move this around when searching
    // within the array
    Marker marker;
    int markerCol;      // 1-based column; it's usually 1


    #if 0  // not yet
    // an index built on top of 'lineLengths' for faster random
    // access; each element indexes approximately 1000 entries in the
    // former array, thus the size of 'index' is approximately 1% of
    // the size of 'lineLengths'
    Marker *index;                   // (owner)

    // # of elements in 'index'
    int indexSize;
    #endif // 0

  private:   // funcs
    File(File&);                     // disallowed
    void resetMarker();
    void advanceMarker();

  public:    // funcs
    // this builds both the array and the index
    File(char const *name, SourceLoc startLoc);
    ~File();
    
    // line number to character offset
    int lineToChar(int lineNum);

    // char offset to line/col
    void charToLineCol(int offset, int &line, int &col);

    // true if this file contains the specified location
    bool hasLoc(SourceLoc sl) const
      { return toInt(startLoc) <= sl &&
               toInt(startLoc) + numChars > sl; }
  };

public:      // types
  // this is used for SourceLocs where the file isn't reliably
  // available, yet we'd like to be able to store some location
  // information anyway; the queries below just return the static
  // information stored, and incremental update is impossible
  class StaticLoc {
  public:
    string name;      // file name
    int offset;       // char offset
    int line, col;    // line,col
              
  public:
    StaticLoc(char const *n, int o, int L, int c)
      : name(n), offset(o), line(L), col(c) {}
    StaticLoc(StaticLoc const &obj)
      : DMEMB(name), DMEMB(offset), DMEMB(line), DMEMB(col) {}
    ~StaticLoc();
  };

private:     // data
  // list of files; it would be possible to use a data structure
  // that is faster to search, but the cache ought to exploit
  // query locality to the extent that it doesn't matter
  ObjList<File> files;

  // most-recently accessed File; this is a cache
  File *recent;                      // (serf)

  // list of StaticLocs; any SourceLoc less than 0 is interpreted
  // as an index into this list
  ObjList<StaticLoc> statics;

  // next source location to assign
  SourceLoc nextLoc;

  // next static (negative) location
  SourceLoc nextStaticLoc;

private:     // funcs
  // let File know about these functions
  friend class SourceLocManager::File;

  static SourceLoc toLoc(int L);
  static int toInt(SourceLoc loc) { return (int)loc; }
  static bool isStatic(SourceLoc loc) { return toInt(loc) <= 0; }

  File *findFile(char const *name);
  File *getFile(char const *name);

  File *findFileWithLoc(SourceLoc loc);
  StaticLoc const *getStatic(SourceLoc loc);

public:      // funcs
  SourceLocManager();
  ~SourceLocManager();

  // origins:
  //   character offsets start at 0
  //   lines start at 1
  //   columns start at 1

  // encode from scratch
  SourceLoc encodeOffset(char const *filename, int charOffset);
  SourceLoc encodeLineCol(char const *filename, int line, int col);
  SourceLoc encodeStatic(StaticLoc const &obj);

  // encode incremental; these are the methods we expect are called
  // the most frequently; this interface is supposed to allow an
  // implementation which uses explicit line/col, even though that
  // is not what is used here
  SourceLoc advCol(SourceLoc base, int colOffset)
    { return toLoc(toInt(base) + colOffset); }
  SourceLoc advLine(SourceLoc base)     // from end of line to beginning of next
    { return toLoc(toInt(base) + 1); }
  SourceLoc advText(SourceLoc base, char const *text, int textLen)
    { return toLoc(toInt(base) + textLen); }

  // decode
  void decodeOffset(SourceLoc loc, char const *&filename, int &charOffset);
  void decodeLineCol(SourceLoc loc, char const *&filename, int &line, int &col);

  // more specialized decode
  char const *getFile(SourceLoc loc);
  int getOffset(SourceLoc loc);
  int getLine(SourceLoc loc);
  int getCol(SourceLoc loc);

  // render as string in "file:line:col" format
  string getString(SourceLoc loc);
};


// singleton pointer, set automatically by the constructor
extern SourceLocManager *sourceLocManager;

// take advantage of singleton
inline string toString(SourceLoc sl)
  { return sourceLocManager->getString(sl); }


#endif // SRCLOC_H
