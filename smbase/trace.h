// trace.h            see license.txt for copyright and terms of use
// module for diagnostic tracing

#ifndef __TRACE_H
#define __TRACE_H

#include <iostream.h>     // ostream


// add a subsystem to the list of those being traced
void traceAddSys(char const *sysName);

// remove a subsystem; must have been there
void traceRemoveSys(char const *sysName);

// see if a subsystem is among those to trace
bool tracingSys(char const *sysName);

// clear all tracing flags
void traceRemoveAll();


// trace; if the named system is active, this yields cout (after
// sending a little output to identify the system); if not, it
// yields an ostream attached to /dev/null; when using this
// method, it is up to you to put the newline
ostream &trace(char const *sysName);

// give an entire string to trace; do *not* put a newline in it
// (the tracer will do that)
void trstr(char const *sysName, char const *traceString);


// special for "progress" tracing; prints time too;
// 'level' is level of detail -- 1 is highest level, 2 is
// more refined (and therefore usually not printed), etc.
ostream &traceProgress(int level=1);


// add one or more subsystems, separated by commas
void traceAddMultiSys(char const *systemNames);

// if the first argument is a tracing directive, handle it, modify
// argc and argv modified to effectively remove it, and return true
// (argv[0] is assumed to be ignored by everything)
bool traceProcessArg(int &argc, char **&argv);

// so here's a simple loop that will consume any leading
// trace arguments
#define TRACE_ARGS() while (traceProcessArg(argc, argv)) {}


#endif // __TRACE_H
