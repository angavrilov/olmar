// cc_ast_aux.h
// dsw: stuff I would like to put into cc.ast but I can't

#ifndef CC_AST_AUX_H
#define CC_AST_AUX_H

#include "sobjset.h"            // class SObjSet

// I can't put this in cc.ast because the class I inherit from,
// ASTVisitor, is generated at the end of cc.ast.gen.h

// extend the visitor to traverse templates
class ASTTemplVisitor : public ASTVisitor {
  // also visit template definitions of primaries and partials
  bool primariesAndPartials;

  // true if we may assume that the AST has been tchecked
  bool hasBeenTchecked;

  // set of the (primary) TemplateInfo objects the instantiations of
  // which we have visited; prevents us from visiting them twice
  SObjSet<TemplateInfo *> primaryTemplateInfos;

public:
  ASTTemplVisitor
    (bool primariesAndPartials0 = false,
     bool hasBeenTchecked0 = true
     )
    : primariesAndPartials(primariesAndPartials0)
    , hasBeenTchecked(hasBeenTchecked0)
  {}

  virtual ~ASTTemplVisitor() {}

  bool visitTemplateDeclaration(TemplateDeclaration *td);

private:
  void visitTemplateDeclaration_oneTempl(Variable *var0);
};

#endif // CC_AST_AUX_H
