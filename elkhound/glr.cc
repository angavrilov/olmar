// glr.cc
// code for glr.h

/* Implementation Notes
 *
 * A design point: [GLR] uses more 'global's than I do.  My criteria
 * here is that something should be global (stored in class GLR) if
 * it has meaning between processing of tokens.  If something is only
 * used during the processing of a single token, then I make it a
 * parameter where necessary.
 *
 * Update: I've decided to make 'currentToken' and 'parserWorklist'
 * global because they are needed deep inside of 'glrShiftNonterminal',
 * though they are not needed by the intervening levels, and their
 * presence in the argument lists would therefore only clutter them.
 *
 * It should be clear that many factors contribute to this
 * implementation being slow, and I'm going to refrain from any
 * optimization for a bit.
 *
 * UPDATE (3/29/02): I'm now trying to optimize it.  The starting
 * implementation is 300x slower than bison.  Ideal goal is 3x, but
 * more realistic is 10x.
 *
 * Description of the various lists in play here:
 *
 *   activeParsers
 *   -------------
 *   The active parsers are at the frontier of the parse tree
 *   space.  It *never* contains more than one stack node with
 *   a given parse state; I call this the unique-state property
 *   (USP).  If we're about to add a stack node with the same
 *   state as an existing node, we merge them (if it's a shift,
 *   we add another leftAdjState; if it's a reduction, we add a
 *   rule node *and* another leftAdjState).
 *
 *   Before a token is processed, activeParsers contains those
 *   parsers that successfully shifted the previous token.  This
 *   list is then copied to the parserWorklist (described below).
 *
 *   As reductions are processed, new parsers are generated and
 *   added to activeParsers (modulo USP).
 *
 *   Before the accumulated shifts are processed, the activeParsers
 *   list is cleared.  As each shift is processed, the resulting
 *   parser is added to activeParsers (modulo USP).
 *
 *   [GLR] calls this "active-parsers"
 *
 *
 *   parserWorklist
 *   --------------
 *   The worklist contains all parsers we have not yet been considered
 *   for advancement (shifting or reducing).  Initially, it is the
 *   same as the activeParsers, but we take a parser from it on
 *   each iteration of the inner loop in 'glrParse'.
 *
 *   Whenever, during processing of reductions, we add a parser to
 *   activeParsers, we also add it to parserWorklist.  This ensures
 *   the just-reduced parser gets a chance to reduce or shift.
 *
 *   [GLR] calls this "for-actor"
 *
 *
 *   pendingShifts
 *   -------------
 *   The GLR parser alternates between reducing and shifting.
 *   During the processing of a given token, shifting happens last.
 *   This keeps the parsers synchronized, since every token is
 *   shifted by every parser at the same time.  This synchronization
 *   is important because it makes merging parsers possible.
 *
 *   [GLR] calls this "for-shifter"
 *
 * 
 * Discussion of path re-examination, called do-limited-reductions by
 * [GLR]:
 *
 * After thinking about this for some time, I have reached the conclusion
 * that the only way to handle the problem is to separate the collection
 * of paths from the iteration over them.
 *
 * Here are several alternative schemes, and the reasons they don't
 * work:
 *
 *   1. [GLR]'s approach of limiting re-examination to those involving
 *      the new link
 *
 *      This fails because it does not prevent re-examined paths
 *      from appearing in the normal iteration also.
 *
 *   2. Modify [GLR] so the new link can't be used after the re-examination
 *      is complete
 *
 *      Then if *another* new link is added, paths involving both new
 *      links wouldn't be processed.
 *
 *   3. Further schemes involving controlling which re-examination stage can
 *      use which links
 *
 *      Difficult to reason about, unclear a correct scheme exists, short
 *      of the full-blown path-listing approach I'm going to take.
 *
 *   4. My first "fix" which assumes there is never more than one path to
 *      a given parser
 *
 *      This is WRONG.  There can be more than one path, even as all such
 *      paths are labelled the same (namely, with the RHS symbols).  Consider
 *      grammar "E -> x | E + E" parsing "x+x+x": both toplevel parses use
 *      the "E -> E + E" rule, and both arrive at the root parser
 *
 * So, the solution I will implement is to collect all paths into a list
 * before processing any of them.  During path re-examination, I also will
 * collect paths into a list, this time only those that involve the new
 * link.
 *
 * This scheme is clearly correct, since path collection cannot be disrupted
 * by the process of adding links, and when links are added, exactly the new
 * paths are collected and processed.  It's easy to see that every path is
 * considered exactly once.
 *
 *
 * Below, parse-tree building activity is marked "TREEBUILD".
 */


#include "glr.h"         // this module
#include "strtokp.h"     // StrtokParse
#include "syserr.h"      // xsyserror
#include "trace.h"       // tracing system
#include "strutil.h"     // replace
#include "lexer1.h"      // Lexer1
#include "lexer2.h"      // Lexer2
#include "grampar.h"     // readGrammarFile
#include "parssppt.h"    // treeMain
#include "bflatten.h"    // BFlatten

#include <stdio.h>       // FILE
#include <fstream.h>     // ofstream

// D(..) is code to execute for extra debugging info
#ifdef EXTRA_CHECKS
  #define D(stmt) stmt
#else
  #define D(stmt) ((void)0)
#endif

// TRSPARSE(stuff) traces <stuff> during debugging with -tr parse
#if !defined(NDEBUG)
  #define TRSPARSE(stuff) if (trParse) { trsParse << stuff << endl; }
#else
  #define TRSPARSE(stuff)
#endif

// can turn this on to experiment.. but right now it 
// actually makes things slower.. (!)
//#define USE_PARSER_INDEX


// the transition to array-based implementations requires I specify
// initial sizes (they all grow as needed though)
enum {
  TYPICAL_MAX_RHSLEN = 10,
  TYPICAL_MAX_REDUCTION_PATHS = 5,
};


// ------------- front ends to user code ---------------
SemanticValue GLR::duplicateSemanticValue(SymbolId sym, SemanticValue sval)
{
  xassert(sym != 0);
  if (!sval) return sval;

  if (symIsTerm(sym)) {
    return userAct->duplicateTerminalValue(symAsTerm(sym), sval);
  }
  else {
    return userAct->duplicateNontermValue(symAsNonterm(sym), sval);
  }
}

void deallocateSemanticValue(SymbolId sym, UserActions *user,
                             SemanticValue sval)
{
  xassert(sym != 0);
  if (!sval) return;

  if (symIsTerm(sym)) {
    return user->deallocateTerminalValue(symAsTerm(sym), sval);
  }
  else {
    return user->deallocateNontermValue(symAsNonterm(sym), sval);
  }
}

