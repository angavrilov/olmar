// parsetables.cc            see license.txt for copyright and terms of use
// code for parsetables.h

#include "parsetables.h"    // this module
#include "bflatten.h"       // BFlatten
#include "trace.h"          // traceProgress
#include "crc.h"            // crc32
#include "emitcode.h"       // EmitCode
#include "bit2d.h"          // Bit2d

#include <string.h>         // memset
#include <stdlib.h>         // qsort


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

  actionCols = numTerms;
  actionRows = numStates;

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
                                       
  // no compressed info
  uniqueErrorRows = 0;
  errorBits = NULL;
  errorBitsPointers = NULL;

  actionIndexMap = NULL;
  actionRowPointers = NULL;
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
    }
    if (actionIndexMap) {
      delete[] actionIndexMap;
    }
  }

  // these are always owned
  if (errorBitsPointers) {
    delete[] errorBitsPointers;
  }            
  if (actionRowPointers) {
    delete[] actionRowPointers;
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
  traceProgress() << "computing errorBits[]\n";

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

  // compute which rows are identical; I only compress the rows (and
  // not the columns) because I can fold the former's compression into
  // the errorBitsPointers[] access, whereas the latter would require
  // yet another table
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
  delete[] compressed;

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


void ParseTables::mergeActionColumns()
{
  traceProgress() << "merging action columns\n";

  // can only do this if we've already pulled out the errors
  xassert(errorBits);

  // for now I assume we don't have a map yet
  xassert(!actionIndexMap);

  // compute graph of conflicting 'action' columns
  // (will be symmetric)
  Bit2d graph(point(numTerms, numTerms));
  graph.setall(0);

  // fill it in
  for (int t1=0; t1 < numTerms; t1++) {
    for (int t2=0; t2 < t1; t2++) {
      // does column 't1' conflict with column 't2'?
      for (int s=0; s < numStates; s++) {
        ActionEntry a1 = actionEntry(s, t1);
        ActionEntry a2 = actionEntry(s, t2);

        if (isErrorAction(a1) ||
            isErrorAction(a2) ||
            a1 == a2) {
          // no problem
        }
        else {
          // conflict!
          graph.set(point(t1, t2));
          graph.set(point(t2, t1));
          break;
        }
      }
    }
  }
  
  // color the graph
  Array<int> color(numTerms);      // terminal -> color
  int numColors = colorTheGraph(color, graph);
  
  // build a new, compressed action table
  ActionEntry *newTable = new ActionEntry[numStates * numColors];
  memset(newTable, 0, sizeof(newTable[0]) * numStates * numColors);
  
  // merge columns in 'actionTable' into those in 'newTable' 
  // according to the 'color' map
  actionIndexMap = new TermIndex[numTerms];
  for (int t=0; t<numTerms; t++) {
    int c = color[t];

    // merge actionTable[t] into newTable[c]
    for (int s=0; s<numStates; s++) {
      ActionEntry &dest = newTable[s*numColors + c];

      ActionEntry src = actionEntry(s, t);
      if (!isErrorAction(src)) {
        // make sure there's no conflict (otherwise the graph
        // coloring algorithm screwed up)
        xassert(isErrorAction(dest) ||
                dest == src);

        // merge the entry
        dest = src;
      }
    }

    // fill in the action index map
    TermIndex ti = (TermIndex)c;
    xassert(ti == c);     // otherwise value truncation happened
    actionIndexMap[t] = ti;
  }

  trace("compression")
    << "action table: from " << (actionTableSize() * sizeof(ActionEntry))
    << " down to " << (numStates * numColors * sizeof(ActionEntry))
    << " bytes\n";

  // replace the existing table with the compressed one
  delete[] actionTable;
  actionTable = newTable;
  actionCols = numColors;
}


void ParseTables::mergeActionRows()
{
  traceProgress() << "merging action rows\n";

  // can only do this if we've already pulled out the errors
  xassert(errorBits);

  // I should already have a column map
  xassert(actionIndexMap);

  // for now I assume we don't have a map yet
  xassert(!actionRowPointers);

  // compute graph of conflicting 'action' rows
  // (will be symmetric)
  Bit2d graph(point(numStates, numStates));
  graph.setall(0);

  // fill it in
  for (int s1=0; s1 < numStates; s1++) {
    for (int s2=0; s2 < s1; s2++) {
      // does row 's1' conflict with row 's2'?
      for (int t=0; t < actionCols; t++) {    // t is an equivalence class of terminals
        ActionEntry a1 = actionTable[s1*actionCols + t];
        ActionEntry a2 = actionTable[s2*actionCols + t];

        if (isErrorAction(a1) ||
            isErrorAction(a2) ||
            a1 == a2) {
          // no problem
        }
        else {
          // conflict!
          graph.set(point(s1, s2));
          graph.set(point(s2, s1));
          break;
        }
      }
    }
  }
  
  // color the graph
  Array<int> color(numStates);      // state -> color (equivalence class)
  int numColors = colorTheGraph(color, graph);
  
  // build a new, compressed action table
  ActionEntry *newTable = new ActionEntry[numColors * actionCols];
  memset(newTable, 0, sizeof(newTable[0]) * numColors * actionCols);

  // merge rows in 'actionTable' into those in 'newTable'
  // according to the 'color' map
  
  // actionTable[]:
  //
  //             t0    t1    t2    t3      // terminal equivalence classes
  //   s0
  //   s1
  //   s2
  //    ...
  //   /*states*/

  // newTable[]:
  //
  //             t0    t1    t2    t3      // terminal equivalence classes
  //   c0
  //   c1
  //   c2    < e.g., union of state1 and state4 (color[1]==color[4]==2) >
  //    ...
  //   /*state equivalence classes (colors)*/

  actionRowPointers = new ActionEntry* [numStates];
  for (int s=0; s<numStates; s++) {
    int c = color[s];

    // merge actionTable row 's' into newTable row 'c'
    for (int t=0; t<actionCols; t++) {
      ActionEntry &dest = newTable[c*actionCols + t];

      ActionEntry src = actionTable[s*actionCols + t];
      if (!isErrorAction(src)) {
        // make sure there's no conflict (otherwise the graph
        // coloring algorithm screwed up)
        xassert(isErrorAction(dest) ||
                dest == src);

        // merge the entry
        dest = src;
      }
    }

    // fill in the row pointer map
    actionRowPointers[s] = newTable + c*actionCols;
  }

  trace("compression")
    << "action table: from " << (numStates * actionCols * sizeof(ActionEntry))
    << " down to " << (numColors * actionCols * sizeof(ActionEntry))
    << " bytes\n";

  // replace the existing table with the compressed one
  delete[] actionTable;
  actionTable = newTable;
  actionRows = numColors;
  
  // how many single-value rows?
  {
    int ct=0;
    for (int s=0; s<actionRows; s++) {
      int val = 0;
      for (int t=0; t<actionCols; t++) {
        int entry = actionRowPointers[s][t];
        if (val==0) {
          val = entry;
        }
        else if (entry != 0 && entry != val) {
          // not all the same
          goto next_s;
        }
      }
      
      // all same
      ct++;
      
    next_s:
      ;
    }
    trace("compression") << ct << " same-valued rows\n";
  }
}


static int intCompare(void const *left, void const *right)
{
  return *((int const*)left) - *((int const*)right);
}

int ParseTables::colorTheGraph(int *color, Bit2d &graph)
{
  int n = graph.Size().x;  // same as y

  if (tracingSys("graphColor") && n < 20) {
    graph.print();
  }

  // node -> # of adjacent nodes
  Array<int> degree(n);
  memset((int*)degree, 0, n * sizeof(int));

  // node -> # of adjacent nodes that have colors already
  Array<int> blocked(n);

  // initialize some arrays
  enum { UNASSIGNED = -1 };
  {
    for (int i=0; i<n; i++) {
      // clear the color map
      color[i] = UNASSIGNED;
      blocked[i] = 0;

      for (int j=0; j<n; j++) {
        if (graph.get(point(i,j))) {
          degree[i]++;
        }
      }
    }
  }

  // # of colors used
  int usedColors = 0;

  for (int numColored=0; numColored < n; numColored++) {
    // Find a vertex to color.  Prefer nodes that are more constrained
    // (have more blocked colors) to those that are less constrained.
    // Then, prefer those that are least constraining (heave least
    // uncolored neighbors) to those that are more constraining.  If
    // ties remain, choose arbitrarily.
    int best = -1;
    int bestBlocked = 0;
    int bestUnblocked = 0;

    for (int choice = 0; choice < n; choice++) {
      if (color[choice] != UNASSIGNED) continue;

      int chBlocked = blocked[choice];
      int chUnblocked = degree[choice] - blocked[choice];
      if (best == -1 ||                          // no choice yet
          chBlocked > bestBlocked ||             // more constrained
          (chBlocked == bestBlocked &&
           chUnblocked < bestUnblocked)) {       // least constraining
        // new best
        best = choice;
        bestBlocked = chBlocked;
        bestUnblocked = chUnblocked;
      }
    }

    // get the assigned colors of the adjacent vertices
    Array<int> adjColor(bestBlocked);
    int adjIndex = 0;
    for (int i=0; i<n; i++) {
      if (graph.get(point(best,i)) &&
          color[i] != UNASSIGNED) {
        adjColor[adjIndex++] = color[i];
      }
    }
    xassert(adjIndex == bestBlocked);

    // sort them
    qsort((int*)adjColor, bestBlocked, sizeof(int), intCompare);

    // select the lowest-numbered color that won't conflict
    int selColor = 0;
    for (int j=0; j<bestBlocked; j++) {
      if (selColor == adjColor[j]) {
        selColor++;
      }
      else if (selColor < adjColor[j]) {
        // found one that doesn't conflict
        break;
      }
      else {
        // happens when we have two neighbors that have the same color;
        // that's fine, we'll go around the loop again to see what the
        // next neighbor has to say
      }
    }

    // assign 'selColor' to 'best'
    color[best] = selColor;
    if (selColor+1 > usedColors) {
      usedColors = selColor+1;
    }

    // update 'blocked[]'
    for (int k=0; k<n; k++) {
      if (graph.get(point(best,k))) {
        // every neighbor of 'k' now has one more blocked color
        blocked[k]++;
      }
    }
  }

  ostream &os = trace("graphColor") << "colors[]:";

  for (int i=0; i<n; i++) {
    // every node should now have blocked == degree
    xassert(blocked[i] == degree[i]);

    // and have a color assigned
    xassert(color[i] != UNASSIGNED);
    os << " " << color[i];
  }
  
  os << "\n";

  return usedColors;
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
    if (size % rowLength == 0) {
      out << "  // rows: " << (size/rowLength) << "  cols: " << rowLength << "\n";
    }
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


template <class EltType>
void emitOffsetTable(EmitCode &out, EltType **table, EltType *base, int size,
                     char const *typeName, char const *tableName, char const *baseName)
{
  out << "  ret->" << tableName << " = new " << typeName << " [" << size << "];\n";

  // make the pointers persist by storing a table of offsets
  Array<int> offsets(size);
  for (int i=0; i < size; i++) {
    offsets[i] = table[i] - base;
  }
  emitTable(out, (int*)offsets, size, 16, "int", stringc << tableName << "_offsets");

  // at run time, interpret the offsets table
  out << "  for (int i=0; i < " << size << "; i++) {\n"
      << "    ret->" << tableName << "[i] = ret->" << baseName
      <<                                " + " << tableName << "_offsets[i];\n"
      << "  }\n";
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
  SET_VAR(actionCols);
  SET_VAR(actionRows);
  out << "  ret->startState = (StateId)" << (int)startState << ";\n";
  SET_VAR(finalProductionIndex);
  SET_VAR(errorBitsRowSize);
  SET_VAR(uniqueErrorRows);
  #undef SET_VAR
  out << "\n";

  // action table, one row per state
  emitTable(out, actionTable, actionTableSize(), actionCols,
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
    out << "  ret->errorBitsPointers = NULL;\n";
  }
  else {
    emitTable(out, errorBits, uniqueErrorRows * errorBitsRowSize, errorBitsRowSize,
              "ErrorBitsEntry", "errorBits");
    out << "  ret->errorBits = errorBits;\n";
    out << "\n";

    emitOffsetTable(out, errorBitsPointers, errorBits, numStates,
                    "ErrorBitsEntry*", "errorBitsPointers", "errorBits");

    #if 0     // delete me
    out << "  ret->errorBitsPointers = new ErrorBitsEntry* [" << numStates << "];\n";

    // make the pointers persist by storing a table of offsets
    int *offsets = new int[numStates];
    for (int s=0; s < numStates; s++) {
      offsets[s] = errorBitsPointers[s] - errorBits;
    }
    emitTable(out, offsets, numStates, 16, "int", "offsets");
    delete[] offsets;

    // at run time, interpret the offsets table
    out << "  for (int s=0; s < " << numStates << "; s++) {\n"
        << "    ret->errorBitsPointers[s] = ret->errorBits + offsets[s];\n"
        << "  }\n";       
    #endif // 0
  }
  out << "\n";

  // actionIndexMap
  if (!actionIndexMap) {
    out << "  ret->actionIndexMap = NULL;\n";
  }
  else {
    emitTable(out, actionIndexMap, numTerms, 16,
              "TermIndex", "actionIndexMap");
    out << "  ret->actionIndexMap = actionIndexMap;\n";
  }
  out << "\n";

  // actionRowPointers
  if (!actionRowPointers) {
    out << "  ret->actionRowPointers = NULL;\n";
  }
  else {
    emitOffsetTable(out, actionRowPointers, actionTable, numStates,
                    "ActionEntry*", "actionRowPointers", "actionTable");
  }
  out << "\n";

  out << "  return ret;\n"
      << "}\n";
}


// EOF
