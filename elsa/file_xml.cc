// file_xml.cc            see license.txt for copyright and terms of use

#include "file_xml.h"           // this module
#include "strutil.h"            // quoted
#include "xml.h"                // xml serialization macro library
#include "astxml_tokens.h"      // XTOK_*
#include "sobjset.h"            // SObjSet

// make sure that printing of various File objects is idempotent
SObjSet<void const *> printedSetFI;

identity(FI, SourceLocManager::File)
identity(FI, HashLineMap)
identity(FI, HashLineMap::HashLine)
identityTempl(FI, ArrayStack<T>)


FileToXml::FileToXml(ostream &out0, int &depth0, bool indent0)
  : ToXml(out0, depth0, indent0)
{}

void FileToXml::toXml(ObjList<SourceLocManager::File> &files)
{
  FOREACH_OBJLIST_NC(SourceLocManager::File, files, iter) {
    SourceLocManager::File *file = iter.data();
    toXml(file);
  }
}

void FileToXml::toXml(SourceLocManager::File *file)
{
  // idempotency
  if (printed(file)) return;
  openTag(File, file);
  // **** attributes
  printStrRef(name, file->name.c_str());
  printXml_int(numChars, file->numChars);
  printXml_int(numLines, file->numLines);

  // this doesn't work because lineLengths is private
//    printPtr(file, lineLengths);
  unsigned char *lineLengths = file->serializationOnly_get_lineLengths();
  // FIX: this special situation just breaks all the macros so we do
  // it manually
  newline();
  out << "lineLengths=\"FI" << reinterpret_cast<void const *>(lineLengths) << "\"";

  printPtr(file, hashLines);
  tagEnd;

  // **** subtags
  // NOTE: we do not use the trav() macro as we call a non-standard
  // method name; see note at the method declaration
  if (lineLengths) {
    // NOTE: we pass the file instead of the lineLengths
    toXml_lineLengths(file);
  }
  trav(file->hashLines);
}

void FileToXml::toXml_lineLengths(SourceLocManager::File *file)
{
  // NOTE: no idempotency check is needed as the line lengths are
  // one-to-one with the Files.
  unsigned char *lineLengths = file->serializationOnly_get_lineLengths();
  // NOTE: can't do this since we would have to implement dispatch on
  // a pointer to unsigned chars, which is too general; we need
  // LineLengths to be their own class.
//    openTagWhole(LineLengths, lineLengths);
  newline();
  out << "<LineLengths _id=\"FI" << reinterpret_cast<void const *>(lineLengths) << "\">";
  XmlCloseTagPrinter tagCloser("LineLengths", *this);
  IncDec depthManager(this->depth);

  // **** sub-data
  // Note: This simple whitespace-separated list is the suggested
  // output format for lists of numbers in XML:
  // http://www.w3.org/TR/xmlschema-0/primer.html#ListDt
  //
  // Note also that I do not bother to indent blocks of data between
  // tags, just the tags themselves.
  int lineLengthsSize = file->serializationOnly_get_lineLengthsSize();
  for (int i=0; i<lineLengthsSize; ++i) {
    if (i%20 == 0) cout << "\n";
    else cout << " ";
    cout << static_cast<int>(lineLengths[i]);
  }
}

void FileToXml::toXml(HashLineMap *hashLines)
{
  // idempotency
  if (printed(hashLines)) return;
  openTag(HashLineMap, hashLines);
  // **** attributes
  string &ppFname = hashLines->serializationOnly_get_ppFname();
  printStrRef(ppFname, ppFname.c_str());

  // NOTE: can't do this because it is private; FIX: I have inlined
  // "FI" here.
//    printEmbed(hashLines, directives);
  ArrayStack<HashLineMap::HashLine> &directives = hashLines->serializationOnly_get_directives();
  newline();
  out << "directives=\"FI" << addr(&directives) << "\"";
  tagEnd;

  // **** subtags
  // FIX: again, it is private so I inline the macro
//    travArrayStack(hashLines, HashLineMap, directives, HashLine);
  if (!printed(&directives)) {
    openTagWhole(List_HashLineMap_directives, &directives);
    FOREACH_ARRAYSTACK_NC(HashLineMap::HashLine, directives, iter) {
      travListItem(iter.data());
    }
  }
}

void FileToXml::toXml(HashLineMap::HashLine *hashLine)
{
  // idempotency
  if (printed(hashLine)) return;
  openTag(HashLine, hashLine);
  // **** attributes
  printXml_int(ppLine, hashLine->ppLine);
  printXml_int(origLine, hashLine->origLine);
  printStrRef(origFname, hashLine->origFname);
  tagEnd;
}


// -------------------- FileXmlReader -------------------

void *FileXmlReader::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: return NULL;
  case 0: userError("unexpected file termination while looking for an open tag name");
  case XTOK_File:
    return new SourceLocManager::FileData();
    break;
  case XTOK_LineLengths:
    // NOTE: This is not technically a list object has it does not
    // have list item children, only data; it is a regular node that
    // happens to be a list of data.
    return new ArrayStack<unsigned char>;
    break;
  case XTOK_HashLineMap:
    return new HashLineMap("");
    break;
  case XTOK_HashLine:
    return new HashLineMap::HashLine();
    break;
  case XTOK_List_HashLineMap_directives:
    return new ASTList<HashLineMap::HashLine>;
    break;
  }
}


