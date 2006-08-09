val annotation_fun : int * int -> int * int
val string_fun : string -> string
val bool_fun : bool -> bool
val int_fun : int -> int
val sourceLoc_fun : string * int * int -> sourceLoc
val declFlags_fun : declFlag list -> declFlag list
val simpleTypeId_fun : simpleTypeId -> simpleTypeId
val typeIntr_fun : typeIntr -> typeIntr
val accessKeyword_fun : accessKeyword -> accessKeyword
val cVFlags_fun : cVFlag list -> cVFlag list
val overloadableOp_fun : overloadableOp -> overloadableOp
val unaryOp_fun : unaryOp -> unaryOp
val effectOp_fun : effectOp -> effectOp
val binaryOp_fun : binaryOp -> binaryOp
val castKeyword_fun : castKeyword -> castKeyword
val function_flags_fun : function_flags -> function_flags
val array_size_fun : array_size -> array_size
val compoundType_Keyword_fun : compoundType_Keyword -> compoundType_Keyword

val variable_fun : annotated variable -> annotated variable
val baseClass_fun : annotated baseClass -> annotated baseClass
val compound_info_fun : annotated compound_info -> annotated compound_info
val atomicType_fun : annotated atomicType -> annotated atomicType
val cType_fun : annotated cType -> annotated cType
val sTemplateArgument_fun : annotated sTemplateArgument -> annotated sTemplateArgument
val translationUnit_fun : annotated translationUnit_type -> annotated translationUnit_type
val topForm_fun : annotated topForm_type -> annotated topForm_type
val func_fun : annotated function_type -> annotated function_type
val memberInit_fun : annotated memberInit_type -> annotated memberInit_type
val declaration_fun : annotated declaration_type -> annotated declaration_type
val aSTTypeId_fun : annotated aSTTypeId_type -> annotated aSTTypeId_type
val pQName_fun : annotated pQName_type -> annotated pQName_type
val typeSpecifier_fun : annotated typeSpecifier_type -> annotated typeSpecifier_type
val baseClassSpec_fun : annotated baseClassSpec_type -> annotated baseClassSpec_type
val enumerator_fun : annotated enumerator_type -> annotated enumerator_type
val memberList_fun : annotated memberList_type -> annotated memberList_type
val member_fun : annotated member_type -> annotated member_type
val declarator_fun : annotated declarator_type -> annotated declarator_type
val iDeclarator_fun : annotated iDeclarator_type -> annotated iDeclarator_type
val exceptionSpec_fun : annotated exceptionSpec_type -> annotated exceptionSpec_type
val operatorName_fun : annotated operatorName_type -> annotated operatorName_type
val statement_fun : annotated statement_type -> annotated statement_type
val condition_fun : annotated condition_type -> annotated condition_type
val handler_fun : annotated handler_type -> annotated handler_type
val expression_fun : annotated expression_type -> annotated expression_type
val fullExpression_fun : annotated fullExpression_type -> annotated fullExpression_type
val argExpression_fun : annotated argExpression_type -> annotated argExpression_type
val argExpressionListOpt_fun : annotated argExpressionListOpt_type -> annotated argExpressionListOpt_type
val init_fun : annotated initializer_type -> annotated initializer_type
val templateDeclaration_fun : annotated templateDeclaration_type -> annotated templateDeclaration_type
val templateParameter_fun : annotated templateParameter_type -> annotated templateParameter_type
val templateArgument_fun : annotated templateArgument_type -> annotated templateArgument_type
val namespaceDecl_fun : annotated namespaceDecl_type -> annotated namespaceDecl_type
val fullExpressionAnnot_fun : annotated declaration_type list -> annotated declaration_type list
val aSTTypeof_fun : annotated aSTTypeof_type -> annotated aSTTypeof_type
val designator_fun : annotated designator_type -> annotated designator_type
val attributeSpecifierList_fun : annotated attributeSpecifierList_type -> annotated attributeSpecifierList_type
val attributeSpecifier_fun : annotated attributeSpecifier_type -> annotated attributeSpecifier_type
val attribute_fun : annotated attribute_type -> annotated attribute_type
