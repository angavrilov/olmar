// parsetables.cc            see license.txt for copyright and terms of use
// code for parsetables.h

#include "parsetables.h"    // this module
#include "bflatten.h"       // BFlatten
#include "trace.h"          // traceProgress
#include "crc.h"            // crc32
#include "emitcode.h"       // EmitCode

#include <string.h>         // memset


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

  nontermOrder = new NtIndex[nontermOrderSize()];
  memset(nontermOrder, 0, sizeof(nontermOrder[0]) * nontermOrderSize());
            
  // # of bytes, but rounded up to nearest 32-bit boundary
  errorBitsRowSize = ((numTerms+31) >> 5) * 4;
  uniqueErrorRows = 0;
  errorBits = NULL;                    // not computed yet
  errorBitsPointers = NULL;
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

    delete[] nontermOrder;

    if (errorBits) {
      delete[] errorBits;
      delete[] errorBitsPointers;    // TODO: this one is always owned..
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


ParseTables::ParseTables(Flatten &flat)
{
  actionTable = NULL;
  gotoTable = NULL;
  prodInfo = NULL;
  stateSymbol = NULL;
  nontermOrder = NULL;
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
  xferSimpleArray(flat, nontermOrder, nontermOrderSize());

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


// see emittables.cc for ParseTables::emitConstructionCode()
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


// doesn't init anything; for use by emitConstructionCode's emitted code
ParseTables::ParseTables(bool o)
  : owning(o)
{
  xassert(owning == false);
}

      
// -------------------- table compression --------------------
void ParseTables::computeErrorBits()
{                     
  // should only be done once
  xassert(!errorBits);       

  // allocate and clear it
  int rowSize = ((numTerms+31) >> 5) * 4;
  errorBits = new ErrorBitsEntry [numStates * rowSize];
  memset(errorBits, 0, sizeof(errorBits[0]) * numStates * rowSize);

  // build the pointer table
  errorBitsPointers = new ErrorBitsEntry* [numStates];

  // find and set the error bits
  fillInErrorBits(true /*setPointers*/);

  // compute which rows are identical
  int *compressed = new int[numStates];   // row -> new location in errorBits[]
  uniqueErrorRows = 0;
  int s;
  for (s=0; s < numStates; s++) {
    // is 's' the same as any rows that preceded it?
    for (int t=0; t < s; t++) {
      // do 's' and 't' have the same contents?
      if (0==memcmp(errorBitsPointers[s],
                    errorBitsPointers[t],
                    sizeof(ErrorBitsEntry) * errorBitsRowSize)) {
        // yes, map 's' to 't' instead
        compressed[s] = compressed[t];
        goto next_s;
      }
    }

    // not the same as any
    compressed[s] = uniqueErrorRows;
    uniqueErrorRows++;

  next_s:
    ;
  }

  // make a smaller 'errorBits' array
  delete[] errorBits;
  errorBits = new ErrorBitsEntry [uniqueErrorRows * rowSize];
  memset(errorBits, 0, sizeof(errorBits[0]) * uniqueErrorRows * rowSize);

  // rebuild 'errorBitsPointers' according to 'compressed'
  for (s=0; s < numStates; s++) {
    errorBitsPointers[s] = errorBits + (compressed[s] * errorBitsRowSize);
  }

  // fill in the bits again, using the new pointers map
  fillInErrorBits(false /*setPointers*/);
}


void ParseTables::fillInErrorBits(bool setPointers)
{
  for (int s=0; s < numStates; s++) {
    if (setPointers) {
      errorBitsPointers[s] = errorBits + (s * errorBitsRowSize);
    }

    for (int t=0; t < numTerms; t++) {
      if (isErrorAction(actionEntry(s, t))) {
        ErrorBitsEntry &b = errorBitsPointers[s][t >> 3];
        b |= 1 << (t & 7);
      }
    }
  }
}



// --------------------- table emission -------------------
// create literal tables
template <class EltType>
void emitTable(EmitCode &out, EltType const *table, int size, int rowLength,
               char const *typeName, char const *tableName)
{
  bool printHex = 0==strcmp(typeName, "ErrorBitsEntry");
  
  if (size * sizeof(*table) > 50) {    // suppress small ones
    out << "  // storage size: " << size * sizeof(*table) << " bytes\n";
  }

  out << "  static " << typeName << " " << tableName << "[" << size << "] = {";
  for (int i=0; i<size; i++) {
    if (i % rowLength == 0) {    // one row per state
      out << "\n    ";
    }

    if (printHex) {
      out << stringf("0x%02X, ", table[i]);
    }
    else if (sizeof(table[i]) == 1) {
      // little bit of a hack to make sure 'unsigned char' gets
      // printed as an int; the casts are necessary because this
      // code gets compiled even when EltType is ProdInfo
      out << (int)(*((unsigned char*)(table+i))) << ", ";
    }
    else {
      // print the other int-sized things, or ProdInfo using
      // the overloaded '<<' below
      out << table[i] << ", ";
    }
  }
  out << "\n"
      << "  };\n";
}

// used to emit the elements of the prodInfo table
stringBuilder& operator<< (stringBuilder &sb, ParseTables::ProdInfo const &info)
{
  sb << "{" << (int)info.rhsLen << "," << (int)info.lhsIndex << "}";
  return sb;
}


// emit code for a function which, when compiled and executed, will
// construct this same table (except the constructed table won't own
// the table data, since it will point to static program data)
void ParseTables::emitConstructionCode(EmitCode &out, char const *funcName)
{
  out << "// this makes a ParseTables from some literal data;\n"
      << "// the code is written by ParseTables::emitConstructionCode()\n"
      << "// in " << __FILE__ << "\n"
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
  out << "  ret->startState = (StateId)" << (int)startState << ";\n";
  SET_VAR(finalProductionIndex);
  SET_VAR(errorBitsRowSize);
  SET_VAR(uniqueErrorRows);
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

  // nonterminal order
  emitTable(out, nontermOrder, nontermOrderSize(), 16,
            "NtIndex", "nontermOrder");
  out << "  ret->nontermOrder = nontermOrder;\n\n";

  // errorBits
  if (!errorBits) {
    out << "  ret->errorBits = NULL;\n";
    out << "  ret->errorBitsPointers = NULL;\n\n";
  }
  else {
    emitTable(out, errorBits, uniqueErrorRows * errorBitsRowSize, errorBitsRowSize,
              "ErrorBitsEntry", "errorBits");
    out << "  ret->errorBits = errorBits;\n";
    out << "\n";
    out << "  ret->errorBitsPointers = new ErrorBitsEntry* [" << numStates << "];\n";
    
    // for now, use this sort of hackish way of making the pointers persist
    for (int s=0; s < numStates; s++) {
      out << "  ret->errorBitsPointers[" << s << "] = ret->errorBits + "
          << (errorBitsPointers[s] - errorBits) << ";\n";
    }
  }
  out << "\n";

  out << "  return ret;\n"
      << "}\n";
}


// EOF
