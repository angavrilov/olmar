// gramexpl.cc            see license.txt for copyright and terms of use
// interactively query and modify a grammar; primary purpose
// is to assist diagnosing SLR conflict reports

#include "gramanl.h"    // GrammarAnalysis
#include "strtokp.h"    // StrtokParse

#include <iostream.h>   // cin/cout


void grammarExplorer(GrammarAnalysis &g)
{
  cout << "exploring the grammar:\n";

  #if 0
  for (;;) {
    cout << "commands:\n"
            "  terminals\n"
            "  nonterminals\n"
            "  productions <nonterm-id>\n"
            "  state <state-id>\n"
            "  suppress-except <term-id> (-1 to disable)\n"
            "  reach <state-id>\n"
            "  track-la <state-id> <prod-id> <term-id>\n"
            "  quit\n";
    cout << "command> ";
    cout.flush();

    char buf[80];
    cin >> buf;     // buffer overrun potential, don't care
    if (cin.eof()) break;

    StrtokParse tok(buf, " \n\t");
    if (tok == 0) continue;

    try {
      if (0==strcmp(tok[0], "terminals")) {
        for (int i=0; i < g.numTerminals(); i++) {
          Terminal const *t = g.getTerminal(i);
          t->print(cout);
        }
      }

      else if (0==strcmp(tok[0], "nonterminals")) {
        for (int i=0; i < g.numNonterminals(); i++) {
          Nonterminal const *nt = g.getNonterminal(i);
          nt->print(cout);
        }
      }

      else if (0==strcmp(tok[0], "productions")) {
        int id = atoi(tok[1]);
        Nonterminal const *nt = g.getNonterminal(i);
        int ct=0;
        FOREACH_PRODUCTION(g.productions, iter) {
          if (iter.data()->left == nt) {
            cout << "[" << ct << "] ";   // production id
            iter.data()->print(cout);
          }
          ct++;
        }
      }

      else if (0==strcmp(tok[0], "state")) {
        ItemSet const *is = g.getItemSet(atoi(tok[1]));
        is->print(cout, g);
      }

      else if (0==strcmp(tok[0], "suppress-except")) {
        int id = atoi(tok[1]);
        Terminal const *t = (id==-1? NULL : g.getTerminal(atoi(tok[1])));
        DottedProduction::lookaheadSuppressExcept = t;
        if (t) {
          cout << "suppressing  " << t->name << endl;
        }
        else {
          cout << "suppressing nothing\n";
        }
      }

      else if (0==strcmp(tok[0], "reach")) {
        int targetId = atoi(tok[1]);

        // consider every state..
        for (int i=0; i < g.numItemSets(); i++) {
          ItemSet const *set = g.getItemSet(i);

          // iterate over all possible symbols to find transitions
          for (int termId=0; termId < g.numTerminals(); termId++) {
            ItemSet const *dest = set->transitionC(g.getTerminal(termId));
            if (dest && dest->id == targetId) {
              dest->print(cout, g);
            }
          }
          for (int nontermId=0; nontermId < g.numNonterminals(); nontermId++) {
            ItemSet const *dest = set->transitionC(g.getNonterminal(nontermId));
            if (dest && dest->id == targetId) {
              dest->print(cout, g);
            }
          }
        }
      }

      else if (0==strcmp(tok[0], "track-la")) {
        int stateId = atoi(tok[1]);
        ItemSet const *set = g.getItemSet(stateId);

        int prodId = atoi(tok[2]);                 
        Production const *prod = g.productions.nth(prodId);

        int termId = atoi(tok[3]);                         
        Terminal const *term = g.getTerminal(termId);
        
        






      }
      else if (0==strcmp(tok[0], "quit")) {
      }
      else {
        cout << "unknown command: " << tok[0] << endl;
      }
    }
    catch (xArrayBounds &) {
      cout << "too few arguments to " << tok[0] << endl;
    }








  #endif // 0

}