void GLR::deallocateSemanticValue(SymbolId sym, SemanticValue sval)
{
  ::deallocateSemanticValue(sym, userAct, sval);
}


// ----------------------- StackNode -----------------------
int StackNode::numStackNodesAllocd=0;
int StackNode::maxStackNodesAllocd=0;


StackNode::StackNode()
  : stackNodeId(-1),
    tokenColumn(-1),
    state(STATE_INVALID),
    leftSiblings(),
    referenceCount(0),
    determinDepth(0),
    glr(NULL)
{
  // the interesting stuff happens in init()
}

StackNode::~StackNode()
{
  // the interesting stuff happens in deinit()
}


void StackNode::init(int nodeId, int col, StateId st, GLR *g)
{
  stackNodeId = nodeId;
  tokenColumn = col;
  state = st;
  referenceCount = 0;
  determinDepth = 1;    // 0 siblings now, so this node is unambiguous
  glr = g;
  INC_HIGH_WATER(numStackNodesAllocd, maxStackNodesAllocd);
}

void StackNode::deinit()
{
  numStackNodesAllocd--;
  if (!unwinding()) {
    xassert(numStackNodesAllocd >= 0);
    xassert(referenceCount == 0);
  }

  // explicitly deallocate siblings, so I can deallocate their
  // semantic values if necessary (this requires knowing the
  // associated symbol, which the SiblinkLinks don't know)
  while (leftSiblings.isNotEmpty()) {
    Owner<SiblingLink> sib(leftSiblings.removeAt(0));
    D(trace("sval") << "deleting sval " << sib->sval << endl);
    deallocateSemanticValue(getSymbolC(), glr->userAct, sib->sval);
  }
}


SymbolId StackNode::getSymbolC() const
{
  return glr->tables->stateSymbol[state];
}


// add a new sibling by creating a new link
SiblingLink *StackNode::
  addSiblingLink(StackNode *leftSib, SemanticValue sval,
                 SourceLocation const &loc)
{
  if (leftSiblings.isNotEmpty()) {
    // there's currently at least one sibling, and now we're adding another;
    // right now, no other stack node should point at this one (if it does,
    // most likely will catch that when we use the stale info)
    determinDepth = 0;
  }
  else {
    // we don't have any siblings yet, so my depth will be my new sibling's
    // depth, plus 1
    determinDepth = leftSib->determinDepth + 1;
  }

  SiblingLink *link = new SiblingLink(leftSib, sval, loc);
  leftSiblings.append(link);
  return link;
}


void StackNode::decRefCt()
{
  xassert(referenceCount > 0);
  if (--referenceCount == 0) {
    glr->stackNodePool.dealloc(this);
  }
}


STATICDEF void StackNode::printAllocStats()
{
  cout << "stack nodes: " << numStackNodesAllocd
       << ", max stack nodes: " << maxStackNodesAllocd
       << endl;
}


// ------------------ SiblingLink ------------------
SiblingLink::~SiblingLink()
{}


// ------------------ PendingShift ------------------
PendingShift::~PendingShift()
{
  deinit();
}          

PendingShift& PendingShift::operator=(PendingShift const &obj)
{
  if (this != &obj) {
    parser = obj.parser;
    shiftDest = obj.shiftDest;
  }
  return *this;
}


void PendingShift::deinit()
{
  parser = NULL;     // decrement reference count
  shiftDest = STATE_INVALID;
}


// ------------------ PathCollectionState -----------------
PathCollectionState::PathCollectionState()
  : startStateId(STATE_INVALID),
    siblings(TYPICAL_MAX_RHSLEN),
    symbols(TYPICAL_MAX_RHSLEN),
    paths(TYPICAL_MAX_REDUCTION_PATHS)
{}

PathCollectionState::~PathCollectionState()
{}


void PathCollectionState::init(int pi, int len, StateId start)
{
  startStateId = start;
  xassert(paths.length()==0); // paths must start empty
  prodIndex = pi;

  // IN PROGRESS:
    // possible optimization: allocate these once, using the largest length
    // of any production, when parsing starts; do the same for 'toPass'
    // in collectReductionPaths

  siblings.ensureIndexDoubler(len-1);
  symbols.ensureIndexDoubler(len-1);
}


void PathCollectionState::deinit()
{
  while (paths.isNotEmpty()) {
    paths.popAlt().deinit();
  }
}


void PathCollectionState
  ::addReductionPath(StackNode *f, SemanticValue s, SourceLocation const &L)
{
  paths.pushAlt().init(f, s, L);
}


PathCollectionState::ReductionPath::~ReductionPath()
{
  deinit();
}


PathCollectionState::ReductionPath& PathCollectionState::ReductionPath
  ::operator=(PathCollectionState::ReductionPath const &obj)
{
  if (&obj != this) {
    finalState = obj.finalState;
    sval = obj.sval;
    loc = obj.loc;
  }
  return *this;
}


void PathCollectionState::ReductionPath::deinit()
{
  if (sval) {
    // this should be very unusual, since 'sval' is consumed
    // (and nullified) soon after the ReductionPath is created;
    // it can happen if an exception is thrown from certain places
    cout << "interesting: using ReductionPath's deallocator on " << sval << endl;
    deallocateSemanticValue(finalState->getSymbolC(), finalState->glr->userAct, sval);
  }

  finalState = NULL;     // decrement reference count
}


// ------------------------- GLR ---------------------------
void decParserList(ArrayStack<StackNode*> &list)
{
  for (int i=0; i < list.length(); i++) {
    list[i]->decRefCt();
  }
}

void incParserList(ArrayStack<StackNode*> &list)
{
  for (int i=0; i < list.length(); i++) {
    list[i]->incRefCt();
  }
}
               
// candidate for adding to ArrayStack.. but I'm hesitant for some reason
bool parserListContains(ArrayStack<StackNode*> &list, StackNode *node)
{
  for (int i=0; i < list.length(); i++) {
    if (list[i] == node) {
      return true;
    }
  }
  return false;
}


