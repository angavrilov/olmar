
open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_util




(* node type done: 1 - 30, 32, 33, 36 - 40
   not done: 31, 34, 35,
 *)

let oc = ref stdout;;

let print_caddr = ref false

module DS = Dense_set

let visited_nodes = DS.make ()

let visited (annot : annotated) =
  DS.mem (id_annotation annot) visited_nodes

let visit (annot : annotated) =
  (* Printf.eprintf "visit %d\n%!" (id_annotation annot); *)
  DS.add (id_annotation annot) visited_nodes


let not_implemented () =
  if 1 = 0 then 0 else assert false

let dot_escape s =
  let b = Buffer.create (max 31 (String.length s))
  in
    for i = 0 to String.length s -1 do
      match s.[i] with
	| '\\' -> Buffer.add_string b "\\\\"
	| '"' -> Buffer.add_string b "\\\""
	| '\n' -> Buffer.add_string b "\\n"
	| c -> Buffer.add_char b c
    done;
    Buffer.contents b

let any_label (field_name, value) =
  Printf.fprintf !oc "\\n%s: %s" field_name (dot_escape value)

let string_opt = function
  | None -> "(nil)"
  | Some s -> s

let count_rev base l =
  let counter = ref (List.length l)
  in
    List.rev_map 
      (fun x -> decr counter; (x, Printf.sprintf "%s[%d]" base !counter)) 
      l

let nodes_counter = ref 0

let start_label name color id =
  incr nodes_counter;
  Printf.fprintf !oc "    \"%d\" [color=\"%s\", label=\"%s %d" 
    id color name id

let finish_label caddr =
  output_string !oc "\"];\n"


let loc_label (file, line, char) =
  ("loc", Printf.sprintf "%s:%d:%d" file line char)

let edge_counter = ref 0

let child_edge id (cid,label) =
  incr edge_counter;
  Printf.fprintf !oc "    \"%d\" -> \"%d\" [label=\"%s\"];\n" id cid label

let child_edges id childs = 
  List.iter (child_edge id) childs

  
let opt_child child_fun field_name opt = 
  match opt with
    | None -> []
    | Some c -> [(child_fun c, field_name)]

let caddr_label caddr =
  ("caddr", Printf.sprintf "0x%lx" (Int32.shift_left (Int32.of_int caddr) 1))

let ast_node color annot name (labels :(string*string) list) childs =
  let id = id_annotation annot
  in
    start_label name color id;
    List.iter any_label 
      (if !print_caddr then
	 labels @ [(caddr_label (caddr_annotation annot))]
       else
	 labels);
    finish_label ();
    child_edges id childs;
    (* the return value must be the same as retval annot: *)
    (* assert(id = retval annot); *)
    id


let retval annot = id_annotation annot


let ast_loc_node color annot loc name labels childs =
  ast_node color annot name ((loc_label loc) :: labels) childs


(***************** colors *****************************)

let color_TranslationUnit = "red"
let color_TF = "firebrick2"
let color_Declaration = "cyan"
let color_Declarator = "SkyBlue"
let color_Function = "magenta"
let color_MemberInit = "HotPink"
let color_IDeclarator = "SteelBlue"
let color_Member = "SlateBlue1"
let color_MemberList = "SlateBlue4"
let color_PQName = "SkyBlue"
let color_Variable = "purple"
let color_TypeSpecifier = "OliveDrab"
let color_ASTTypeof = "SeaGreen"
let color_Statement = "yellow"
let color_handler = "khaki1"
let color_FullExpression = "coral"
let color_Expression = "orange"
let color_ArgExpression = "DarkOrange"
let color_ArgExpressionListOpt = "tomato"
let color_Condition = "OrangeRed"
let color_OperatorName = "chocolate1"
let color_Initializer = "gold"
let color_ASTTypeID = "MediumAquamarine"
let color_Enumerator = "PaleTurquoise"
let color_CType = "green"
let color_ATomicType = "LimeGreen"
let color_Compound_info = "LawnGreen"
let color_Enum_Value = "OliveDrab1"
let color_BaseClassSpec = "PaleGreen"
let color_BaseClass = "SpringGreen"
let color_exceptionSpec = "salmon"
let color_TemplateDeclaration = "brown3"
let color_TemplateParameter = "tan3"
let color_TemplateArgument = "peru"
let color_STemplateArgument = "maroon"
let color_NamespaceDecl = "grey50"
let color_Designator = "LemonChiffon"
let color_Attribute = "orchid1"

(***************** variable ***************************)

let rec variable_fun (v : annotated variable) =
  if visited v.poly_var then retval v.poly_var
  else begin
    visit v.poly_var;
    ast_loc_node color_Variable v.poly_var v.loc "Variable"
      [("name", string_opt v.var_name);
       ("flags", string_of_declFlags v.flags)
      ]
      (let l1 = opt_child cType_fun "type" !(v.var_type) in
       let l2 = opt_child expression_fun "varValue" v.value in
       let l3 = opt_child cType_fun "defaultParamType" v.defaultParam in
       let l4 = opt_child func_fun "funcDefn" !(v.funcDefn)
       in
	 l1 @ l2 @ l3 @ l4)
  end


(***************** cType ******************************)

and baseClass_fun bc =
  if visited bc.poly_base then retval bc.poly_base
  else begin
    visit bc.poly_base;
    ast_node color_BaseClass bc.poly_base "BaseClass"
      [("access", string_of_accessKeyword bc.bc_access);
       ("isVirtual", string_of_bool bc.is_virtual)]
      [(compound_info_fun bc.compound, "ct")]
  end

