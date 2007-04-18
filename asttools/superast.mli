open Cc_ml_types
open Cc_ast_gen_type
open Ast_annotation

type 'a super_ast =
  | TranslationUnit_type of 'a translationUnit_type
  | TopForm_type of 'a topForm_type
  | Function_type of 'a function_type
  | MemberInit_type of 'a memberInit_type
  | Declaration_type of 'a declaration_type
  | ASTTypeId_type of 'a aSTTypeId_type
  | PQName_type of 'a pQName_type
  | TypeSpecifier_type of 'a typeSpecifier_type
  | BaseClassSpec_type of 'a baseClassSpec_type
  | Enumerator_type of 'a enumerator_type
  | MemberList_type of 'a memberList_type
  | Member_type of 'a member_type
  | ExceptionSpec_type of 'a exceptionSpec_type
  | OperatorName_type of 'a operatorName_type
  | Statement_type of 'a statement_type
  | Condition_type of 'a condition_type
  | Handler_type of 'a handler_type
  | Expression_type of 'a expression_type
  | FullExpression_type of 'a fullExpression_type
  | ArgExpression_type of 'a argExpression_type
  | ArgExpressionListOpt_type of 'a argExpressionListOpt_type
  | Initializer_type of 'a initializer_type
  | TemplateDeclaration_type of 'a templateDeclaration_type
  | TemplateParameter_type of 'a templateParameter_type
  | TemplateArgument_type of 'a templateArgument_type
  | NamespaceDecl_type of 'a namespaceDecl_type
  | Declarator_type of 'a declarator_type
  | IDeclarator_type of 'a iDeclarator_type
  | FullExpressionAnnot_type of 'a fullExpressionAnnot_type
  | ASTTypeof_type of 'a aSTTypeof_type
  | Designator_type of 'a designator_type
  (* we use attribute_type list list instead of AttributeSpecifierList_type,
   * see gnu_attribute_hack.ast
   * 
   * | AttributeSpecifierList_type of 'a attributeSpecifierList_type
   * | AttributeSpecifier_type of 'a attributeSpecifier_type
   *)
  | Attribute_type of 'a attribute_type
  | Variable of 'a variable
  | BaseClass of 'a baseClass
  | Compound_info of 'a compound_info
  | EnumType_Value_type of 'a enumType_Value_type
  | AtomicType of 'a atomicType
  | CType of 'a cType
  | STemplateArgument of 'a sTemplateArgument
  | Scope of 'a scope 

  | NoAstNode



val into_array : 
  int -> annotated translationUnit_type -> annotated super_ast array

val load_marshaled_ast_array : string -> annotated super_ast array

val iter : 
  (annotated super_ast -> unit) -> annotated super_ast array -> unit

val iteri : 
  (int -> annotated super_ast -> unit) -> annotated super_ast array -> unit

val node_loc : annotated super_ast -> sourceLoc option
