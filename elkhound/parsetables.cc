// parsetables.cc            see license.txt for copyright and terms of use
// code for parsetables.h

#include "parsetables.h"    // this module
#include "bflatten.h"       // BFlatten
#include "trace.h"          // traceProgress
#include "crc.h"            // crc32

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
