// parsetables.cc
// code for parsetables.h

#include "parsetables.h"    // this module
#include "bflatten.h"       // BFlatten
#include "emitcode.h"       // EmitCode
#include "grammar.h"        // grammarStringTable
#include "trace.h"          // traceProgress
#include "crc.h"            // crc32


ParseTables::ParseTables(int t, int nt, int s, int p, StateId start, int final)
{
  alloc(t, nt, s, p, start, final);
}

void ParseTables::alloc(int t, int nt, int s, int p, StateId start, int final)
{
  owning = true;

  numTerms = t;
  numNonterms = nt;
  numStates = s;
  numProds = p;

  actionTable = new ActionEntry[actionTableSize()];
  memset(actionTable, 0, sizeof(actionTable[0]) * actionTableSize());

  gotoTable = new GotoEntry[gotoTableSize()];
  memset(gotoTable, 0, sizeof(gotoTable[0]) * gotoTableSize());

  prodInfo = new ProdInfo[numProds];
  memset(prodInfo, 0, sizeof(prodInfo[0]) * numProds);

  stateSymbol = new SymbolId[numStates];
  memset(stateSymbol, 0, sizeof(stateSymbol[0]) * numStates);

  startState = start;
  finalProductionIndex = final;
}


ParseTables::~ParseTables()
{
  if (owning) {
    delete[] actionTable;
    delete[] gotoTable;
    delete[] prodInfo;
    delete[] stateSymbol;

    for (int i=0; i<numAmbig(); i++) {
      delete[] ambigAction[i];
    }
  }
}


ActionEntry ParseTables::validateAction(int code) const
{
  // make sure that 'code' is representable; if this fails, most likely
  // there are more than 32k states or productions; in turn, the most
  // likely cause of *that* would be the grammar is being generated
  // automatically from some other specification; you can change the
  // typedefs of ActionEntry and GotoEntry in gramanl.h to get more
  // capacity
  ActionEntry ret = (ActionEntry)code;
  xassert((int)ret == code);
  return ret;
}

GotoEntry ParseTables::validateGoto(int code) const
{
  // see above
  GotoEntry ret = (GotoEntry)code;
  xassert((int)ret == code);
  return ret;
}


ParseTables::ParseTables(Flatten&)
{
  actionTable = NULL;
  gotoTable = NULL;
  prodInfo = NULL;
}

     
template <class T>
void xferSimpleArray(Flatten &flat, T *array, int numElements)
{
  int len = sizeof(array[0]) * numElements;
  flat.xferSimple(array, len);
  flat.checkpoint(crc32((unsigned char const *)array, len));
}

void ParseTables::xfer(Flatten &flat)
{
  // arbitrary number which serves to make sure we're at the
  // right point in the file
  flat.checkpoint(0x1B2D2F16);

  flat.xferInt(numTerms);
  flat.xferInt(numNonterms);
  flat.xferInt(numStates);
  flat.xferInt(numProds);

  flat.xferInt((int&)startState);
  flat.xferInt(finalProductionIndex);

  if (flat.reading()) {
    alloc(numTerms, numNonterms, numStates, numProds,
          startState, finalProductionIndex);
  }
           
  xferSimpleArray(flat, actionTable, actionTableSize());
  xferSimpleArray(flat, gotoTable, gotoTableSize());
  xferSimpleArray(flat, prodInfo, numProds);
  xferSimpleArray(flat, stateSymbol, numStates);

  // ambigAction
  if (flat.writing()) {
    flat.writeInt(numAmbig());
    for (int i=0; i<numAmbig(); i++) {
      flat.writeInt(ambigAction[i][0]);    // length of this entry

      for (int j=0; j<ambigAction[i][0]; j++) {
        flat.writeInt(ambigAction[i][j+1]);
      }
    }
  }

  else {
    int ambigs = flat.readInt();
    for (int i=0; i<ambigs; i++) {
      int len = flat.readInt();
      ActionEntry *entry = new ActionEntry[len+1];
      entry[0] = len;

      for (int j=0; j<len; j++) {
        entry[j+1] = flat.readInt();
      }

      ambigAction.push(entry);
    }
  }

  // make sure reading and writing agree
  flat.checkpoint(numAmbig());

}


