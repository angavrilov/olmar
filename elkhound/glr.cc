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
 * global because they are needed deep inside of 'glrShiftRule',
 * though they are not needed by the intervening levels, and their
 * presence in the argument lists would therefore only clutter them.
 *
 * It should be clear that many factors contribute to this
 * implementation being slow, and I'm going to refrain from any
 * optimization for a bit.
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
 *
 *   pendingShifts
 *   -------------
 *   The GLR parser alternates between reducing and shifting.
 *   During the processing of a given token, shifting happens last.
 *   This keeps the parsers synchronized, since every token is
 *   shifted by every parser at the same time.  This synchronization
 *   is important because it makes merging parsers possible.
 *
 */


#include "glr.h"         // this module
#include "strtokp.h"     // StrtokParse
#include "syserr.h"      // xsyserror

#include <stdio.h>       // FILE


// convenient indented output
ostream &doIndent(ostream &os, int i)
{
  while (i--) {
    os << " ";
  }   
  return os;
}

#define IND doIndent(cout, indent)


// ----------------------- StackNode -----------------------
StackNode::StackNode(int id, int col, ItemSet const *st, Symbol const *sym)
  : stackNodeId(id),
    tokenColumn(col),
    state(st),
    symbol(sym)
{}

StackNode::~StackNode()
{}


void StackNode::printParseTree(int indent) const
{
  if (rules.isEmpty()) {
    // I am a leaf
    IND << symbol->name << endl;
  }

  else if (rules.count() == 1) {
    // I am an unambiguous rule node
    rules.nthC(0)->printParseTree(indent);
  }

  else {
    // I am an ambiguous rule node
    IND << "ALTERNATIVE PARSES for nonterminal " << symbol->name << ":\n";
    indent += 2;

    FOREACH_OBJLIST(RuleNode, rules, rule) {
      rule.data()->printParseTree(indent);
    }
  }
}


// ---------------------- RuleNode -------------------------
RuleNode::RuleNode(Production const *prod)
  : production(prod)
{}


RuleNode::~RuleNode()
{}


void RuleNode::printParseTree(int indent) const
{
  IND << *(production) << endl;
              
  // print children
  indent += 2;     
  SFOREACH_OBJLIST(StackNode, children, child) {
    child.data()->printParseTree(indent);
  }
}


// ------------------------- GLR ---------------------------
GLR::GLR()
  : currentToken(NULL)
  // some fields initialized by 'clearAllStackNodes'
{}

GLR::~GLR()
{}


void GLR::clearAllStackNodes()
{
  // throw away any parse nodes leftover from previous parses
  allStackNodes.deleteAll();
  nextStackNodeId = initialStackNodeId;
  currentTokenColumn = 0;
}


// process the input string, and yield a parse graph
void GLR::glrParse(char const *input)
{
  // tokenize the input
  StrtokParse tok(input, " \t\r\n");

  // get ready..
  clearAllStackNodes();

  // create an initial ParseTop with grammar-initial-state,
  // set active-parsers to contain just this
  StackNode *first = makeStackNode(itemSets.nth(0), NULL);
  activeParsers.append(first);

  // for each input symbol
  INTLOOP(t, 0, tok) {
    // debugging
    cout << "processing token " << tok[t]
         << ", # active parsers = " << activeParsers.count()
         << endl;

    // convert the token to a symbol
    currentToken = findTerminalC(tok[t]);
    currentTokenColumn = t;

    // ([GLR] called the code from here to the end of
    // the loop 'parseword')

    // we will queue up shifts and process them all
    // at the end
    ObjList<PendingShift> pendingShifts;                  // starts empty

    // put active parser tops into a worklist
    parserWorklist = activeParsers;

    // work through the worklist
    while (parserWorklist.isNotEmpty()) {
      StackNode *parser = parserWorklist.removeAt(0);     // dequeue
      glrParseAction(parser, pendingShifts);
    }

    // process all pending shifts
    glrShiftTerminals(pendingShifts);

    // if all active parsers have died, there was an error
    if (activeParsers.isEmpty()) {
      cout << "Parse error at " << tok[t] << endl;
      return;
    }
  }


  cout << "Parse succeeded!\n";


  // print the parse as a graph for my graph viewer
  // (the slight advantage to printing the graph first is that
  // if there is circularity in the parse tree, the graph
  // printer will handle it ok, whereas thet tree printer will
  // get into an infinite loop.)
  writeParseGraph(input);


  // print parse tree in ascii; the final activeParser is the one
  // that shifted the end-of-stream marker, so we want its left
  // sibling, since that will be the reduction(s) to the start
  // symbol
  activeParsers.nthC(0)->                // end-of-stream
    leftAdjStates.nthC(0)->              // start symbol
    printParseTree(2 /*indent*/);
}


