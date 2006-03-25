// xml_file_writer.cc            see license.txt for copyright and terms of use

#include "xml_file_writer.h"    // this module
#include "xml_writer.h"         // serialization support
#include "asthelp.h"            // xmlAttrQuote()
#include "sobjset.h"            // SObjSet


// make sure that printing of various File objects is idempotent
SObjSet<void const *> printedSetFI;

identity_defn(FI, SourceLocManager::File)
identity_defn(FI, HashLineMap)
identity_defn(FI, HashLineMap::HashLine)
identity_defn(FI, unsigned char) // for lineLengths
identityTempl_defn(FI, ArrayStack<T>)
identityTempl_defn(FI, ObjList<T>)

XmlFileWriter::XmlFileWriter(ostream &out0, int &depth0, bool indent0)
  : XmlWriter(out0, depth0, indent0)
{}

void XmlFileWriter::toXml(ObjList<SourceLocManager::File> &files)
{
  travObjList0(files, files, SourceLocManager::File, FOREACH_OBJLIST_NC, ObjList);

  // FOREACH_OBJLIST_NC(SourceLocManager::File, files, iter) {
  //   SourceLocManager::File *file = iter.data();
  //   toXml(file);
  // }
}

void XmlFileWriter::toXml(SourceLocManager::File *file)
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
  out << "lineLengths=" << xmlAttrQuote(xmlPrintPointer("FI", uniqueId(lineLengths)));

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

void XmlFileWriter::toXml_lineLengths(SourceLocManager::File *file)
{
  // NOTE: no idempotency check is needed as the line lengths are
  // one-to-one with the Files.
  unsigned char *lineLengths = file->serializationOnly_get_lineLengths();
  // NOTE: can't do this since we would have to implement dispatch on
  // a pointer to unsigned chars, which is too general; we need
  // LineLengths to be their own class.
//    openTagWhole(LineLengths, lineLengths);
  newline();
  out << "<LineLengths _id=" << xmlAttrQuote(xmlPrintPointer("FI", uniqueId(lineLengths))) << ">";
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
    if (i%20 == 0) out << "\n";
    else out << " ";
    out << static_cast<int>(lineLengths[i]);
  }
}

void XmlFileWriter::toXml(HashLineMap *hashLines)
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
  out << "directives=" << xmlAttrQuote(xmlPrintPointer("FI", uniqueId(&directives)));
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

void XmlFileWriter::toXml(HashLineMap::HashLine *hashLine)
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
