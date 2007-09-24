(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* simple types needed in the ocaml ast of astgen *)

type fieldFlag =
  (* | FF_NONE     (\* = 0x00,	// no flag			  *\) *)
  | FF_IS_OWNER (* = 0x01,	// is_owner modifier present	  *)
  | FF_NULLABLE (* = 0x02,	// nullable modifier present	  *)
  | FF_FIELD    (* = 0x04,	// field modifier present	  *)
  | FF_XML      (* = 0x08,	// some xml.* modifier present	  *)
  | FF_PRIVAT   (* = 0x10	// private, use accessor function *)
  | FF_CIRCULAR (* = 0x20	// possibly circular field        *)


type fieldFlags = fieldFlag list

let string_of_fieldFlag = function
  | FF_IS_OWNER -> "FF_IS_OWNER"
  | FF_NULLABLE -> "FF_NULLABLE"
  | FF_FIELD    -> "FF_FIELD"
  | FF_XML      -> "FF_XML"
  | FF_PRIVAT   -> "FF_PRIVAT"
  | FF_CIRCULAR -> "FF_CIRCULAR"


let string_of_fieldFlags l =
  Astgen_util.string_of_flag_list string_of_fieldFlag l


type accessCtl =
  | AC_PUBLIC      (* access		*)
  | AC_PRIVATE     (*   control		*)
  | AC_PROTECTED   (*     keywords	*)
  | AC_CTOR        (* insert into ctor	*)
  | AC_DTOR        (* insert into dtor	*)
  | AC_PUREVIRT   (* declare pure virtual in superclass, and impl in subclass *)


let string_of_accessCtl = function
  | AC_PUBLIC    -> "AC_PUBLIC"
  | AC_PRIVATE   -> "AC_PRIVATE"
  | AC_PROTECTED -> "AC_PROTECTED"
  | AC_CTOR      -> "AC_CTOR"
  | AC_DTOR      -> "AC_DTOR"
  | AC_PUREVIRT  -> "AC_PUREVIRT"
