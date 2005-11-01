// xml_do_read.cc          see license.txt for copyright and terms of use

#include "xml_do_read.h"        // this module
#include "fstream.h"            // ifstream
#include "xml_lexer.h"          // XmlLexer
#include "xml_file_reader.h"    // XmlFileReader
#include "xml_type_reader.h"    // XmlTypeReader
#include "xml_ast_reader.h"     // XmlAstReader

class TranslationUnit;


TranslationUnit *xmlDoRead(StringTable &strTable, char const *inputFname) {
  // make reader manager
  ifstream in(inputFname);
  XmlLexer lexer(inputFname);
  lexer.restart(&in);
  XmlReaderManager manager(inputFname, lexer, strTable);

  // prevent the SourceLocManager from looking at files in the file
  // system
  sourceLocManager->mayOpenFiles = false;

  // make file reader
  XmlFileReader fileReader;
  manager.registerReader(&fileReader);

  // make ast reader
  XmlAstReader astReader;
  manager.registerReader(&astReader);

  // make type reader
//    BasicTypeFactory tFac;
  XmlTypeReader typeReader;
  manager.registerReader(&typeReader);

  // read until we get a translation unit tag; FIX: not sure what
  // happens if the last tag is not a TranslationUnit
  while(true) {
    manager.parseOneTopLevelTag();
    if (lexer.haveSeenEof()) {
      manager.userError("unexpected EOF");
    }
    int lastKind = manager.getLastKind();
    if (lastKind == XTOK_File) {
      // complete the link graph so that the FileData object is
      // complete
      manager.satisfyLinks();

      SourceLocManager::FileData *fileData = (SourceLocManager::FileData*) manager.getLastNode();
      if (!fileData->complete()) {
        manager.userError("missing attributes to File tag");
      }
      sourceLocManager->loadFile(fileData);
      // FIX: recursively delete the file and its members here
    } else if (lastKind == XTOK_TranslationUnit) {
      break;                    // we are done
    } else {
      manager.userError("illegal top-level tag");
    }
  }

  // complete the link graph
  manager.satisfyLinks();

//    if (manager.getLastKind() != XTOK_TranslationUnit) {
//      manager.userError("top tag is not a TranslationUnit");
//    }
  TranslationUnit *tunit = (TranslationUnit*) manager.getLastNode();

  return tunit;
}