bool FileXmlReader::registerStringToken(void *target, int kind, char const *yytext0) {
  switch(kind) {
  default: return false; break;

  case XTOK_File: {
    userError("cannot register data with a File tag");
    break;
  }

  case XTOK_LineLengths: {
    ArrayStack<unsigned char> *lineLengths = (ArrayStack<unsigned char>*)target;
    // FIX: this does not detect any errors if it is not a non-neg int
    lineLengths->push(atoi(yytext0));
    break;
  }

  }
  return true;
}


bool FileXmlReader::registerAttribute(void *target0, int kind, int attr, char const *strValue) {
  switch(kind) {
  default: return false; break;

  case XTOK_File: {
    SourceLocManager::FileData *obj = (SourceLocManager::FileData*)target0;
    switch(attr) {
    default: userError("illegal attribute for a File tag"); break;
    case XTOK_name: obj->name = manager->strTable(parseQuotedString(strValue)); break;
    case XTOK_numChars: fromXml_int(obj->numChars, parseQuotedString(strValue)); break;
    case XTOK_numLines: fromXml_int(obj->numLines, parseQuotedString(strValue)); break;
    case XTOK_lineLengths: ul(lineLengths, XTOK_LineLengths); break;
    case XTOK_hashLines: ul(hashLines, XTOK_HashLineMap); break;
    }
    break;
  }

  case XTOK_LineLengths: {
    // currently unused so I turned it off to avoid the compiler warning
//      ArrayStack<unsigned char> *obj = (ArrayStack<unsigned char>*)target0;
    switch(attr) {
    default: userError("illegal attribute for a LineLengths tag"); break;
    }
    break;
  }

  case XTOK_HashLineMap: {
    HashLineMap *obj = (HashLineMap*) target0;
    switch(attr) {
    default: userError("illegal attribute for a HashLineMap tag"); break;
    case XTOK_ppFname: obj->serializationOnly_set_ppFname(parseQuotedString(strValue)); break;
    case XTOK_directives:
      ulList(_List, directives, XTOK_List_HashLineMap_directives);
      break;
    // NOTE: there is no XTOK_filenames; the file names dictionary is
    // redundant and reconstructed from the File names fields
    }
    break;
  }

  case XTOK_HashLine: {
    HashLineMap::HashLine *obj = (HashLineMap::HashLine*) target0;
    switch(attr) {
    default: userError("illegal attribute for a HashLine tag"); break;
    case XTOK_ppLine: fromXml_int(obj->ppLine, parseQuotedString(strValue)); break;
    case XTOK_origLine: fromXml_int(obj->origLine, parseQuotedString(strValue)); break;
    case XTOK_origFname: obj->origFname = manager->strTable(parseQuotedString(strValue)); break;
    }
    break;
  }

  }
  return true;
}

bool FileXmlReader::kind2kindCat(int kind, KindCategory *kindCat) {
  switch(kind) {
  default: return false;        // we don't know this kind
  case XTOK_File:
  case XTOK_LineLengths:
  case XTOK_HashLineMap:
  case XTOK_HashLine:
    *kindCat = KC_Node;
    break;

  case XTOK_List_HashLineMap_directives:
    *kindCat = KC_ArrayStack;
    break;
  }
  return true;
}

bool FileXmlReader::recordKind(int kind, bool& answer) {
  switch(kind) {
  default: return false;        // we don't know this kind
  // **** do not record these
  case XTOK_File:
  case XTOK_LineLengths:
  case XTOK_HashLineMap:
  case XTOK_HashLine:
  case XTOK_List_HashLineMap_directives:
    answer = false;
    return true;
    break;
  }
}

bool FileXmlReader::callOpAssignToEmbeddedObj(void *obj, int kind, void *target) {
  xassert(obj);
  xassert(target);
  switch(kind) {
  default:
    // This handler conflates two situations; see the node in
    // TypeXmlReader::callOpAssignToEmbeddedObj().
    return false;
    break;
  }
}

bool FileXmlReader::upcastToWantedType(void *obj, int kind, void **target, int targetKind) {
  xassert(obj);
  xassert(target);
  // This handler conflates two situations; see the node in
  // TypeXmlReader::upcastToWantedType
  return false;
}

bool FileXmlReader::convertList2FakeList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool FileXmlReader::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool FileXmlReader::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool FileXmlReader::convertList2ArrayStack(ASTList<char> *list, int listKind, void **target) {
  xassert(list);
  switch(listKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_List_HashLineMap_directives:
    // FIX: the HashLine objects are being copied by value here, so
    // the originals should be destructed
    convertArrayStack(ArrayStack, HashLineMap::HashLine);
    break;
  }

  return true;
}

bool FileXmlReader::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

bool FileXmlReader::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}