// do the actions for this parser, on input 'token'; if a shift
// is required, we postpone it by adding the necessary state
// to 'pendingShifts'
// ([GLR] called this 'actor')
void GLR::glrParseAction(StackNode *parser,
                         ObjList<PendingShift> &pendingShifts)
{
  postponeShift(parser, pendingShifts);

  doAllPossibleReductions(parser, NULL /*no link restrictions*/);
}


void GLR::postponeShift(StackNode *parser,
                        ObjList<PendingShift> &pendingShifts)
{
  // see where a shift would go
  ItemSet const *shiftDest = parser->state->transitionC(currentToken);
  if (shiftDest != NULL) {
    // postpone until later; save necessary state (the
    // parser and the state to transition to)

    // add (parser, shiftDest) to pending-shifts
    pendingShifts.append(new PendingShift(parser, shiftDest));
  }
}


// mustUseLink: if non-NULL, then we only want to consider
// reductions that use that link
void GLR::doAllPossibleReductions(StackNode *parser,
                                  SiblingLinkDesc *mustUseLink)
{
  // get all possible reductions where 'currentToken' is in Follow(LHS)
  ProductionList reductions;
  parser->state->getPossibleReductions(reductions, currentToken);

  // for each possible reduction, do it
  SFOREACH_PRODUCTION(reductions, prod) {
    int rhsLen = prod.data()->rhsLength();
    xassert(rhsLen >= 0);    // paranoia before using this to control recursion

    // in ordinary LR parsing, at this point we would pop 'rhsLen'
    // symbols off the stack, and push the LHS of this production.
    // here, we search down the stack for the node 'sibling' that
    // would be at the top after popping, and then tack on a new
    // StackNode after 'sibling' as the new top of stack

    // however, there might be more than one 'sibling' node, so
    // we must process all candidates.  (note that each such
    // candidate defines a unique path -- all paths must be composed
    // of the same symbol sequence (namely the RHS symbols), and
    // if more than one such path existed it would represent a
    // failure to collapse and share somewhere)

    // so, the strategy will be to do a simple depth-first search
    SObjList<StackNode> poppedSymbols;     // state during recursion
    popStackSearch(rhsLen, poppedSymbols, parser, prod.data(),
                   mustUseLink);

    // invariant: poppedSymbols' length is equal to the recursion
    // depthin 'popStackSearch'; thus, should be empty now
    xassert(poppedSymbols.isEmpty());
  }
}


/*
 * popStackSearch():
 *   this function searches for all paths (over the
 *   sibling links) of a particular length, starting
 *   at currentNode
 *
 * popsRemaining: 
 *   number of links left to traverse before we've popped
 *   the correct number
 *
 * poppedSymbols:
 *   list of symbols popped so far; 0th is most-recently-
 *   popped
 *
 * currentNode: 
 *   where we are in the search; this call will consider the
 *   siblings of currentNode
 *
 * production: 
 *   the production being reduced
 *
 * mustUseLink: 
 *   a particular sibling link that must appear in any path
 *   we consider for reducing (explained in 'glrStackRule');
 *   if it's NULL, there is no such restriction
 *
 * ([GLR] called this 'do-reductions')
 */
