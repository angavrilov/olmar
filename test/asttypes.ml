
(* hand translation of the enum in the verbatim section *)
(* xml serialization unclear via seemingly undefined toXml *)
type accessCtl =
  | AC_PUBLIC      (* access *)
  | AC_PRIVATE     (* control *)
  | AC_PROTECTED   (* keywords *)
  | AC_CTOR        (* insert into ctor *)
  | AC_DTOR        (* insert into dtor *)
  | AC_PUREVIRT   (* declare pure virtual in superclass, and impl in subclass *)
  | NUM_ACCESSCTLS



(*** Local Variables: ***)
(*** compile-command : "ocamlc.opt -c asttypes.ml" ***)
(*** End: ***)
