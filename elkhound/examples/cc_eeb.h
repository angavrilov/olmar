// cc_eeb.h
// *** DO NOT EDIT BY HAND ***
// automatically generated by elkhound, from cc_eeb.gr

#ifndef CC_EEB_H
#define CC_EEB_H

#include "useract.h"     // UserActions


// parser context class
class 
#line 4 "cc_eeb.gr"
 EEB : public UserActions {
#line 16 "cc_eeb.h"


private:
  USER_ACTION_FUNCTIONS      // see useract.h

  // declare the actual action function
  static SemanticValue doReductionAction(
    EEB *ths,
    int productionId, SemanticValue const *semanticValues,
  SourceLoc loc);

  // declare the classifier function
  static int reclassifyToken(
    EEB *ths,
    int oldTokenType, SemanticValue sval);

  Node* action0___EarlyStartSymbol(SourceLoc loc, Node* top);
  Node* action1_E(SourceLoc loc, Node* e1, Node* e2);
  Node* action2_E(SourceLoc loc, int b);
  inline Node* dup_E(Node* n) ;
  inline void del_E(Node* n) ;
  inline Node* merge_E(Node* a, Node* b) ;

// the function which makes the parse tables
public:
  virtual ParseTables *makeTables();
};

#endif // CC_EEB_H