// trace.cc
// code for trace.h

#include "trace.h"     // this module
#include "objlist.h"   // List
#include "str.h"       // string
#include "strtokp.h"   // StrtokParse
#include "nonport.h"   // getMilliseconds()

#include <fstream.h>   // ofstream


// auto-init
static bool inited = false;

// list of active tracers, initially empty
static ObjList<string> tracers;

// stream connected to /dev/null
static ostream *devNull = NULL;


// initialize
static void init()
{
  if (inited) {
    return;
  }

  // there's a more efficient way to do this, but I don't care to dig
  // around and find out how
  devNull = new ofstream("/dev/null");

  inited = true;
}


void traceAddSys(char const *sysName)
{
  init();

  tracers.prepend(new string(sysName));
}


void traceRemoveSys(char const *sysName)
{
  init();

  MUTATE_EACH_OBJLIST(string, tracers, mut) {
    if (mut.data()->compareTo(sysName) == 0) {
      mut.deleteIt();
      return;
    }
  }
  xfailure("traceRemoveSys: tried to remove system that isn't there");
}


bool tracingSys(char const *sysName)
{
  init();

  FOREACH_OBJLIST(string, tracers, iter) {
    if (iter.data()->compareTo(sysName) == 0) {
      return true;
    }
  }
  return false;
}


ostream &trace(char const *sysName)
{
  init();

  if (tracingSys(sysName)) {
    cout << "%%% " << sysName << ": ";
    return cout;
  }
  else {
    return *devNull;
  }
}


void trstr(char const *sysName, char const *traceString)
{
  trace(sysName) << traceString << endl;
}


ostream &traceProgress()
{
  static long progStart = getMilliseconds();
  
  return trace("progress") << (getMilliseconds() - progStart) << "ms: ";
}


void traceAddMultiSys(char const *systemNames)
{
  StrtokParse tok(systemNames, ",");
  loopi(tok) {
    if (tok[i][0] == '-') {
      // treat a leading '-' as a signal to *remove*
      // a tracing flag, e.g. from some defaults specified
      // statically      
      char const *name = tok[i]+1;
      if (tracingSys(name)) {      // be forgiving here
        traceRemoveSys(name);
      }
      else {
        cout << "Currently, `" << name << "' is not being traced.\n";
      }
    }
    
    else {
      // normal behavior: add things to the trace list
      traceAddSys(tok[i]);
    }
  }
}


bool traceProcessArg(int &argc, char **&argv)
{
  if (argc >= 3  &&  0==strcmp(argv[1], "-tr")) {
    traceAddMultiSys(argv[2]);
    argc -= 2;
    argv += 2;
    return true;
  }
  else {
    return false;
  }
}