GLR::GLR(UserActions *user)
  : userAct(user),
    activeParsers(),
    parserIndex(NULL),
    nextStackNodeId(initialStackNodeId),
    currentTokenColumn(0),
    currentTokenClass(NULL),
    currentTokenValue(NULL),
    parserWorklist(),
    pcsStack(),
    pcsStackHeight(0),
    toPass(TYPICAL_MAX_RHSLEN),
    stackNodePool(30),
    trParse(tracingSys("parse")),
    trsParse(trace("parse")),
    trSval(tracingSys("sval")),
    trsSval(trace("sval"))
  // some fields (re-)initialized by 'clearAllStackNodes'
{}

GLR::~GLR()
{
  decParserList(activeParsers);
  decParserList(parserWorklist);
  if (parserIndex) {
    delete[] parserIndex;
  }
}


void GLR::clearAllStackNodes()
{
  nextStackNodeId = initialStackNodeId;
  currentTokenColumn = 0;

  // the stack nodes themselves are now reference counted, so they
  // should already be cleared if we're between parses
}


// process the input file, and yield a parse graph
bool GLR::glrParseNamedFile(Lexer2 &lexer2, SemanticValue &treeTop,
                            char const *inputFname)
{
  // do first phase lexer
  traceProgress() << "lexical analysis...\n";
  traceProgress(2) << "lexical analysis stage 1...\n";
  Lexer1 lexer1;
  {
    FILE *input = fopen(inputFname, "r");
    if (!input) {
      xsyserror("fopen", inputFname);
    }

    lexer1_lex(lexer1, input);
    fclose(input);

    if (lexer1.errors > 0) {
      printf("L1: %d error(s)\n", lexer1.errors);
      return false;
    }
  }

  // do second phase lexer
  traceProgress(2) << "lexical analysis stage 2...\n";
  lexer2_lex(lexer2, lexer1, inputFname);

  // parsing itself
  return glrParse(lexer2, treeTop);
}


// used to extract the svals from the nodes just under the
// start symbol reduction
SemanticValue GLR::grabTopSval(StackNode *node)
{
  SiblingLink *sib = node->leftSiblings.first();
  SemanticValue ret = sib->sval;
  sib->sval = duplicateSemanticValue(node->getSymbolC(), sib->sval);

  trsSval << "dup'd " << ret << " for top sval, yielded "
          << sib->sval << endl;

  return ret;
}

