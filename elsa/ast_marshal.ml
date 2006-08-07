open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation

let marshal_translation_unit (u : annotated translationUnit_type) fname =
  let oc = open_out fname 
  in
    (* 
     * Printf.eprintf "marshall ast with %d elems to %s\n%!" (List.length u) fname;
     * (match u with
     *    | [TF_decl(loc,(flags, 
     * 	    TS_classSpec(loc2, flags2, ti, Some pq, bl, 
     * 	      [c;d;e;
     * 	       MR_func(loc3,
     * 		 (flags3, ts, 
     * 		  (D_func(loc4, idt, 
     * 		    [((TS_type(loc5, flags5, 
     * 			t
     * 			     )) as ttt, dt)], 
     * 			  flags4, None, pt2), 
     * 		   None, None, None),
     * 		  mil, st, htl, None, true));
     * 	       g;h]), 
     * 		       dtl)) ;b] -> 
     * 	   Printf.eprintf "is block %b at addr %x\n%!" 
     * 	     (Obj.is_block (Obj.repr t))
     * 	     ((Obj.magic ttt))
     * 	   ;
     * 	   if String.length 
     * 	     (Marshal.to_string ttt []) = 0 
     * 	   then Printf.eprintf "MARSHAL ERROR\n%!"
     * 	   else Printf.eprintf "marshall ok\n%!"
     * 	     
     *    | _ -> Printf.eprintf "marshal match error\n%!";
     * );
     *)
    Marshal.to_channel oc u [];
    close_out oc

let marshal_translation_unit_callback u fname =
  try
    marshal_translation_unit u fname
  with
    | x -> 
	Printf.eprintf
	  "MARSHALL ERROR: exception in marshal_translation_unit:\n%s\n%!"
	  (Printexc.to_string x);
	exit(1)

let register_marshal_callback () =
  Callback.register
    "marshal_translation_unit_callback"
    marshal_translation_unit_callback

  
  