and compound_info_fun info = 
  if visited info.compound_info_poly then retval info.compound_info_poly 
  else begin
    visit info.compound_info_poly;
    ast_node color_Compound_info info.compound_info_poly "CompoundType" 
      [("name", info.compound_name);
       ("access", string_of_accessKeyword info.ci_access);
       ("forward", string_of_bool info.is_forward_decl);
       ("keyword", string_of_compoundType_Keyword info.keyword);
       ("instName", info.inst_name)
      ]
      (let x1 = variable_fun info.typedef_var, "typedefVar" in
       let l2 = 
	 count_rev "dataMembers"
	   (List.rev_map variable_fun info.data_members) in
       let l3 =
	 count_rev "virtualBases"
	   (List.rev_map baseClass_fun info.bases) in
       let l4 = 
	 count_rev "conversionOperators"
	   (List.rev_map variable_fun info.conversion_operators) in
       let l5 = 
	 count_rev "friends"
	   (List.rev_map variable_fun info.friends) in
       let l6 = opt_child cType_fun "selfType" !(info.self_type)
       in
	 x1 :: l2 @ l3 @ l4 @ l5 @ l6)
  end


and enum_value_fun (string, int) =
  ast_node color_Enum_Value (pseudo_annotation ()) "Enum::Value"
    [("name", string);
     ("value", string_of_int int)]
    []

and atomicType_fun at = 
  let annot = atomicType_annotation at
  in
    if visited annot then retval annot
    else 
      let atnode = ast_node color_ATomicType annot 
      in match at with
	(*
	 * put calls to visit here before in each case, except for CompoundType
	 *)

	| SimpleType(annot, simpleTypeId) ->
	    visit annot;
	    atnode "SimpleType" 
	      [("type", string_of_simpleTypeId simpleTypeId)] []

	| CompoundType(compound_info) ->
	    compound_info_fun compound_info

	| PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			      compound_info, sTemplateArgument_list) ->
	    visit annot;
	    atnode "PseudoInstantiation" 
	      [("name", str);
	       ("access", string_of_accessKeyword accessKeyword)]
	      (let l1 = opt_child variable_fun "typedefVar" variable_opt in
	       let l2 = [compound_info_fun compound_info, "primary"] in
	       let l3 = 
		 count_rev "args" 
		   (List.rev_map 
		      sTemplateArgument_fun sTemplateArgument_list)
	       in
		 l1 @ l2 @ l3)

	| EnumType(annot, string, variable, accessKeyword, string_int_list) ->
	    visit annot;
	    atnode "EnumType" 
	      [("name", string);
	       ("access", string_of_accessKeyword accessKeyword)]
	      (let x1 = variable_fun variable, "typedefVar" in
	       let l2 = 
		 count_rev "values"
		   (List.rev_map enum_value_fun string_int_list)
	       in 
		 x1 :: l2)

	| TypeVariable(annot, string, variable, accessKeyword) ->
	    visit annot;
	    atnode "TypeVariable" 
	      [("name", string);
	       ("access", string_of_accessKeyword accessKeyword)]
	      [(variable_fun variable, "typedefVar")]




and cType_fun t = 
  let annot = cType_annotation t 
  in
    if visited annot then retval annot
    else 
      let _ = visit annot in
      let tnode = ast_node color_CType annot in
      let tnode_1d name field value childs = tnode name [(field, value)] childs
      in match t with
	| CVAtomicType(annot, cVFlags, atomicType) ->
	    tnode_1d "CVAtomicType" "cv" (string_of_cVFlags cVFlags)
	      [(atomicType_fun atomicType, "atomic")]

	| PointerType(annot, cVFlags, cType) ->
	    tnode_1d "PointerType" "cv" (string_of_cVFlags cVFlags)
	      [(cType_fun cType, "atType")]
      

	| ReferenceType(annot, cType) ->
	    tnode "ReferenceType" [] [(cType_fun cType, "atType")]

	| FunctionType(annot, function_flags, cType, 
		       variable_list, cType_list_opt) ->
	    tnode "FunctionType" 
	      [("flags", (string_of_function_flags function_flags));
	       ("exnSpec", 
		match cType_list_opt with
		  | None -> "not present"
		  | Some [] -> "present, empty"
		  | Some _ -> "present, nonempty")
	      ]
	      (let x1 = (cType_fun cType, "retType") in
	       let l2 = 
		 count_rev "params" (List.rev_map variable_fun variable_list) in
	       let l3 = match cType_list_opt with
		 | None -> []
		 | Some l -> count_rev "exnSpec" (List.rev_map cType_fun l)
	       in
		 x1 :: l2 @ l3)

	| ArrayType(annot, cType, array_size) ->
	    tnode_1d "ArrayType" "size" (string_of_array_size array_size)
	      [(cType_fun cType, "eltType")]

	| PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			      cVFlags, cType) ->
	    assert(match atomicType with 
		     | SimpleType _ -> false
		     | CompoundType _
		     | PseudoInstantiation _
		     | EnumType _
		     | TypeVariable _ -> true);
	    tnode_1d "PointerToMemberType" "cv" (string_of_cVFlags cVFlags)
	      (let x1 = (atomicType_fun atomicType, "inClassNat") in
	       let x2 = (cType_fun cType, "atType")
	       in
		 [x1; x2])


and sTemplateArgument_fun ta = 
  let annot = sTemplateArgument_annotation ta
  in
    if visited annot then retval annot
    else 
      let _ = visit annot in
      let tanode = ast_node color_STemplateArgument annot in
      let tanode_1c name field child = tanode name [] [(child, field)]
      in match ta with
	| STA_NONE annot -> 
	    tanode "STA_NONE" [] []

	| STA_TYPE(annot, cType) -> 
	    tanode "STA_TYPE" [] [(cType_fun cType, "sta_value.t")]

	| STA_INT(annot, int) -> 
	    tanode "STA_INT" [("sta_value.i", string_of_int int)] []

	| STA_ENUMERATOR(annot, variable) -> 
	    tanode_1c "STA_ENUMERATOR" "sta_value.v" (variable_fun variable)

	| STA_REFERENCE(annot, variable) -> 
	    tanode_1c "STA_REFERENCE" "sta_value.v" (variable_fun variable)

	| STA_POINTER(annot, variable) -> 
	    tanode_1c "STA_POINTER" "sta_value.v" (variable_fun variable)

	| STA_MEMBER(annot, variable) ->
	    tanode_1c "STA_MEMBER" "sta_value.v" (variable_fun variable)

	| STA_DEPEXPR(annot, expression) -> 
	    tanode_1c "STA_DEPEXPR" "sta_value.e" (expression_fun expression)

	| STA_TEMPLATE annot -> 
	    tanode "STA_TEMPLATE" [] []

	| STA_ATOMIC(annot, atomicType) -> 
	    tanode_1c "STA_ATOMIC" "sta_value.at" (atomicType_fun atomicType)