bool GLR::glrParse(Lexer2 const &lexer2, SemanticValue &treeTop)
{
  // get ready..
  traceProgress() << "parsing...\n";
  clearAllStackNodes();

  // build the parser index (I do this regardless of whether I'm going
  // to use it, because up here it makes no performance difference,
  // and I'd like as little code as possible being #ifdef'd)
  if (parserIndex) {
    delete[] parserIndex;
  }
  parserIndex = new ParserIndexEntry[numItemSets()];
  {
    for (int i=0; i<numItemSets(); i++) {
      parserIndex[i] = INDEX_NO_PARSER;
    }
  }

  // create an initial ParseTop with grammar-initial-state,
  // set active-parsers to contain just this
  StackNode *first = makeStackNode(startState->id);
  addActiveParser(first);
  
  // we will queue up shifts and process them all at the end (pulled
  // out of loop so I don't deallocate the array between tokens)
  ArrayStack<PendingShift> pendingShifts;                  // starts empty

  // for each input symbol
  int tokenNumber = 0;
  for (ObjListIter<Lexer2Token> tokIter(lexer2.tokens);
       !tokIter.isDone(); tokIter.adv(), tokenNumber++) {
    Lexer2Token const *currentToken = tokIter.data();

    // debugging
    TRSPARSE(
           "------- "
        << "processing token " << currentToken->toString()
        << ", " << activeParsers.length() << " active parsers"
        << " -------"
    )

    // token reclassification
    int classifiedType = userAct->reclassifyToken(currentToken->type,
                                                  currentToken->sval);

    // convert the token to a symbol
    xassert((unsigned)classifiedType < (unsigned)numTerms);
    currentTokenClass = indexedTerms[classifiedType];
    currentTokenColumn = tokenNumber;
    currentTokenValue = currentToken->sval;
    currentTokenLoc = currentToken->loc;


  tryDeterministic:    
    // optimization: if there's only one active parser, and the
    // action is unambiguous, and it doesn't involve traversing
    // parts of the stack which are nondeterministic, then do the
    // parse action the way an ordinary LR parser would
    //
    // please note:  The code in this section is cobbled together
    // from various other GLR functions.  Everything here appears in
    // at least one other place, so modifications will usually have
    // to be done in both places.
    if (activeParsers.length() == 1) {
      StackNode *parser = activeParsers[0];
      xassert(parser->referenceCount==1);     // 'activeParsers[0]' is referrer
      ActionEntry action =
        tables->actionEntry(parser->state, currentTokenClass->termIndex);

      if (tables->isShiftAction(action)) {
        // can shift unambiguously
        StateId newState = tables->decodeShift(action);

        TRSPARSE("state " << parser->state <<
                 ", (unambig) shift token " << currentTokenClass->name <<
                 ", to state " << newState);

        StackNode *rightSibling = makeStackNode(newState);
        rightSibling->addSiblingLink(parser, currentTokenValue, currentTokenLoc);

        // replace 'parser' with 'rightSibling' in the activeParsers list
        activeParsers[0] = rightSibling;
        parser->decRefCt();
        xassert(parser->referenceCount==1);         // rightSibling refers to it
        rightSibling->incRefCt();
        xassert(rightSibling->referenceCount==1);   // activeParsers[0] refers to it

        // get next token
        continue;
      }

      else if (tables->isReduceAction(action)) {
        int prodIndex = tables->decodeReduce(action);
        ParseTables::ProdInfo const &prodInfo = tables->prodInfo[prodIndex];
        int rhsLen = prodInfo.rhsLen;
        if (rhsLen <= parser->determinDepth) {
          // can reduce unambiguously
          int startStateId = parser->state;

          // record location of left edge
          SourceLocation leftEdge;     // defaults to no location (used for epsilon rules)

          // pop off 'rhsLen' stack nodes, collecting as many semantic
          // values into 'toPass'
          toPass.ensureIndexDoubler(rhsLen-1);
          for (int i=rhsLen-1; i>=0; i--) {
            // grab 'parser's only sibling link
            xassert(parser->leftSiblings.count() == 1);
            SiblingLink *sib = parser->leftSiblings.first();

            // Store its semantic value it into array that will be
            // passed to user's routine.  Note that there is no need to
            // dup() this value, since it will never be passed to
            // another action routine (avoiding that overhead is
            // another advantage to the LR mode).
            toPass[i] = sib->sval;
            sib->sval = NULL;                  // link no longer owns the value

            // if it has a valid source location, grab it
            if (sib->loc.validLoc()) {
              leftEdge = sib->loc;
            }

            // pop 'parser' and move to the next one
            StackNode *next = sib->sib;           // grab before deallocating
            next->incRefCt();                     // so 'next' survives deallocation of 'sib'
            xassert(next->referenceCount==2);     // 'sib' and the fake one
            xassert(parser->referenceCount==1);
            parser->decRefCt();                   // deinit 'parser', dealloc 'sib'
            parser = next;
            xassert(parser->referenceCount==1);   // fake refct only
          }

          TRSPARSE("state " << startStateId <<
                   ", (unambig) reducing by production " << prodIndex <<
                   " (rhsLen=" << rhsLen <<
                   "), back to state " << parser->state);

          // call the user's action function (TREEBUILD)
          SemanticValue sval = userAct->doReductionAction(
                                 prodIndex, toPass.getArray(), leftEdge);
          D(trsSval << "reduced via production " << pcs.prodIndex
                    << ", yielding " << sval << endl);

          // now, push a new state; essentially, shift prodInfo.lhsIndex.
          // do "glrShiftNonterminal(parser, prodInfo.lhsIndex, sval, leftEdge);",
          // except avoid interacting with the worklists

          // this is like a shift -- we need to know where to go; the
          // 'goto' table has this information
          StateId newState = tables->decodeGoto(
            tables->gotoEntry(parser->state, prodInfo.lhsIndex));

          // debugging
          TRSPARSE("state " << parser->state <<
                   ", (unambig) shift nonterm " << (int)prodInfo.lhsIndex <<
                   ", to state " << newState);
          
          // 'parser' has refct 1, reflecting the local variable only
          xassert(parser->referenceCount==1);

          // push new state
          StackNode *newNode = makeStackNode(newState);
          newNode->addSiblingLink(parser, sval, leftEdge);
          xassert(parser->referenceCount==2);
          parser->decRefCt();                      // local variable "parser" about to go out of scope

          // replace whatever is in 'activeParsers[0]' with 'newNode'
          activeParsers[0] = newNode;
          newNode->incRefCt();
          xassert(newNode->referenceCount == 1);   // activeParsers[0] is referrer

          // after all this, we haven't shift any tokens, so the token
          // context remains; let's go back and try to keep acting
          // determinstically (if at some point we can't be deterministic,
          // then we drop into full GLR)
          goto tryDeterministic;
        }
      }

      else {
        // error or ambig; not deterministic
      }
    }   

    // if we get here, we're dropping into the nondeterministic GLR
    // algorithm in its full glory
    cout << "not deterministic\n";

    // ([GLR] called the code from here to the end of
    // the loop 'parseword')

    // paranoia
    xassert(pendingShifts.length() == 0);

    // put active parser tops into a worklist
    decParserList(parserWorklist);      // about to empty the list
    //parserWorklist = activeParsers;
    {
      parserWorklist.empty();
      for (int i=0; i < activeParsers.length(); i++) {
        parserWorklist.push(activeParsers[i]);
      }
    }
    incParserList(parserWorklist);

    // work through the worklist
    RCPtr<StackNode> lastToDie = NULL;
    while (parserWorklist.isNotEmpty()) {
      RCPtr<StackNode> parser = parserWorklist.pop();     // dequeue
      parser->decRefCt();     // no longer on worklist

      // to count actions, first record how many parsers we have
      // before processing this one
      int parsersBefore = parserWorklist.length() + pendingShifts.length();

      // process this parser
      ActionEntry action =      // consult the 'action' table
        tables->actionEntry(parser->state, currentTokenClass->termIndex);
      glrParseAction(parser, action, pendingShifts);

      // now observe change -- normal case is we now have one more
      int actions = (parserWorklist.length() + pendingShifts.length()) -
                      parsersBefore;

      if (actions == 0) {
        TRSPARSE("parser in state " << parser->state << " died");
        lastToDie = parser;          // for reporting the error later if necessary
      }
      else if (actions > 1) {
        TRSPARSE("parser in state " << parser->state <<
                 " split into " << actions << " parsers");
      }
    }

    // process all pending shifts
    glrShiftTerminals(pendingShifts);

    // if all active parsers have died, there was an error
    if (activeParsers.isEmpty()) {
      cout << "Line " << currentToken->loc.line
           << ", col " << currentToken->loc.col
           << ": Parse error at " 
           << currentToken->toStringType(false /*sexp*/, 
                                         (Lexer2TokenType)classifiedType) 
           << endl;

      if (lastToDie == NULL) {
        // I'm not entirely confident it has to be nonnull..
        cout << "what the?  lastToDie is NULL??\n";
      }
      else {
        // print out the context of that parser
        cout << "last parser (state " << lastToDie->state << ") to die had:\n"
             << "  sample input: " 
             << sampleInput(getItemSet(lastToDie->state)) << "\n"
             << "  left context: " 
             << leftContextString(getItemSet(lastToDie->state)) << "\n";
      }

      return false;
    }
  }

  traceProgress() << "done parsing\n";
  trsParse << "Parse succeeded!\n";


  // finish the parse by reducing to start symbol
  if (activeParsers.length() != 1) {
    cout << "parsing finished with more than one active parser!\n";
    return false;
  }
  StackNode *last = activeParsers.top();

  // pull out the semantic values; this assumes the start symbol
  // always looks like "Start -> Something EOF"; it also assumes
  // the top of the tree is unambiguous
  SemanticValue arr[2];
  StackNode *nextToLast = last->leftSiblings.first()->sib;
  arr[0] = grabTopSval(nextToLast);   // Something's sval
  arr[1] = grabTopSval(last);         // eof's sval

  // reduce
  trsSval << "handing toplevel svals " << arr[0]
          << " and " << arr[1] << " top start's reducer\n";
  treeTop = userAct->doReductionAction(
              getItemSet(last->state)->getFirstReduction()->prodIndex, arr,
              last->leftSiblings.firstC()->loc);


  #if 0
  // print the parse as a graph for my graph viewer
  // (the slight advantage to printing the graph first is that
  // if there is circularity in the parse tree, the graph
  // printer will handle it ok, whereas thet tree printer will
  // get into an infinite loop (so at least the graph will be
  // available in a file after I kill the process))
  if (tracingSys("parse-graph")) {      // profiling says this is slow
    traceProgress() << "writing parse graph...\n";
    writeParseGraph("parse.g");
  }


  // print parse tree in ascii
  TreeNode const *tn = getParseTree();
  if (tracingSys("parse-tree")) {
    tn->printParseTree(trace("parse-tree") << endl, 2 /*indent*/, false /*asSexp*/);
  }


  // generate an ambiguity report
  if (tracingSys("ambiguities")) {
    tn->ambiguityReport(trace("ambiguities") << endl);
  }
  #endif // 0

  StackNode::printAllocStats();

  return true;
}