void GLR::popStackSearch(int popsRemaining, SObjList<StackNode> &poppedSymbols,
                         StackNode *currentNode, Production const *production,
                         SiblingLinkDesc *mustUseLink)
{
  // inefficiency: all this mechanism is somewhat overkill, given that
  // most of the time the reductions are unambiguous and unrestricted

  if (popsRemaining == 0) {
    if (mustUseLink != NULL) {
      // we are not going to traverse any more links, but none
      // of the traversals so far have satisfied the restriction,
      // so this path must be ignored
      return;
    }

    // we've popped the required number of symbols; collect the
    // popped symbols into a RuleNode
    RuleNode *rn = new RuleNode(production);
    rn->children = poppedSymbols;
    
    // previously, I had been reversing the children here; that
    // is wrong!  since the most recent symbol prepended is the
    // leftmost one in the input, the list is in the correct order

    // this is like shifting LHS onto 'currentNode'
    glrShiftRule(currentNode, rn);
  }

  else {
    // currentNode is being popped
    poppedSymbols.prepend(currentNode);

    // explore currentNode's siblings
    SMUTATE_EACH_OBJLIST(StackNode, currentNode->leftAdjStates, sibling) {
      // does this link satisfy the restriction?
      if (mustUseLink != NULL &&
          mustUseLink->right == currentNode &&
          mustUseLink->left == sibling.data()) {
        // yes!  lift the restriction for the rest of the path
        popStackSearch(popsRemaining-1, poppedSymbols, sibling.data(), production,
                       NULL /*no restriction*/);
      }
      else {
        // either there is no restriction, or we didn't satisfy it; either
        // way carry the restriction forward
        popStackSearch(popsRemaining-1, poppedSymbols, sibling.data(), production,
                       mustUseLink /*same as before*/);
      }
    }

    // un-pop currentNode, so exploring another path will work
    poppedSymbols.removeAt(0);
  }
}


// shift ruleNode onto 'leftSibling' parser
// ([GLR] calls this 'reducer')
void GLR::glrShiftRule(StackNode *leftSibling, RuleNode *ruleNode)
{
  // this is like a shift -- we need to know where to go
  ItemSet const *rightSiblingState =
    leftSibling->state->transitionC(ruleNode->production->left);

  // debugging
  cout << "from state " << leftSibling->state->id
       << ", shifting production " << *(ruleNode->production)
       << ", transitioning to state " << rightSiblingState->id
       << endl;

  // is there already an active parser with this state?
  StackNode *rightSibling = findActiveParser(rightSiblingState);
  if (rightSibling) {
    // does it already have the sibling link?
    if (rightSibling->leftAdjStates.contains(leftSibling)) {
      // yes; don't need to add it
    }
    else {
      // no, so add the link
      rightSibling->leftAdjStates.append(leftSibling);

      // describe it (we need this below)
      SiblingLinkDesc linkDesc(rightSibling, leftSibling);

      // adding a new sibling link may have introduced additional
      // opportunties to do reductions from parsers we thought
      // we were finished with.
      //
      // what's more, it's not just the
      // parser ('rightSibling') we added the link to -- if
      // rightSibling's itemSet contains 'A -> alpha . B beta' and
      // B ->* empty (so A's itemSet also has 'B -> .'), then we
      // reduced it (if lookahead ok), so 'rightSibling' now has
      // another left sibling with 'A -> alpha B . beta'.  We need
      // to let this sibling re-try its reductions also.
      //
      // so, the strategy is to let all 'finished' parsers re-try
      // reductions, and process those that actually use the just-
      // added link

      // for each 'finished' parser (i.e. those not still on
      // the worklist)
      SMUTATE_EACH_OBJLIST(StackNode, activeParsers, parser) {
        if (parserWorklist.contains(parser.data())) continue;

        // do any reduce actions that are now enabled
        doAllPossibleReductions(parser.data(), &linkDesc);
      }
    }
  }

  else {
    // no, there is not already an active parser with this
    // state.  we must create one; it will become the right
    // sibling of 'leftSibling'
    rightSibling = makeStackNode(rightSiblingState,
                                 ruleNode->production->left);

    // add the sibling link
    rightSibling->leftAdjStates.append(leftSibling);

    // (no need for the elaborate checking above, since we
    // just created rightSibling, so no new opportunities
    // for reduction could have arisen)

    // since this is a new parser top, it needs to become a
    // member of the frontier
    activeParsers.append(rightSibling);
    parserWorklist.append(rightSibling);
  }


  // the situation now is that 'rightSibling' points to
  // 'leftSibling', and is a member of the frontier of
  // parsers.  the only remaining task is to attach the rule
  // node to 'rightSibling' to indicate the details of the
  // reduction
  rightSibling->rules.append(ruleNode);
}


void GLR::glrShiftTerminals(ObjList<PendingShift> &pendingShifts)
{      
  // clear the active-parsers list; we rebuild it in this fn
  activeParsers.removeAll();

  // foreach (leftSibling, newState) in pendingShifts
  FOREACH_OBJLIST(PendingShift, pendingShifts, pshift) {
    StackNode *leftSibling = pshift.data()->parser;
    ItemSet const *newState = pshift.data()->shiftDest;

    // debugging
    cout << "from state " << leftSibling->state->id
         << ", shifting terminal " << currentToken->name
         << ", transitioning to state " << newState->id
         << endl;

    // if there's already a parser with this state
    StackNode *rightSibling = findActiveParser(newState);
    if (rightSibling != NULL) {
      // no need to create the node

      // but, verify my assumption that all outgoing links
      // are labelled (in the [GLR] sense) with the same
      // symbol (has to, it's the same 'currentToken'!)
      xassert(rightSibling->symbol == currentToken);
    }

    else {
      // must make a new stack node
      rightSibling = makeStackNode(newState, currentToken);

      // and add it to the active parsers
      activeParsers.append(rightSibling);
    }

    // either way, add the sibling link now
    rightSibling->leftAdjStates.append(leftSibling);
  }
}


