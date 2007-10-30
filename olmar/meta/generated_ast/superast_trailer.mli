(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* 
 * there are no source locs in the astgen ast
 * val super_source_loc : 'a super_ast -> sourceLoc option
 *)

val into_array : 
  int -> annotated aSTSpecFile_type -> (annotated super_ast array * int array)

val iteri : 
  (int -> annotated super_ast -> unit) -> annotated super_ast array -> unit