// add a new parser to the 'activeParsers' list, maintaing
// related invariants
void GLR::addActiveParser(StackNode *parser)
{
  activeParsers.push(parser);
  parser->incRefCt();

  // I implemented this index, and then discovered it made no difference
  // (actually, slight degradation) in performance; so for now it will 
  // be an optional design choice, off by default
  #ifdef USE_PARSER_INDEX
    // fill in the state id index; if the assertion here ever fails, it
    // means there are more than 255 active parsers; either the grammer
    // is highly ambiguous by mistake, or else ParserIndexEntry needs to
    // be re-typedef'd to something bigger than 'char'
    int index = activeParsers.length()-1;   // index just used
    xassert(index < INDEX_NO_PARSER);

    xassert(parserIndex[parser->state] == INDEX_NO_PARSER);
    parserIndex[parser->state] = index;
  #endif // USE_PARSER_INDEX
}


// do the actions for this parser, on input 'token'; if a shift
// is required, we postpone it by adding the necessary state
// to 'pendingShifts'; returns number of actions performed
// ([GLR] called this 'actor')
int GLR::glrParseAction(StackNode *parser, ActionEntry action,
                        ArrayStack<PendingShift> &pendingShifts)
{
  if (tables->isShiftAction(action)) {
    // shift
    StateId destState = tables->decodeShift(action);
    // add (parser, shiftDest) to pending-shifts
    pendingShifts.pushAlt().init(parser, destState);

    // debugging
    //trace("parse")
    //  << "moving to state " << state
    //  << " after shifting symbol " << symbol->name << endl;

    return 1;
  }

  else if (tables->isReduceAction(action)) {
    // reduce
    int prodIndex = tables->decodeReduce(action);
    doReduction(parser, NULL /*mustUseLink*/, prodIndex);

    // debugging
    //trace("parse")
    //  << "moving to state " << state
    //  << " after reducing by rule id " << prodIndex << endl;

    return 1;
  }

  else if (tables->isErrorAction(action)) {
    // error
    //trace("parse")
    //  << "no actions defined for symbol " << symbol->name
    //  << " in state " << state << endl;
    return 0;
  }

  else {
    // conflict
    //trace("parse")
    //  << "conflict for symbol " << symbol->name
    //  << " in state " << state
    //  << "; possible actions:\n";

    // get actions
    int ambigId = tables->decodeAmbigAction(action);
    ActionEntry *entry = tables->ambigAction[ambigId];

    // do each one
    for (int i=0; i<entry[0]; i++) {
      glrParseAction(parser, entry[i+1], pendingShifts);
    }

    return entry[0];    // # actions
  }
}


// this is basically the same as glrParseAction, except we only
// look at reductions, and those reductions have to use 'sibLink';
// I don't fold this into the code above because above is in the
// critical path whereas this typically won't be
void GLR::doAllPossibleReductions(StackNode *parser, ActionEntry action,
                                  SiblingLink *sibLink)
{
  if (tables->isShiftAction(action)) {
    // do nothing
  }
  else if (tables->isReduceAction(action)) {
    // reduce
    int prodIndex = tables->decodeReduce(action);
    doReduction(parser, sibLink, prodIndex);
  }
  else if (tables->isErrorAction(action)) {
    // don't think this can happen
    breaker();
  }
  else {
    // ambiguous; check for reductions
    int ambigId = tables->decodeAmbigAction(action);
    ActionEntry *entry = tables->ambigAction[ambigId];
    for (int i=0; i<entry[0]; i++) {
      doAllPossibleReductions(parser, entry[i+1], sibLink);
    }
  }
}


// mustUseLink: if non-NULL, then we only want to consider
// reductions that use that link
void GLR::doReduction(StackNode *parser,
                      SiblingLink *mustUseLink,
                      int prodIndex)
{
  ParseTables::ProdInfo const &info = tables->prodInfo[prodIndex];
  int rhsLen = info.rhsLen;
  xassert(rhsLen >= 0);    // paranoia before using this to control recursion

  // in ordinary LR parsing, at this point we would pop 'rhsLen'
  // symbols off the stack, and push the LHS of this production.
  // here, we search down the stack for the node 'sibling' that
  // would be at the top after popping, and then tack on a new
  // StackNode after 'sibling' as the new top of stack

  // however, there might be more than one 'sibling' node, so we
  // must process all candidates.  the strategy will be to do a
  // simple depth-first search

  // WRONG:
    // (note that each such candidate defines a unique path -- all
    // paths must be composed of the same symbol sequence (namely the
    // RHS symbols), and if more than one such path existed it would
    // indicate a failure to collapse and share somewhere)

  // CORRECTION:
    // there *can* be more than one path to a given candidate; see
    // the comments at the top

  // step 1: collect all paths of length 'rhsLen' that start at
  // 'parser', via DFS
  //PathCollectionState pcs(prodIndex, rhsLen, parser->state);
  if (pcsStackHeight == pcsStack.length()) {
    // need a new pcs instance, since the existing ones are being
    // used by my (recursive) callers
    pcsStack.push(new PathCollectionState);     // done very rarely
  }
  PathCollectionState &pcs = *( pcsStack[pcsStackHeight++] );
  try {
    pcs.init(prodIndex, rhsLen, parser->state);
    collectReductionPaths(pcs, rhsLen, parser, mustUseLink);

    // invariant: poppedSymbols' length is equal to the recursion
    // depth in 'popStackSearch'; thus, should be empty now
    // update: since it's now an array, this is implicitly true
    //xassert(pcs.poppedSymbols.isEmpty());

    // terminology:
    //   - a 'reduction' is a grammar rule plus the TreeNode children
    //     that constitute the RHS
    //   - a 'path' is a reduction plus the parser (StackNode) that is
    //     at the end of the sibling link path (which is essentially the
    //     parser we're left with after "popping" the reduced symbols)

    // step 2: process those paths
    // ("mutate" because need non-const access to rpath->finalState)
    for (int i=0; i < pcs.paths.length(); i++) {
      PathCollectionState::ReductionPath &rpath = pcs.paths[i];

      // I'm not sure what is the best thing to call an 'action' ...
      //actions++;

      // this is like shifting the reduction's LHS onto 'finalParser'
      glrShiftNonterminal(rpath.finalState, info.lhsIndex,
                          rpath.sval, rpath.loc);

      // nullify 'sval' to mark it as consumed
      rpath.sval = NULL;
    } // for each path
  }
  
  // make sure the height gets propely decremented in any situation,
  // and deinit the pcs; this is where reduction paths get removed
  catch (...) {
    pcsStackHeight--;
    pcs.deinit();
    throw;
  }
  pcsStackHeight--;
  pcs.deinit();
}


