// fileloc.h
// data structures for recording character positions in files

#ifndef __FILELOC_H
#define __FILELOC_H

#include "str.h"       // string
#include "objlist.h"   // ObjList

// identifies a location in a source file
class FileLocation {
public:
  enum Constants {
    firstColumn = 1,               // number of first column
    firstLine = 1,                 // number of first line
  };

  int line;    // line #, 1-based
  int col;     // column #, 1-based

public:
  FileLocation()                          : line(1), col(1) {}
  FileLocation(FileLocation const &obj)	  : line(obj.line), col(obj.col) {}
  ~FileLocation()                         {}

  FileLocation& operator= (FileLocation const &obj)
    { line=obj.line; col=obj.col; return *this; }
    
  // "line %d, col %d"
  string toString() const;

  // move forward to reflect location after 'text'
  void advance(char const *text, int length);
  
  // wrap to the next line
  void newLine();
};


// names a source file
// (will get bigger; mostly a placeholder for now)
class SourceFile {
public:
  string filename;
  
public:
  SourceFile(char const *fn) : filename(fn) {}
  ~SourceFile();
};


// position in file, and pointer to which file
class SourceLocation : public FileLocation {
public:
  SourceFile *file;         // (serf)

public:
  SourceLocation(SourceFile *f = NULL) : file(f) {}
  SourceLocation(FileLocation const &floc, SourceFile *f);
  SourceLocation(SourceLocation const &obj);
  ~SourceLocation() {}

  SourceLocation& operator= (SourceLocation const &obj);
  
  char const *fname() const { return file->filename; }

  // "file %s, line %d, col %d"
  string toString() const;
};


// global list of files processed; expectation is tools toss
// files in here when opened and use the resulting pointer to
// refer to the file, even after it's closed
class SourceFileList {
private:     // data
  ObjList<SourceFile> files;
  
public:
  SourceFileList();
  ~SourceFileList();
                                      
  // get permanent name for a file; if you call open twice
  // with the same name (case sensitive), it will return the
  // same structure as before
  SourceFile * /*serf*/ open(char const *fname);
};

// the global list
extern SourceFileList sourceFileList;


#endif // __FILELOC_H
