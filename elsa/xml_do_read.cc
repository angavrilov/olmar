// xml_do_read.cc          see license.txt for copyright and terms of use

#include "xml_do_read.h"        // this module
#include "fstream.h"            // ifstream
#include "xml_writer.h"         // XmlReaderManager
#include "xml_lexer.h"          // AstXmlLexer
#include "xml_file_reader.h"    // XmlFileReader
#include "xml_type_reader.h"    // XmlTypeReader
#include "xml_ast_reader.h"     // XmlAstReader

class TranslationUnit;


TranslationUnit *xmlDoRead(StringTable &strTable, char const *inputFname) {
  // make reader manager
  ifstream in(inputFname);
  AstXmlLexer lexer(inputFname);
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

  manager.readUntilTUnitTag();

  // complete the link graph
  manager.satisfyLinks();

//    if (manager.getLastKind() != XTOK_TranslationUnit) {
//      manager.userError("top tag is not a TranslationUnit");
//    }
  TranslationUnit *tunit = (TranslationUnit*) manager.getLastNode();

  return tunit;
}