/*
 * collectReductionPaths():
 *   this function searches for all paths (over the
 *   sibling links) of a particular length, starting
 *   at currentNode
 *
 * pcs:
 *   various search state elements; see glr.h
 *
 * popsRemaining:
 *   number of links left to traverse before we've popped
 *   the correct number
 *
 * currentNode:
 *   where we are in the search; this call will consider the
 *   siblings of currentNode
 *
 * mustUseLink:
 *   a particular sibling link that must appear on all collected
 *   paths; it is NULL if we have no such requirement
 *
 * ([GLR] called this 'do-reductions' and 'do-limited-reductions')
 */
void GLR::collectReductionPaths(PathCollectionState &pcs, int popsRemaining,
                                StackNode *currentNode, SiblingLink *mustUseLink)
{
  // inefficiency: all this mechanism is somewhat overkill, given that
  // most of the time the reductions are unambiguous and unrestricted

  if (popsRemaining == 0) {
    // we've found a path of the required length

    // if we have failed to use the required link, ignore this path
    if (mustUseLink != NULL) {
      return;
    }

    // info about the production
    ParseTables::ProdInfo const &prodInfo = tables->prodInfo[pcs.prodIndex];
    int rhsLen = prodInfo.rhsLen;

    TRSPARSE("state " << pcs.startStateId <<
             ", reducing by production " << pcs.prodIndex <<
             " (rhsLen=" << rhsLen <<
             "), back to state " << currentNode->state);

    // record location of left edge
    SourceLocation leftEdge;     // defaults to no location (used for epsilon rules)

    // before calling the user, duplicate any needed values
    //SemanticValue *toPass = new SemanticValue[rhsLen];
    toPass.ensureIndexDoubler(rhsLen-1);
    for (int i=0; i < rhsLen; i++) {
      SiblingLink *sib = pcs.siblings[i];

      // we're about to yield sib's 'sval' to the reduction action
      toPass[i] = sib->sval;

      // left edge?  or, have all previous tokens failed to yield
      // information?
      if (!leftEdge.validLoc()) {
        leftEdge = sib->loc;
      }

      // we inform the user, and the user responds with a value
      // to be kept in this sibling link *instead* of the passed
      // value; if this link yields a value in the future, it will
      // be this replacement
      sib->sval = duplicateSemanticValue(pcs.symbols[i], sib->sval);

      D(trsSval << "dup'd " << toPass[i] << " to pass, yielding "
                << sib->sval << " to store" << endl);
    }

    // we've popped the required number of symbols; call the
    // user's code to synthesize a semantic value by combining them
    // (TREEBUILD)
    SemanticValue sval = userAct->doReductionAction(
                           pcs.prodIndex, toPass.getArray(), leftEdge);
    D(trsSval << "reduced via production " << pcs.prodIndex
              << ", yielding " << sval << endl);
    //delete[] toPass;

    // see if the user wants to keep this reduction
    if (!userAct->keepNontermValue(prodInfo.lhsIndex, sval)) {
      D(trsSval << "but the user decided not to keep it" << endl);
      return;
    }

    // and just collect this reduction, and the final state, in our
    // running list
    pcs.addReductionPath(currentNode, sval, leftEdge);
  }

  else {
    // explore currentNode's siblings
    MUTATE_EACH_OBJLIST(SiblingLink, currentNode->leftSiblings, sibling) {
      // the symbol on the sibling link is being popped;
      // TREEBUILD: we are collecting 'treeNode' for the purpose
      // of handing it to the user's reduction routine
      pcs.siblings[popsRemaining-1] = sibling.data();
      pcs.symbols[popsRemaining-1] = currentNode->getSymbolC();

      // recurse one level deeper, having traversed this link
      if (sibling.data() == mustUseLink) {
        // consumed the mustUseLink requirement
        collectReductionPaths(pcs, popsRemaining-1, sibling.data()->sib,
                              NULL /*mustUseLink*/);
      }
      else {
        // either no requirement, or we didn't consume it; just
        // propagate current requirement state
        collectReductionPaths(pcs, popsRemaining-1, sibling.data()->sib,
                              mustUseLink);
      }
    }
  }
}


#if 0
// temporary
void *mergeAlternativeParses(int ntIndex, void *left, void *right)
{
  cout << "merging at ntIndex " << ntIndex << ": "
       << left << " and " << right << endl;

  // just pick one arbitrarily
  return left;
}
#endif // 0


