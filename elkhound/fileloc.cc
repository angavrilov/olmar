// fileloc.cc
// code for fileloc.h

#include "fileloc.h"       // this module

// -------------------- FileLocation -----------------------------
string FileLocation::toString() const
{
  if (isValid()) {
    return stringc << "line " << line << ", col " << col;
  }
  else {
    return string("(unknown loc)");
  }
}


void FileLocation::advance(char const *text, int length)
{ 
  // can't advance an invalid location
  xassert(isValid());

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
  // can't advance an invalid location
  xassert(isValid());

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


char const *SourceLocation::fname() const
{ 
  if (file) {
    return file->filename;
  }
  else {
    return NULL;
  }
}

string SourceLocation::toString() const
{
  if (fname()) {
    return stringc << "file " << fname() << ", "
                   << FileLocation::toString();
  }
  else {
    return FileLocation::toString();
  }
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
