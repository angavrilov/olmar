// fileloc.cc
// code for fileloc.h

#include "fileloc.h"       // this module

// -------------------- FileLocation -----------------------------
string FileLocation::toString() const
{
  return stringc << "line " << line << ", col " << col;
}


void FileLocation::advance(char const *text, int length)
{
  char const *p = text;
  char const *endp = text + length;
  for (; p < endp; p++) {
    if (*p == '\n') {      // new line
      line++;
      col = 1;
    }
    else {	      	   // ordinary character
      col++;
    }
  }
}


void FileLocation::newLine()
{
  line++;
  col = firstColumn;
}


// ------------------- SourceFile --------------------
SourceFile::~SourceFile()
{}


// ------------------- SourceLocation --------------------
SourceLocation::SourceLocation(SourceLocation const &obj)
  : FileLocation(obj),
    file(obj.file)
{}

SourceLocation& SourceLocation::operator= (SourceLocation const &obj)
{
  FileLocation::operator=(obj);
  file = obj.file;
  return *this;
}


SourceLocation::SourceLocation(FileLocation const &floc, SourceFile *f)
  : FileLocation(floc),
    file(f)
{}


string SourceLocation::toString() const
{
  return stringc << "file " << fname() << ", "
                 << FileLocation::toString();
}


// ------------------- SourceFileList -----------------
SourceFileList::SourceFileList()
{}

SourceFileList::~SourceFileList()
{}


SourceFile *SourceFileList::open(char const *fname)
{
  // check for an existing SourceFile
  MUTATE_EACH_OBJLIST(SourceFile, files, iter) {
    if (0==strcmp(iter.data()->filename, fname)) {
      // found match
      return iter.data();
    }
  }

  // make a new one
  SourceFile *ret = new SourceFile(fname);
  files.append(ret);
  return ret;
}


// the global list
// (not thread-safe)
SourceFileList sourceFileList;