// shift reduction onto 'leftSibling' parser, 'prod' says which
// production we're using; 'sval' is the semantic value of this
// subtree, and 'loc' is the location of the left edge
// ([GLR] calls this 'reducer')
void GLR::glrShiftNonterminal(StackNode *leftSibling, int lhsIndex,
                              SemanticValue /*owner*/ sval,
                              SourceLocation const &loc)
{
  // this is like a shift -- we need to know where to go; the
  // 'goto' table has this information
  StateId rightSiblingState = tables->decodeGoto(
    tables->gotoEntry(leftSibling->state, lhsIndex));

  // debugging
  TRSPARSE("state " << leftSibling->state <<
           ", shift nonterm " << lhsIndex <<
           ", to state " << rightSiblingState);

  // is there already an active parser with this state?
  StackNode *rightSibling = findActiveParser(rightSiblingState);
  if (rightSibling) {
    // does it already have a sibling link to 'leftSibling'?
    MUTATE_EACH_OBJLIST(SiblingLink, rightSibling->leftSiblings, sibIter) {
      SiblingLink *sibLink = sibIter.data();

      // check this link for pointing at the right place
      if (sibLink->sib == leftSibling) {
        // we already have a sibling link, so we don't need to add one

        // +--------------------------------------------------+
        // | it is here that we are bringing the tops of two  |
        // | alternative parses together (TREEBUILD)          |
        // +--------------------------------------------------+
        // call the user's code to merge, and replace what we have
        // now with the merged version
        D(SemanticValue old = sibLink->sval);
        sibLink->sval = userAct->mergeAlternativeParses(lhsIndex,
                                                        sibLink->sval, sval);
        D(trsSval << "merged " << old << " and " << sval
                  << ", yielding " << sibLink->sval << endl);

        // ok, done
        return;

        // and since we didn't add a link, there is no potential for new
        // paths
      }
    }

    // we get here if there is no suitable sibling link already
    // existing; so add the link (and keep the ptr for loop below)
    SiblingLink *sibLink =
      rightSibling->addSiblingLink(leftSibling, sval, loc);

    // adding a new sibling link may have introduced additional
    // opportunties to do reductions from parsers we thought
    // we were finished with.
    //
    // what's more, it's not just the parser ('rightSibling') we
    // added the link to -- if rightSibling's itemSet contains 'A ->
    // alpha . B beta' and B ->* empty (so A's itemSet also has 'B
    // -> .'), then we reduced it (if lookahead ok), so
    // 'rightSibling' now has another left sibling with 'A -> alpha
    // B . beta'.  We need to let this sibling re-try its reductions
    // also.
    //
    // so, the strategy is to let all 'finished' parsers re-try
    // reductions, and process those that actually use the just-
    // added link

    // TODO: I think this code path is unusual; confirm by measurement
    // update: it's taken maybe 1 in 10 times through this function..
    //cout << "got here\n";

    // for each 'finished' parser (i.e. those not still on
    // the worklist)
    for (int i=0; i < activeParsers.length(); i++) {
      StackNode *parser = activeParsers[i];
      if (parserListContains(parserWorklist, parser)) continue;

      // do any reduce actions that are now enabled
      ActionEntry action =
        tables->actionEntry(parser->state, currentTokenClass->termIndex);
      doAllPossibleReductions(parser, action, sibLink);
    }
  }

  else {
    // no, there is not already an active parser with this
    // state.  we must create one; it will become the right
    // sibling of 'leftSibling'
    rightSibling = makeStackNode(rightSiblingState);

    // add the sibling link (and keep ptr for tree stuff)
    rightSibling->addSiblingLink(leftSibling, sval, loc);

    // since this is a new parser top, it needs to become a
    // member of the frontier
    addActiveParser(rightSibling);

    parserWorklist.push(rightSibling);
    rightSibling->incRefCt();

    // no need for the elaborate re-checking above, since we
    // just created rightSibling, so no new opportunities
    // for reduction could have arisen
  }
}


void GLR::glrShiftTerminals(ArrayStack<PendingShift> &pendingShifts)
{
  // clear the active-parsers list; we rebuild it in this fn
  for (int i=0; i < activeParsers.length(); i++) {
    StackNode *parser = activeParsers[i];
    #ifdef USE_PARSER_INDEX
      xassert(parserIndex[parser->state] == i);
      parserIndex[parser->state] = INDEX_NO_PARSER;
    #endif // USE_PARSER_INDEX
    parser->decRefCt();
  }
  activeParsers.empty();

  // foreach (leftSibling, newState) in pendingShifts
  while (pendingShifts.isNotEmpty()) {
    RCPtr<StackNode> leftSibling = pendingShifts.top().parser;
    StateId newState = pendingShifts.top().shiftDest;
    pendingShifts.popAlt().deinit();

    // debugging
    TRSPARSE("state " << leftSibling->state <<
             ", shift token " << currentTokenClass->name <<
             ", to state " << newState);

    // if there's already a parser with this state
    StackNode *rightSibling = findActiveParser(newState);
    if (rightSibling != NULL) {
      // no need to create the node
    }

    else {
      // must make a new stack node
      rightSibling = makeStackNode(newState);

      // and add it to the active parsers
      addActiveParser(rightSibling);
    }

    // either way, add the sibling link now
    D(trsSval << "grabbed token sval " << currentTokenValue << endl);
    rightSibling->addSiblingLink(leftSibling, currentTokenValue,
                                 currentTokenLoc);
  }
}


// if an active parser is at 'state', return it; otherwise
// return NULL
StackNode *GLR::findActiveParser(StateId state)
{
  #ifdef USE_PARSER_INDEX
    int index = parserIndex[state];
    if (index != INDEX_NO_PARSER) {
      return activeParsers[index];
    }
    else {
      return NULL;
    }
  #else
    for (int i=0; i < activeParsers.length(); i++) {
      StackNode *node = activeParsers[i];
      if (node->state == state) {
        return node;
      }
    }
    return NULL;
  #endif
}


StackNode *GLR::makeStackNode(StateId state)
{
  StackNode *sn = stackNodePool.alloc();
  sn->init(nextStackNodeId++, currentTokenColumn, state, this);
  return sn;
}


#if 0
SemanticValue GLR::getParseResult()
{
  // the final activeParser is the one that shifted the end-of-stream
  // marker, so we want its left sibling, since that will be the
  // reduction(s) to the start symbol
  SemanticValue sv =
    activeParsers.first()->                    // parser that shifted end-of-stream
      leftSiblings.first()->sib->              // parser that shifted start symbol
      leftSiblings.first()->                   // sibling link with start symbol
      sval;                                    // start symbol tree node

  return sv;
}    
#endif // 0


// ------------------ stuff for outputting raw graphs ------------------
#if 0   // disabled for now
// name for graphs (can't have any spaces in the name)
string stackNodeName(StackNode const *sn)
{     
  Symbol const *s = sn->getSymbolC();
  char const *symName = (s? s->name.pcharc() : "(null)");
  return stringb(sn->stackNodeId
              << ":col="  << sn->tokenColumn
              << ",st=" << sn->state->id
              << ",sym=" << symName);
}

// name for rules; 'rn' is the 'ruleNo'-th rule in 'sn'
// (again, no spaces allowed)
string reductionName(StackNode const *sn, int ruleNo, Reduction const *red)
{
  return stringb(sn->stackNodeId << "/" << ruleNo << ":"
              << replace(red->production->toString(), " ", "_"));
}                                                            


