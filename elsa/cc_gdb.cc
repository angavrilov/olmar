// cc.ast            see license.txt for copyright and terms of use
// Provide every ast node with a gdb() method which prints it out

#include "cc_ast.h"

void TranslationUnit::gdb()      {debugPrint(cout, 0);};
void TopForm::gdb()              {debugPrint(cout, 0);};
void Function::gdb()             {debugPrint(cout, 0);};
void MemberInit::gdb()           {debugPrint(cout, 0);};
void Declaration::gdb()          {debugPrint(cout, 0);};
void ASTTypeId::gdb()            {debugPrint(cout, 0);};
void PQName::gdb()               {debugPrint(cout, 0);};
void TypeSpecifier::gdb()        {debugPrint(cout, 0);};
void BaseClassSpec::gdb()        {debugPrint(cout, 0);};
void Enumerator::gdb()           {debugPrint(cout, 0);};
void MemberList::gdb()           {debugPrint(cout, 0);};
void Member::gdb()               {debugPrint(cout, 0);};
void Declarator::gdb()           {debugPrint(cout, 0);};
void IDeclarator::gdb()          {debugPrint(cout, 0);};
void ExceptionSpec::gdb()        {debugPrint(cout, 0);};
void Statement::gdb()            {debugPrint(cout, 0);};
void Condition::gdb()            {debugPrint(cout, 0);};
void Handler::gdb()              {debugPrint(cout, 0);};
void Expression::gdb()           {debugPrint(cout, 0);};
void FullExpression::gdb()       {debugPrint(cout, 0);};
void ArgExpression::gdb()        {debugPrint(cout, 0);};
void ArgExpressionListOpt::gdb() {debugPrint(cout, 0);};
void Initializer::gdb()          {debugPrint(cout, 0);};
void TemplateDeclaration::gdb()  {debugPrint(cout, 0);};
void TemplateParameter::gdb()    {debugPrint(cout, 0);};
void TemplateArgument::gdb()     {debugPrint(cout, 0);};
void NamespaceDecl::gdb()        {debugPrint(cout, 0);};
