(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cc_ast_gen_type
open Ast_annotation

let name_of_function 
    (_annot, _declFlags, _typeSpecifier, declarator, _memberInit_list, 
     _s_compound_opt, _handler_list, _func, _variable_opt_1, 
     _variable_opt_2, _statement_opt, _bool) =
  match declarator with
      (_annot, iDeclarator, _init_opt, 
       _variable_opt, _ctype_opt, _declaratorContext,
       _statement_opt_ctor, _statement_opt_dtor) 
      ->
	match iDeclarator with
	  | D_func(_annot, _sourceLoc, iDeclarator, _aSTTypeId_list, _cVFlags, 
		   _exceptionSpec_opt, _pq_name_list, _bool) -> 
	      (match iDeclarator with
		 | D_name(_annot, _sourceLoc, pQName_opt) -> 
		     (match pQName_opt with
			| Some pq_name ->
			    (match pq_name with
			       | PQ_name(_annot, _sourceLoc, name) -> name

			       | PQ_qualifier _
			       | PQ_operator _
			       | PQ_template _
			       | PQ_variable _
				 -> assert false
			    )
			| None -> assert false
		     )
		 | D_func _
		 | D_pointer _
		 | D_reference _
		 | D_array _
		 | D_bitfield _
		 | D_ptrToMember _
		 | D_grouping _
		 | D_attribute _
		     -> assert false
	      )
	  | D_name _
	  | D_pointer _
	  | D_reference _
	  | D_array _
	  | D_bitfield _
	  | D_ptrToMember _
	  | D_grouping _
	  | D_attribute _
	    -> assert false



let body_of_function
    (_annot, _declFlags, _typeSpecifier, _declarator, _memberInit_list, 
     s_compound_opt, _handler_list, _func, _variable_opt_1, 
     _variable_opt_2, _statement_opt, _bool) =
  match s_compound_opt with
    | Some compound -> compound
    | None -> assert false