// this prints the graph in my java graph applet format, where
// nodes lines look like
//   n <name> <optional-desc>
// and edges look like
//   e <from> <to>
// unfortunately, the graph applet needs a bit of work before it
// is worthwhile to use this routinely (though it's great for
// quickly verifying a single (small) parse)
//
// however, it's worth noting that the text output is not entirely
// unreadable...
void GLR::writeParseGraph(char const *fname) const
{
  FILE *out = fopen(stringb("graphs/" << fname), "w");
  if (!out) {
    xsyserror("fopen", stringb("opening file `graphs/" << fname << "'"));
  }

  // header info
  fprintf(out, "# parse graph file: %s\n", fname);
  fprintf(out, "# automatically generated\n"
               "\n");

  #if 0    // can't do anymore because allStackNodes is gone ...
  // for each stack node
  FOREACH_OBJLIST(StackNode, allStackNodes, stackNodeIter) {
    StackNode const *stackNode = stackNodeIter.data();
    string myName = stackNodeName(stackNode);

    // visual delimiter
    fputs(stringb("\n# ------ node: " << myName << " ------\n"), out);

    // write info for the node itself
    fputs(stringb("n " << myName << "\n\n"), out);

    // for all sibling links
    int ruleNo=0;
    FOREACH_OBJLIST(SiblingLink, stackNode->leftSiblings, sibIter) {
      SiblingLink const *link = sibIter.data();

      // write the sibling link
      fputs(stringb("e " << myName << " "
                         << stackNodeName(link->sib) << "\n"), out);

      // ideally, we'd attach the reduction nodes directly to the
      // sibling edge.  however, since I haven't developed the
      // graph applet far enough for that, I'll instead attach it
      // to the stack node directly..

      if (link->treeNode->isNonterm()) {
        // for each reduction node
        FOREACH_OBJLIST(Reduction, link->treeNode->asNonterm().reductions,
                        redIter) {
          Reduction const *red = redIter.data();
          ruleNo++;

          string ruleName = reductionName(stackNode, ruleNo, red);

          // write info for the rule node
          fputs(stringb("n " << ruleName << "\n"), out);

          // put the link from the stack node to the rule node
          fputs(stringb("e " << myName << " " << ruleName << "\n"), out);

          // write all child links
          // ACK!  until my graph format is better, this is almost impossible
          #if 0
          SFOREACH_OBJLIST(StackNode, rule->children, child) {
            fputs(stringb("e " << ruleName << " "
                               << stackNodeName(child.data()) << "\n"), out);
          }
          #endif // 0

          // blank line for visual separation
          fputs("\n", out);
        } // for each reduction
      } // if is nonterminal
    } // for each sibling
  } // for each stack node
  #endif // 0

  // done
  if (fclose(out) != 0) {
    xsyserror("fclose");
  }
}
#endif // 0


// --------------------- testing ------------------------
// read an entire file into a single string
// currenty is *not* pipe-friendly because it must seek
// (candidate for adding to 'str' module)
string readFileIntoString(char const *fname)
{
  // open file
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    xsyserror("fopen", stringb("opening `" << fname << "' for reading"));
  }

  // determine file's length
  if (fseek(fp, 0, SEEK_END) < 0) {
    xsyserror("fseek");
  }
  int len = (int)ftell(fp);      // conceivably problematic cast..
  if (len < 0) {
    xsyserror("ftell");
  }
  if (fseek(fp, 0, SEEK_SET) < 0) {
    xsyserror("fseek");
  }

  // allocate a sufficiently large buffer
  string ret(len);
  
  // read the file into that buffer
  if (fread(ret.pchar(), 1, len, fp) < (size_t)len) {
    xsyserror("fread");
  }

  // close file
  if (fclose(fp) < 0) {
    xsyserror("fclose");
  }

  // return the new string
  return ret;
}


bool GLR::glrParseFrontEnd(Lexer2 &lexer2, SemanticValue &treeTop,
                           char const *grammarFname, char const *inputFname)
{
  #if 0
    // [ASU] grammar 4.19, p.222: demonstrating LR sets-of-items construction
    parseLine("E' ->  E $                ");
    parseLine("E  ->  E + T  |  T        ");
    parseLine("T  ->  T * F  |  F        ");
    parseLine("F  ->  ( E )  |  id       ");

    char const *input[] = {
      " id                 $",
      " id + id            $",
      " id * id            $",
      " id + id * id       $",
      " id * id + id       $",
      " ( id + id ) * id   $",
      " id + id + id       $",
      " id + ( id + id )   $"
    };
  #endif // 0/1

 #if 0
    // a simple ambiguous expression grammar
    parseLine("S  ->  E  $                  ");
    parseLine("E  ->  x  |  E + E  | E * E  ");

    char const *input[] = {
      "x + x * x $",
    };
  #endif // 0/1

  #if 0
    // my cast-problem grammar
    parseLine("Start -> Expr $");
    parseLine("Expr -> CastExpr");
    parseLine("Expr -> Expr + CastExpr");
    parseLine("CastExpr -> AtomExpr");
    parseLine("CastExpr -> ( Type ) CastExpr");
    parseLine("AtomExpr -> 3");
    parseLine("AtomExpr -> x");
    parseLine("AtomExpr -> ( Expr )");
    parseLine("AtomExpr -> AtomExpr ( Expr )");
    parseLine("Type -> int");
    parseLine("Type -> x");

    char const *input[] = {
      "( x ) ( x ) $",
    };

    printProductions(cout);
    runAnalyses();

    INTLOOP(i, 0, (int)TABLESIZE(input)) {
      cout << "------ parsing: `" << input[i] << "' -------\n";
      glrParse(input[i]);
    }
  #endif // 0/1

  #if 1
    // read grammar
    //if (!readFile(grammarFname)) {
    //  // error with grammar; message already printed
    //  return;
    //}

    if (!strstr(grammarFname, ".bin")) {
      // assume it's an ascii grammar file and do the whole thing

      // new code to read a grammar (throws exception on failure)
      readGrammarFile(*this, grammarFname);

      // spit grammar out as something bison might be able to parse
      //{
      //  ofstream bisonOut("bisongr.y");
      //  printAsBison(bisonOut);
      //}

      if (tracingSys("grammar")) {
        printProductions(trace("grammar") << endl);
      }

      runAnalyses(NULL);
    }

    else {
      // before using 'xfer' we have to tell it about the string table
      flattenStrTable = &grammarStringTable;

      // assume it's a binary grammar file and try to
      // read it in directly
      traceProgress() << "reading binary grammar file " << grammarFname << endl;
      BFlatten flat(grammarFname, true /*reading*/);
      xfer(flat);
    }


    // parse input
    return glrParseNamedFile(lexer2, treeTop, inputFname);

  #endif // 0/1
}


#ifdef GLR_MAIN

int main(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  ParseTreeAndTokens tree;
  treeMain(tree, argc, argv);

  return 0;
}
#endif // GLR_MAIN