// if an active parser is at 'state', return it; otherwise
// return NULL
StackNode *GLR::findActiveParser(ItemSet const *state)
{
  SMUTATE_EACH_OBJLIST(StackNode, activeParsers, parser) {
    if (parser.data()->state == state) {
      return parser.data();
    }
  }
  return NULL;
}


StackNode *GLR::makeStackNode(ItemSet const *state, Symbol const *symbol)
{
  StackNode *sn = new StackNode(nextStackNodeId++, currentTokenColumn,
                                state, symbol);
  allStackNodes.prepend(sn);
  return sn;
}


// ------------------ stuff for outputting raw graphs ------------------
// name for graphs (can't have any spaces in the name)
string stackNodeName(StackNode const *sn)
{
  char const *symName = (sn->symbol? sn->symbol->name.pcharc() : "(null)");
  return stringb(sn->stackNodeId
              << ":col="  << sn->tokenColumn
              << ",st=" << sn->state->id
              << ",sym=" << symName);
}

// name for rules; 'rn' is the 'ruleNo'-th rule in 'sn'
// (again, no spaces allowed)
string ruleNodeName(StackNode const *sn, int ruleNo, RuleNode const *rn)
{
  return stringb(sn->stackNodeId << "/" << ruleNo << ":"
              << replace(rn->production->toString(), " ", "_"));
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
void GLR::writeParseGraph(char const *input) const
{
  // write it to a file named the same as the input
  FILE *out = fopen(stringb("graphs/" << input), "w");
  if (!out) {
    xsyserror("fopen", stringb("opening file `graphs/" << input << "'"));
  }

  // header info
  fprintf(out, "# parse graph file: %s\n", input);
  fprintf(out, "# automatically generated\n"
               "\n");

  // for each stack node
  FOREACH_OBJLIST(StackNode, allStackNodes, stackNodeIter) {
    StackNode const *stackNode = stackNodeIter.data();
    string myName = stackNodeName(stackNode);

    // visual delimiter
    fputs(stringb("\n# ------ node: " << myName << " ------\n"), out);

    // write info for the node itself
    fputs(stringb("n " << myName << "\n"), out);

    // write all sibling links
    SFOREACH_OBJLIST(StackNode, stackNode->leftAdjStates, sibling) {
      fputs(stringb("e " << myName << " "
                         << stackNodeName(sibling.data()) << "\n"), out);
    }

    if (stackNode->rules.isNotEmpty()) {
      fputs("\n# rule nodes\n", out);
    }

    // for each rule node
    int ruleNo=0;
    FOREACH_OBJLIST(RuleNode, stackNode->rules, ruleIter) {
      RuleNode const *rule = ruleIter.data();
      ruleNo++;

      string ruleName = ruleNodeName(stackNode, ruleNo, rule);

      // write info for the rule node
      fputs(stringb("n " << ruleName << "\n"), out);

      // put the link from the stack node to the rule node
      fputs(stringb("e " << myName << " " << ruleName << "\n"), out);

      // write all child links
      SFOREACH_OBJLIST(StackNode, rule->children, child) {
        fputs(stringb("e " << ruleName << " "
                           << stackNodeName(child.data()) << "\n"), out);
      }
      
      fputs("\n", out);
    }
  }

  // done
  if (fclose(out) != 0) {
    xsyserror("fclose");
  }
}


// --------------------- testing ------------------------
void GLR::glrTest()
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

  #if 1
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
  #endif // 0/1

  printProductions(cout);
  runAnalyses();

  INTLOOP(i, 0, (int)TABLESIZE(input)) {
    cout << "------ parsing: `" << input[i] << "' -------\n";
    glrParse(input[i]);
  }
}


#ifdef GLR_MAIN
int main()
{
  GLR g;
  g.glrTest();
  return 0;
}
#endif // GLR_MAIN