(***************** generated ast nodes ****************)

and translationUnit_fun 
               ((annot, topForm_list) : annotated translationUnit_type) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_TranslationUnit annot "TranslationUnit" []
      (count_rev "topForms" (List.rev_map topForm_fun topForm_list))
  end

and topForm_fun tf =
  let annot = topForm_annotation tf
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tf_node = ast_loc_node color_TF annot (topForm_loc tf) in
      let tf_node_11 name label child = tf_node name [label] [child] 
      in match tf with
	| TF_decl(annot, loc, declaration) -> 
	    tf_node "TF_decl" [] [(declaration_fun declaration, "decl")]

	| TF_func(annot, loc, func) -> 
	    tf_node "TF_func" [] [(func_fun func, "f")]

	| TF_template(annot, loc, templateDeclaration) -> 
	    tf_node "TF_template" []
	      [(templateDeclaration_fun templateDeclaration, "td")]

	| TF_explicitInst(annot, loc, declFlags, declaration) -> 
	    tf_node_11 "TF_explicitInst" 
	      ("instFlags", string_of_declFlags declFlags)
	      (declaration_fun declaration, "d")

	| TF_linkage(annot, loc, linkage, translationUnit) -> 
	    tf_node_11 "TF_linkage" 
	      ("linkage", linkage)
	      (translationUnit_fun translationUnit, "forms")

	| TF_one_linkage(annot, loc, linkage, topForm) -> 
	    tf_node_11 "TF_one_linkage"
	      ("linkage", linkage)
	      (topForm_fun topForm, "form")

	| TF_asm(annot, loc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    tf_node "TF_asm" [] [(expression_fun e_stringLit, "text")]

	| TF_namespaceDefn(annot, loc, name_opt, topForm_list) -> 
	    tf_node "TF_namespaceDefn" 
	      [("name", string_opt name_opt)]
	      (count_rev "forms" (List.rev_map topForm_fun topForm_list))

	| TF_namespaceDecl(annot, loc, namespaceDecl) -> 
	    tf_node "TF_namespaceDecl" [] 
	      [(namespaceDecl_fun namespaceDecl, "decl")]



and func_fun(annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	 s_compound_opt, handler_list, statement_opt, bool) =
  if visited annot then retval annot
  else begin
    visit annot;
    assert(match s_compound_opt with
	     | None -> true
	     | Some s_compound ->
		 match s_compound with 
		   | S_compound _ -> true 
		   | _ -> false);
    ast_node color_Function annot "Function"
      [("dflags", string_of_declFlags declFlags);
       ("implicit def", string_of_bool bool)]
      (let x1 = typeSpecifier_fun typeSpecifier, "retspec" in
       let x2 = declarator_fun declarator, "nameAndParams" in
       let l4 = count_rev "inits" (List.rev_map memberInit_fun memberInit_list) 
       in
       let l5 = opt_child statement_fun "body" s_compound_opt in
       let l6 = count_rev "handlers" (List.rev_map handler_fun handler_list) in
       let l7 = opt_child statement_fun "dtor" statement_opt 
       in
	 x1 :: x2 :: l4 @ l5 @ l6 @ l7)
  end


and memberInit_fun(annot, pQName, argExpression_list, statement_opt) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_MemberInit annot "MemberInit" []
      (let x1 = (pQName_fun pQName, "name") in
       let l2 = 
	 count_rev "args" (List.rev_map argExpression_fun argExpression_list) in
       let l3 = opt_child statement_fun "ctorStatement" statement_opt
       in
	 x1 :: l2 @ l3)
  end

and declaration_fun(annot, declFlags, typeSpecifier, declarator_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_Declaration annot "Declaration" 
      [("dflags", string_of_declFlags declFlags)]
      (let x1 = (typeSpecifier_fun typeSpecifier, "spec") in
       let l2 = 
	 count_rev "decllist" (List.rev_map declarator_fun declarator_list)
       in
	 x1 :: l2)
  end


and aSTTypeId_fun(annot, typeSpecifier, declarator) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_ASTTypeID annot "ASTTypeId" []
      (let x1 = (typeSpecifier_fun typeSpecifier, "spec") in
       let x2 = (declarator_fun declarator, "decl")
       in
	 [x1; x2])
  end


and pQName_fun pq = 
  let annot = pQName_annotation pq
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let pq_node = ast_loc_node color_PQName annot (pQName_loc pq)
      in match pq with
	| PQ_qualifier(annot, loc, stringRef_opt, 
		       templateArgument_opt, pQName) -> 
	    pq_node "PQ_qualifier" 
	      [("qualifier", string_opt stringRef_opt)]
	      (let l1 = 
		 opt_child templateArgument_fun "templArgs" templateArgument_opt
	       in
	       let l2 = [(pQName_fun pQName, "rest")]
	       in
		 l1 @ l2)

	| PQ_name(annot, loc, stringRef) -> 
	    pq_node "PQ_name" [("name", stringRef)] []

	| PQ_operator(annot, loc, operatorName, stringRef) -> 
	    pq_node "PQ_operator" 
	      [("fakeName", stringRef)]
	      [(operatorName_fun operatorName, "o")]

	| PQ_template(annot, loc, stringRef, templateArgument_opt) -> 
	    pq_node "PQ_template" 
	      [("name", stringRef)]
	      (opt_child templateArgument_fun "templArgs" templateArgument_opt)

	| PQ_variable(annot, loc, variable) -> 
	    pq_node "PQ_variable" [] [(variable_fun variable, "var")]



and typeSpecifier_fun ts =
  let annot = typeSpecifier_annotation ts
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tsnode name labels childs = 
	ast_loc_node color_TypeSpecifier annot 
	  (typeSpecifier_loc ts) name
	  (("cv", string_of_cVFlags (typeSpecifier_cv ts)) :: labels)
	  childs
      in
      let tsnode_1d name field value childs = 
	tsnode name [(field, value)] childs
      in match ts with
	| TS_name(annot, loc, cVFlags, pQName, bool) -> 
	    tsnode_1d "TS_name" "typenameUsed" (string_of_bool bool)
	      [(pQName_fun pQName, "name")]

	| TS_simple(annot, loc, cVFlags, simpleTypeId) -> 
	    tsnode_1d "TS_simple" "id" (string_of_simpleTypeId simpleTypeId) []

	| TS_elaborated(annot, loc, cVFlags, typeIntr, pQName) -> 
	    tsnode_1d "TS_elaborated" "keyword" (string_of_typeIntr typeIntr)
	      [(pQName_fun pQName, "name")]

	| TS_classSpec(annot, loc, cVFlags, typeIntr, pQName_opt, 
		       baseClassSpec_list, memberList) -> 
	    tsnode_1d "TS_classSpec" "keyword" (string_of_typeIntr typeIntr)
	      (let l1 = opt_child pQName_fun "name" pQName_opt in
	       let l2 = 
		 count_rev "bases" 
		   (List.rev_map baseClassSpec_fun baseClassSpec_list) in
	       let l3 = [memberList_fun memberList, "members"]
	       in 
		 l1 @ l2 @ l3)

	| TS_enumSpec(annot, loc, cVFlags, stringRef_opt, enumerator_list) -> 
	    tsnode_1d "TS_enumSpec" "name" (string_opt stringRef_opt)
	      (count_rev "elts" (List.rev_map enumerator_fun enumerator_list))

	| TS_type(annot, loc, cVFlags, cType) -> 
	    tsnode "TS_type" [] [(cType_fun cType, "type")]

	| TS_typeof(annot, loc, cVFlags, aSTTypeof) -> 
	    tsnode "TS_typeof" [] [(aSTTypeof_fun aSTTypeof, "atype")]



and baseClassSpec_fun(annot, bool, accessKeyword, pQName) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_BaseClassSpec annot "BaseClassSpec"
      [("isVirtual", string_of_bool bool);
       ("access", string_of_accessKeyword accessKeyword)]
      [(pQName_fun pQName, "name")]
  end


and enumerator_fun(annot, loc, stringRef, expression_opt) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_loc_node color_Enumerator annot loc "Enumerator" 
      [("name", stringRef)]
      (opt_child expression_fun "expr" expression_opt)
  end


and memberList_fun(annot, member_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_MemberList annot "MemberList" []
      (count_rev "list" (List.rev_map member_fun member_list))
  end

and member_fun m = 
  let annot = member_annotation m
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let mnode = ast_loc_node color_Member annot (member_loc m) in
      let mnode_1c name child field = mnode name [] [(child, field)] 
      in match m with
	| MR_decl(annot, loc, declaration) -> 
	    mnode_1c "MR_decl" (declaration_fun declaration) "d"

	| MR_func(annot, loc, func) -> 
	    mnode_1c "MR_func" (func_fun func) "f"

	| MR_access(annot, loc, accessKeyword) -> 
	    mnode "MR_access" [("k", string_of_accessKeyword accessKeyword)] []

	| MR_usingDecl(annot, loc, nd_usingDecl) -> 
	    assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	    mnode_1c "MR_usingDecl" (namespaceDecl_fun nd_usingDecl) "decl"

	| MR_template(annot, loc, templateDeclaration) -> 
	    mnode_1c "MR_template" 
	      (templateDeclaration_fun templateDeclaration) "d"


and declarator_fun(annot, iDeclarator, init_opt, 
		   statement_opt_ctor, statement_opt_dtor) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_Declarator annot "Declarator" []
      (let x1 = (iDeclarator_fun iDeclarator, "decl") in
       let l2 = opt_child init_fun "init" init_opt in
       let l3 = opt_child statement_fun "ctor" statement_opt_ctor in
       let l4 = opt_child statement_fun "dtor" statement_opt_dtor
       in
	 x1 :: l2 @ l3 @ l4)
  end
    

and iDeclarator_fun idecl =
  let annot = iDeclarator_annotation idecl
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let inode = ast_loc_node color_IDeclarator annot (iDeclarator_loc idecl) 
      in
      let inode_1d name field value childs = inode name [(name,field)] childs
      in match idecl with
	| D_name(annot, loc, pQName_opt) -> 
	    inode "D_name" [] (opt_child pQName_fun "name" pQName_opt)

	| D_pointer(annot, loc, cVFlags, iDeclarator) -> 
	    inode_1d "D_pointer" "cv" (string_of_cVFlags cVFlags)
	      [(iDeclarator_fun iDeclarator, "base")]

	| D_reference(annot, loc, iDeclarator) -> 
	    inode "D_reference" [] [(iDeclarator_fun iDeclarator, "base")]

	| D_func(annot, loc, iDeclarator, aSTTypeId_list, cVFlags, 
		 exceptionSpec_opt, pq_name_list) -> 
	    assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		     pq_name_list);
	    inode_1d "D_func" "cv" (string_of_cVFlags cVFlags)
	      (let x1 = (iDeclarator_fun iDeclarator, "base") in
	       let l2 = 
		 count_rev "params" 
		   (List.rev_map aSTTypeId_fun aSTTypeId_list) in
	       let l3 = 
		 opt_child exceptionSpec_fun "exnSpec" exceptionSpec_opt in
	       let l4 = 
		 count_rev "kAndR_params" 
		   (List.rev_map pQName_fun pq_name_list)
	       in
		 x1 :: l2 @ l3 @ l4)

	| D_array(annot, loc, iDeclarator, expression_opt) -> 
	    inode "D_array" [] 
	      (let x1 = (iDeclarator_fun iDeclarator, "base") in
	       let l2 = opt_child expression_fun "size" expression_opt
	       in
		 x1 :: l2)

	| D_bitfield(annot, loc, pQName_opt, expression) -> 
	    inode "D_bitfield" []
	      (let l1 = opt_child pQName_fun "name" pQName_opt in
	       let l2 = [(expression_fun expression, "bits")]
	       in
		 l1 @ l2)

	| D_ptrToMember(annot, loc, pQName, cVFlags, iDeclarator) -> 
	    inode_1d "D_ptrToMember" "cv" (string_of_cVFlags cVFlags)
	      (let x1 = (pQName_fun pQName, "nestedName") in
	       let x2 = (iDeclarator_fun iDeclarator, "base")
	       in
		 [x1; x2])

	| D_grouping(annot, loc, iDeclarator) -> 
	    inode "D_grouping" [] [(iDeclarator_fun iDeclarator, "base")]

	| D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
	    inode "D_attribute" [] 
	      ((iDeclarator_fun iDeclarator, "base") ::
		 (List.flatten
		    (List.map
		       (fun (al, fname) -> count_rev fname al)
		       (count_rev "alist" 
			  (List.rev_map (List.rev_map attribute_fun)
			     attribute_list_list)))))


and exceptionSpec_fun(annot, aSTTypeId_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_exceptionSpec annot "ExceptionSpec" []
      (count_rev "types" (List.rev_map aSTTypeId_fun aSTTypeId_list))
  end


and operatorName_fun on = 
  let annot = operatorName_annotation on
  in 
    if visited annot then retval annot
    else
      let _ = visit annot in
      let onode = ast_node color_OperatorName annot
      in match on with
	| ON_newDel(annot, bool_is_new, bool_is_array) -> 
	    onode "ON_newDel" 
	      [("is_new", string_of_bool bool_is_new);
	       ("is_array", string_of_bool bool_is_array)]
	      []

	| ON_operator(annot, overloadableOp) -> 
	    onode "ON_operator"
	      [("op", string_of_overloadableOp overloadableOp)]
	      []

	| ON_conversion(annot, aSTTypeId) -> 
	    onode "ON_conversion" []
	      [(aSTTypeId_fun aSTTypeId, "type")]


and statement_fun s =
  let annot = statement_annotation s
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let snode = ast_loc_node color_Statement annot (statement_loc s) in
      let snode_1d name field value childs = snode name [(field, value)] childs
      in match s with
	| S_skip(annot, loc) -> 
	    snode "S_skip" [] []

	| S_label(annot, loc, stringRef, statement) -> 
	    snode_1d "S_label" "name" stringRef [(statement_fun statement, "s")]

	| S_case(annot, loc, expression, statement) -> 
	    snode "S_case" []
	      (let x1 = (expression_fun expression, "expr") in
	       let x2 = (statement_fun statement, "s")
	       in
		 [x1; x2])

	| S_default(annot, loc, statement) -> 
	    snode "S_default" [] [(statement_fun statement, "s")]

	| S_expr(annot, loc, fullExpression) -> 
	    snode "S_expr" [] [(fullExpression_fun fullExpression, "s")]

	| S_compound(annot, loc, statement_list) -> 
	    snode "S_compound" []
	      (count_rev "stmts" (List.rev_map statement_fun statement_list))

	| S_if(annot, loc, condition, statement_then, statement_else) -> 
	    snode "S_if" []
	      (let x1 = (condition_fun condition, "cond") in
	       let x2 = (statement_fun statement_then, "then") in
	       let x3 = (statement_fun statement_else, "else")
	       in
		 [x1; x2; x3])

	| S_switch(annot, loc, condition, statement) -> 
	    snode "S_switch" []
	      (let x1 = (condition_fun condition, "cond") in
	       let x2 = (statement_fun statement, "branches")
	       in
		 [x1; x2])

	| S_while(annot, loc, condition, statement) -> 
	    snode "S_while" []
	      (let x1 = (condition_fun condition, "cond") in
	       let x2 = (statement_fun statement, "body")
	       in
		 [x1; x2])

	| S_doWhile(annot, loc, statement, fullExpression) -> 
	    snode "S_doWhile" []
	      (let x1 = (statement_fun statement, "body") in
	       let x2 = (fullExpression_fun fullExpression, "expr")
	       in
		 [x1; x2])

	| S_for(annot, loc, statement_init, condition, 
		fullExpression, statement_body) -> 
	    snode "S_for" []
	      (let x1 = (statement_fun statement_init, "init") in
	       let x2 = (condition_fun condition, "cond") in
	       let x3 = (fullExpression_fun fullExpression, "after") in
	       let x4 = (statement_fun statement_body, "body")
	       in
		 [x1; x2; x3; x4])

	| S_break(annot, loc) -> 
	    snode "S_break" [] []

	| S_continue(annot, loc) ->
	    snode "S_continue" [] []

	| S_return(annot, loc, fullExpression_opt, statement_opt) -> 
	    snode "S_return" []
	      (let l1 = 
		 opt_child fullExpression_fun "expr" fullExpression_opt in
	       let l2 = opt_child statement_fun "copy_ctor" statement_opt
	       in 
		 l1 @ l2)

	| S_goto(annot, loc, stringRef) -> 
	    snode_1d "S_goto" "target" stringRef []

	| S_decl(annot, loc, declaration) -> 
	    snode "S_decl" [] [(declaration_fun declaration, "decl")]

	| S_try(annot, loc, statement, handler_list) -> 
	    snode "S_try" []
	      (let x1 = (statement_fun statement, "body") in
	       let l2 = 
		 count_rev "handler" (List.rev_map handler_fun handler_list) 
	       in
		 x1 :: l2)

	| S_asm(annot, loc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    snode "S_asm" [] [(expression_fun e_stringLit, "text")]

	| S_namespaceDecl(annot, loc, namespaceDecl) -> 
	    snode "S_namespaceDecl" [] 
	      [(namespaceDecl_fun namespaceDecl, "decl")]

	| S_function(annot, loc, func) -> 
	    snode "S_function" []
	      [(func_fun func, "f")]

	| S_rangeCase(annot, loc, expression_lo, expression_hi, statement) -> 
	    snode "S_rangeCase" []
	      (let x1 = (expression_fun expression_lo, "exprLo") in
	       let x2 = (expression_fun expression_hi, "exprHi") in
	       let x3 = (statement_fun statement, "s")
	       in
		 [x1; x2; x3])

	| S_computedGoto(annot, loc, expression) -> 
	    snode "S_computedGoto" [] [(expression_fun expression, "target")]


and condition_fun co = 
  let annot = condition_annotation co
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let conode = ast_node color_Condition annot
      in match co with
	| CN_expr(annot, fullExpression) -> 
	    conode "CN_expr" [] [(fullExpression_fun fullExpression, "expr")]

	| CN_decl(annot, aSTTypeId) -> 
	    conode "CN_decl" [] [(aSTTypeId_fun aSTTypeId, "typeId")]


and handler_fun(annot, aSTTypeId, statement_body,
		expression_opt, statement_gdtor) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_handler annot "Handler" []
      (let x1 = (aSTTypeId_fun aSTTypeId, "typeId") in
       let x2 = (statement_fun statement_body, "body") in
       let l3 = (opt_child expression_fun "localArg" expression_opt) in
       let l4 = (opt_child statement_fun "globalDtor" statement_gdtor)
       in
	 x1 :: x2 :: l3 @ l4)
  end


and expression_fun ex = 
  let annot = expression_annotation ex
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let exnode = ast_node color_Expression annot in
      let exnode_1d name field value childs = 
	                        exnode name [(field, value)] childs
      in match ex with
	| E_boolLit(annot, bool) -> 
	    exnode_1d "E_boolLit" "b" (string_of_bool bool) []

	| E_intLit(annot, stringRef) -> 
	    exnode_1d "E_intLit" "text" stringRef []

	| E_floatLit(annot, stringRef) -> 
	    exnode_1d "E_floatLit" "text" stringRef []

	| E_stringLit(annot, stringRef, e_stringLit_opt) -> 
	    assert(match e_stringLit_opt with 
		     | Some(E_stringLit _) -> true 
		     | None -> true
		     | _ -> false);
	    exnode_1d "E_stringLit" "text" stringRef
	      (opt_child expression_fun "continuation" e_stringLit_opt)

	| E_charLit(annot, stringRef) -> 
	    exnode_1d "E_charLit" "text" stringRef []

	| E_this annot -> 
	    exnode "E_this" [] []

	| E_variable(annot, pQName) -> 
	    exnode "E_variable" [] [(pQName_fun pQName, "name")]

	| E_funCall(annot, expression_func, argExpression_list, 
		    expression_retobj_opt) -> 
	    exnode "E_funCall" []
	      (let x1 = (expression_fun expression_func, "func") in
	       let l2 = 
		 count_rev "args"
		   (List.rev_map argExpression_fun argExpression_list) in
	       let l3 = opt_child expression_fun "retObj" expression_retobj_opt
	       in
		 x1 :: l2 @ l3)

	| E_constructor(annot, typeSpecifier, argExpression_list, 
			bool, expression_opt) -> 
	    exnode_1d "E_constructor" "artificial" (string_of_bool bool)
	      (let x1 = (typeSpecifier_fun typeSpecifier, "spec") in
	       let l2 = 
		 count_rev "args" 
		   (List.rev_map argExpression_fun argExpression_list) in
	       let l3 = opt_child expression_fun "retObj" expression_opt
	       in
		 x1 :: l2 @ l3)

	| E_fieldAcc(annot, expression, pQName) -> 
	    exnode "E_fieldAcc" []
	      (let x1 = (expression_fun expression, "obj") in
	       let x2 = (pQName_fun pQName, "fieldName")
	       in
		 [x1; x2])

	| E_sizeof(annot, expression) -> 
	    exnode "E_sizeof" []
	      [(expression_fun expression, "expr")]

	| E_unary(annot, unaryOp, expression) -> 
	    exnode_1d "E_unary" "op" (string_of_unaryOp unaryOp)
	      [(expression_fun expression, "expr")]

	| E_effect(annot, effectOp, expression) -> 
	    exnode_1d "E_effect" "op" (string_of_effectOp effectOp)
	      [(expression_fun expression, "expr")]

	| E_binary(annot, expression_left, binaryOp, expression_right) -> 
	    exnode_1d "E_binary" "op" (string_of_binaryOp binaryOp) 
	      (let x1 = (expression_fun expression_left, "e1") in
	       let x2 = (expression_fun expression_right, "e2")
	       in
		 [x1; x2])

	| E_addrOf(annot, expression) -> 
	    exnode "E_addrOf" [] [(expression_fun expression, "expr")]

	| E_deref(annot, expression) -> 
	    exnode "E_deref" [] [(expression_fun expression, "prt")]

	| E_cast(annot, aSTTypeId, expression) -> 
	    exnode "E_cast" []
	      (let x1 = (expression_fun expression, "expr") in
	       let x2 = (aSTTypeId_fun aSTTypeId, "ctype")
	       in
		 [x1; x2])

	| E_cond(annot, expression_cond, expression_true, expression_false) -> 
	    exnode "E_cond" []
	      (let x1 = (expression_fun expression_cond, "cond") in
	       let x2 = (expression_fun expression_true, "th") in
	       let x3 = (expression_fun expression_false, "el")
	       in
		 [x1; x2; x3])

	| E_sizeofType(annot, aSTTypeId) -> 
	    exnode "E_sizeofType" []
	      [(aSTTypeId_fun aSTTypeId, "atype")]

	| E_assign(annot, expression_target, binaryOp, expression_src) -> 
	    exnode_1d "E_assign" "op" (string_of_binaryOp binaryOp)
	      (let x1 = (expression_fun expression_target, "target") in
	       let x2 = (expression_fun expression_src, "src")
	       in
		 [x1; x2])

	| E_new(annot, bool, argExpression_list, aSTTypeId, 
		argExpressionListOpt_opt, statement_opt) -> 
	    exnode_1d "E_new" "colonColon" (string_of_bool bool)
	      (let l1 = 
		 count_rev "placementArgs"
		   (List.rev_map argExpression_fun argExpression_list) in
	       let x2 = (aSTTypeId_fun aSTTypeId, "atype") in
	       let l3 = 
		 opt_child argExpressionListOpt_fun 
		   "ctorArgs" argExpressionListOpt_opt in
	       let l4 = opt_child statement_fun "ctorStatement" statement_opt
	       in
		 l1 @ x2 :: l3 @ l4)

	| E_delete(annot, bool_colon, bool_array, 
		   expression_opt, statement_opt) ->
	    exnode "E_delete"
	      [("colonColon", string_of_bool bool_colon);
	       ("array", string_of_bool  bool_array)]
	      (let l1 = opt_child expression_fun "expr" expression_opt in
	       let l2 = opt_child statement_fun "dtorStatement" statement_opt
	       in
		 l1 @ l2)

	| E_throw(annot, expression_opt, statement_opt) -> 
	    exnode "E_throw" []
	      ((opt_child expression_fun "expr" expression_opt)
	       @ (opt_child statement_fun "globalCtorStatement" statement_opt))

	| E_keywordCast(annot, castKeyword, aSTTypeId, expression) -> 
	    exnode_1d "E_keywordCast" "key" (string_of_castKeyword castKeyword)
	      (let x1 = (aSTTypeId_fun aSTTypeId, "ctype") in
	       let x2 = (expression_fun expression, "expr")
	       in
		 [x1; x2])

	| E_typeidExpr(annot, expression) -> 
	    exnode "E_typeidExpr" [] [(expression_fun expression, "expr")]

	| E_typeidType(annot, aSTTypeId) -> 
	    exnode "E_typeidType" [] [(aSTTypeId_fun aSTTypeId, "ttype")]

	| E_grouping(annot, expression) -> 
	    exnode "E_grouping" [] [(expression_fun expression, "expr")]

	| E_arrow(annot, expression, pQName) -> 
	    exnode "E_arrow" []
	      (let x1 = (expression_fun expression, "obj") in
	       let x2 = (pQName_fun pQName, "fieldName")
	       in
		 [x1; x2])

	| E_statement(annot, s_compound) -> 
	    assert(match s_compound with | S_compound _ -> true | _ -> false);
	    exnode "E_statement" [] 
	      [(statement_fun s_compound, "s")]

	| E_compoundLit(annot, aSTTypeId, in_compound) -> 
	    assert(match in_compound with | IN_compound _ -> true | _ -> false);
	    exnode "E_compoundLit" []
	      (let x1 = (aSTTypeId_fun aSTTypeId, "stype") in
	       let x2 = (init_fun in_compound, "init")
	       in
		 [x1; x2])

	| E___builtin_constant_p(annot, loc, expression) -> 
	    ast_loc_node color_Expression annot loc "E___builtin_constant_p" []
	      [(expression_fun expression, "expr")]

	| E___builtin_va_arg(annot, loc, expression, aSTTypeId) -> 
	    ast_loc_node color_Expression annot loc "E___builtin_va_arg" []
	      (let x1 = (expression_fun expression, "expr") in
	       let x2 = (aSTTypeId_fun aSTTypeId, "atype")
	       in
		 [x1; x2])

	| E_alignofType(annot, aSTTypeId) -> 
	    exnode "E_alignofType" []
	      [(aSTTypeId_fun aSTTypeId, "atype")]

	| E_alignofExpr(annot, expression) -> 
	    exnode "E_alignofExpr" []
	      [(expression_fun expression, "expr")]

	| E_gnuCond(annot, expression_cond, expression_false) -> 
	    exnode "E_gnuCond" []
	      (let x1 = (expression_fun expression_cond, "cond") in
	       let x2 = (expression_fun expression_false, "el")
	       in
		 [x1; x2])

	| E_addrOfLabel(annot, stringRef) -> 
	    exnode_1d "E_addrOfLabel" "labelName" stringRef []


and fullExpression_fun(annot, expression_opt) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_FullExpression annot "FullExpression" []
      (opt_child expression_fun "expr" expression_opt)
  end


and argExpression_fun(annot, expression) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_ArgExpression annot "ArgExpression" []
      [(expression_fun expression, "expr")]
  end


and argExpressionListOpt_fun(annot, argExpression_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_ArgExpressionListOpt annot "ArgExpressionListOpt" []
      (count_rev "list" (List.rev_map argExpression_fun argExpression_list))
  end


and init_fun i = 
  let annot = init_annotation i
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let inode = ast_loc_node color_Initializer annot (init_loc i) 
      in match i with
	| IN_expr(annot, loc, expression) -> 
	    inode "IN_expr" [] [(expression_fun expression, "e")]

	| IN_compound(annot, loc, init_list) -> 
	    inode "IN_compound" []
	      (count_rev "inits" (List.rev_map init_fun init_list))

	| IN_ctor(annot, loc, argExpression_list, bool) -> 
	    inode "IN_ctor" 
	      [("was_IN_expr", string_of_bool bool)]
	      (count_rev "args" 
		 (List.rev_map argExpression_fun argExpression_list))

	| IN_designated(annot, loc, designator_list, init) -> 
	    inode "IN_designated" []
	      (let l1 = 
		 count_rev "designator"
		   (List.rev_map designator_fun designator_list) in
	       let l2 = [(init_fun init, "init")]
	       in
		 l1 @ l2)


and templateDeclaration_fun td = 
  let annot = templateDeclaration_annotation td
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tdnode name childs = 
	ast_node color_TemplateDeclaration annot name [] childs
      in match td with
	| TD_func(annot, templateParameter_opt, func) -> 
	    tdnode "TD_func" 
	      (let l1 = opt_child
		 templateParameter_fun "params" templateParameter_opt in
	       let l2 = [(func_fun func, "f")]
	       in
		 l1 @ l2)

	| TD_decl(annot, templateParameter_opt, declaration) -> 
	    tdnode "TD_decl" 
	      (let l1 = opt_child
		 templateParameter_fun "params" templateParameter_opt in
	       let l2 = [(declaration_fun declaration, "d")]
	       in
		 l1 @ l2)

	| TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
	    tdnode "TD_tmember"
	      (let l1 = 
		 opt_child 
		   templateParameter_fun "params" templateParameter_opt in
	       let l2 = [(templateDeclaration_fun templateDeclaration, "d")]
	       in
		 l1 @ l2)


and templateParameter_fun tp = 
  let annot = templateParameter_annotation tp
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tpnode name next labels childs =
	ast_loc_node color_TemplateParameter annot (templateParameter_loc tp)
	  name labels
	  (childs @ (opt_child templateParameter_fun "next" next))
      in match tp with
	| TP_type(annot, loc, stringRef, 
		  aSTTypeId_opt, templateParameter_opt) -> 
	    tpnode "TP_type" templateParameter_opt 
	      [("name", stringRef)]
	      (opt_child aSTTypeId_fun "defaultType" aSTTypeId_opt)

	| TP_nontype(annot, loc, aSTTypeId, templateParameter_opt) -> 
	    tpnode "TP_nontype" templateParameter_opt []
	      [(aSTTypeId_fun aSTTypeId, "param")]



and templateArgument_fun ta = 
  let annot = templateArgument_annotation ta
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tanode name next childs = 
	ast_node color_TemplateArgument annot name []
	  (childs @ (opt_child templateArgument_fun "next" next))
      in match ta with
	| TA_type(annot, aSTTypeId, templateArgument_opt) -> 
	    tanode "TA_type" templateArgument_opt 
	      [(aSTTypeId_fun aSTTypeId, "type")]

	| TA_nontype(annot, expression, templateArgument_opt) -> 
	    tanode "TA_nontype" templateArgument_opt
	      [(expression_fun expression, "expr")]

	| TA_templateUsed(annot, templateArgument_opt) -> 
	    tanode "TA_templateUsed" templateArgument_opt []


and namespaceDecl_fun nd = 
  let annot = namespaceDecl_annotation nd
  in 
    if visited annot then retval annot
    else
      let _ = visit annot in
      let ndnode = ast_node color_NamespaceDecl annot
      in match nd with
	| ND_alias(annot, stringRef, pQName) -> 
	    ndnode "ND_alias" 
	      [("alias", stringRef)]
	      [(pQName_fun pQName, "original")]

	| ND_usingDecl(annot, pQName) -> 
	    ndnode "ND_usingDecl" []
	      [(pQName_fun pQName, "name")]

	| ND_usingDir(annot, pQName) -> 
	    ndnode "ND_usingDir" []
	      [(pQName_fun pQName, "name")]

(* 
 * 31 and fullExpressionAnnot_fun(declaration_list) =
 *     List.iter declaration_fun declaration_list
 *)


and aSTTypeof_fun a = 
  let annot = aSTTypeof_annotation a
  in
    if visited annot then retval annot
    else
      let _ = visited annot in
      let anode = ast_node color_ASTTypeof annot
      in match a with
	| TS_typeof_expr(annot, fullExpression) -> 
	    anode "TS_typeof_expr" [] 
	      [(fullExpression_fun fullExpression, "expr")]

	| TS_typeof_type(annot, aSTTypeId) -> 
	    anode "TS_typeof_type" []
	      [(aSTTypeId_fun aSTTypeId, "atype")]



and designator_fun d = 
  let annot = designator_annotation d
  in 
    if visited annot then retval annot
    else
      let _ = visit annot in
      let dnode = ast_loc_node color_Designator annot (designator_loc d)
      in match d with
	| FieldDesignator(annot, loc, stringRef) -> 
	    dnode "FieldDesignator" [("id", stringRef)] []

	| SubscriptDesignator(annot, loc, expression, expression_opt) -> 
	    dnode "SubscriptDesignator" []
	      (let x1 = (expression_fun expression, "idx_expr") in
	       let l2 = opt_child expression_fun "idx_expr2" expression_opt
	       in
		 x1 :: l2)


and attribute_fun a = 
  let annot = attribute_annotation a
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let anode = ast_loc_node color_Attribute annot (attribute_loc a)
      in match a with
	| AT_empty(annot, loc) -> 
	    anode "AT_empty" [] []

	| AT_word(annot, loc, stringRef) -> 
	    anode "AT_word" [("w", stringRef)] []

	| AT_func(annot, loc, stringRef, argExpression_list) -> 
	    anode "AT_func" 
	      [("f", stringRef)]
	      (count_rev "args" 
		 (List.rev_map argExpression_fun argExpression_list))




(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)


let out_file = ref ""

let size_flag = ref false

let arguments = Arg.align
  [
    ("-o", Arg.Set_string out_file,
     "file set output file name");
    ("-size", Arg.Set size_flag,
     " limit size of output")

  ]

let usage_msg = 
  "usage: ast_graph [options...] <file>\n\
   recognized options are:"

let usage () =
  prerr_endline usage_msg;
  exit(1)
  
let file = ref ""

let file_set = ref false

let anonfun fn = 
  if !file_set 
  then
    begin
      Printf.eprintf "don't know what to do with %s\n" fn;
      usage()
    end
  else
    begin
      file := fn;
      file_set := true
    end

let start_file infile =
  output_string !oc "digraph ";
  Printf.fprintf !oc "\"%s\"" infile;
  output_string !oc " {\n";
  if !size_flag then
    output_string !oc "size=\"90,90\";\n";
  output_string !oc 
    "    color=white;\n";
  output_string !oc
    "    ordering=out;\n";
  output_string !oc
    "    node [ style = filled ];\n";
  output_string !oc
    "    edge [ arrowtail=odot ];\n"


let finish_file () =
  output_string !oc "}\n"

let main () =
  Arg.parse arguments anonfun usage_msg;
  if not !file_set then
    usage();				(* does not return *)
  let ic = open_in !file in
  let ofile = 
    if !out_file <> "" 
    then !out_file
    else "nodes.dot"
  in
  let _ = oc := open_out (ofile) in
  let ast = (Marshal.from_channel ic : annotated translationUnit_type)
  in
    start_file !file;
    ignore(translationUnit_fun ast);
    finish_file ();
    Printf.printf "graph with %d nodes and %d edges generated\n%!"
      !nodes_counter !edge_counter
      
;;


Printexc.catch main ()


