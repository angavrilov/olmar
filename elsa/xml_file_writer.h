// xml_file_writer.h            see license.txt for copyright and terms of use

// Serialization of file information for purposes of capturing the
// state of the SourceLocManager.

#ifndef XML_FILE_WRITER_H
#define XML_FILE_WRITER_H

#include "objlist.h"            // ObjList
#include "srcloc.h"             // SourceLocManager
#include "xml_writer.h"         // XmlWriter
#include "hashline.h"           // HashLineMap


class XmlFileWriter : public XmlWriter {
  public:
  XmlFileWriter(ostream &out0, int &depth0, bool indent0);

  void toXml(ObjList<SourceLocManager::File> &files);
  void toXml(SourceLocManager::File *file);
  // this is an exception to the generic toXml() mechanism since
  // lineLengths are not self-contained
  void toXml_lineLengths(SourceLocManager::File *file);
  void toXml(HashLineMap *hashLines);
  void toXml(HashLineMap::HashLine *hashLine);
};

#endif // XML_FILE_WRITER_H
