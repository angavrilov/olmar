// emittables.cc            see license.txt for copyright and terms of use
// implementation of ParseTables::emitConstructionCode()

#include "parsetables.h"    // this module
#include "emitcode.h"       // EmitCode


// create literal tables
template <class EltType>
void emitTable(EmitCode &out, EltType const *table, int size, int rowLength,
               char const *typeName, char const *tableName)
{
  out << "  static " << typeName << " " << tableName << "[" << size << "] = {";
  for (int i=0; i<size; i++) {
    if (i % rowLength == 0) {    // one row per state
      out << "\n    ";
    }

    if (sizeof(table[i]) == 1) {
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

  // table of ambiguous nonterminals
  emitTable(out, ambigNonterms, ambigTableSize(), 16, 
            "unsigned char", "ambigNonterms");
  out << "  ret->ambigNonterms = ambigNonterms;\n\n";

  out << "  return ret;\n"
      << "}\n";
}