// create literal tables and initialize 'ret' with pointers to them
template <class EltType>
void emitTable(EmitCode &out, EltType const *table, int size, int rowLength,
               char const *typeName, char const *tableName)
{
  out << "  static " << typeName << " " << tableName << "[" << size << "] = {";
  for (int i=0; i<size; i++) {
    if (i % rowLength == 0) {    // one row per state
      out << "\n    ";
    }
    out << table[i] << ", ";
  }
  out << "\n"
      << "  };\n";
}


// doesn't init anything; for use by emitConstructionCode's emitted code
ParseTables::ParseTables(bool o)
  : owning(o)
{
  xassert(owning == false);
}


// used to emit the elements of the prodInfo table
stringBuilder& operator<< (stringBuilder &sb, ParseTables::ProdInfo const &info)
{
  sb << "{" << (int)info.rhsLen << "," << (int)info.lhsIndex << "}";
  return sb;
}

// emit code for a function which, when compiled and executed, will
// construct this same table (except the constructed table won't own
// the table data, since it will be static program data)
void ParseTables::emitConstructionCode(EmitCode &out, char const *funcName)
{
  out << "// this makes a ParseTables from some literal data;\n"
      << "// the code is written by ParseTables::emitConstructionCode()\n"
      << "ParseTables *" << funcName << "()\n"
      << "{\n";

  out << "  ParseTables *ret = new ParseTables(false /*owning*/);\n"
      << "\n";

  // set all the integer-like variables
  #define SET_VAR(var) \
    out << "  ret->" #var " = " << var << ";\n";
  SET_VAR(numTerms);
  SET_VAR(numNonterms);
  SET_VAR(numStates);
  SET_VAR(numProds);
  out << "  ret->startState = (StateId)" << startState << ";\n";
  SET_VAR(finalProductionIndex);
  #undef SET_VAR
  out << "\n";

  // action table, one row per state
  emitTable(out, actionTable, actionTableSize(), numTerms, 
            "ActionEntry", "actionTable");
  out << "  ret->actionTable = actionTable;\n\n";

  // goto table, one row per state
  emitTable(out, gotoTable, gotoTableSize(), numNonterms,
            "GotoEntry", "gotoTable");
  out << "  ret->gotoTable = gotoTable;\n\n";

  // production info, arbitrarily 16 per row
  emitTable(out, prodInfo, numProds, 16, "ParseTables::ProdInfo", "prodInfo");
  out << "  ret->prodInfo = prodInfo;\n\n";

  // state symbol map, arbitrarily 16 per row
  emitTable(out, stateSymbol, numStates, 16, "SymbolId", "stateSymbol");
  out << "  ret->stateSymbol = stateSymbol;\n\n";

  // each of the ambiguous-action tables
  out << "  ret->ambigAction.setSize(" << numAmbig() << ");\n\n";
  for (int i=0; i<numAmbig(); i++) {
    string name = stringc << "ambigAction" << i;
    emitTable(out, ambigAction[i], ambigAction[i][0]+1, 16, "ActionEntry", name);
    out << "  ret->ambigAction[" << i << "] = " << name << ";\n\n";
  }
  out << "\n";

  out << "  return ret;\n"
      << "}\n";
}


ParseTables *readParseTablesFile(char const *fname)
{
  // assume it's a binary grammar file and try to
  // read it in directly
  traceProgress() << "reading parse tables file " << fname << endl;
  BFlatten flat(fname, true /*reading*/);

  ParseTables *ret = new ParseTables(flat);
  ret->xfer(flat);

  return ret;
}


void writeParseTablesFile(ParseTables const *tables, char const *fname)
{
  BFlatten flatOut(fname, false /*reading*/);
  
  // must cast away constness because there's no way to tell
  // the compiler that 'xfer' doesn't modify its argument
  // when it writing mode
  const_cast<ParseTables*>(tables)->xfer(flatOut);
}
