// cc_flags.h            see license.txt for copyright and terms of use
// ocaml serialization utility functions



#ifndef CC_OCAML_H
#define CC_OCAML_H

#include "cc_flags.h"
#include "ocamlhelp.h"
#include "variable.h"      // Variable
#include "cc_type.h"       // CType, FunctonType, CompoundType


value ocaml_from_SourceLoc(const SourceLoc &, ToOcamlData *);

// for flag sets
value ocaml_from_DeclFlags(const DeclFlags &, ToOcamlData *);
value ocaml_from_CVFlags(const CVFlags &, ToOcamlData *);

// for real enums
value ocaml_from_SimpleTypeId(const SimpleTypeId &, ToOcamlData *);
value ocaml_from_TypeIntr(const TypeIntr &, ToOcamlData *);
value ocaml_from_AccessKeyword(const AccessKeyword &, ToOcamlData *);
value ocaml_from_OverloadableOp(const OverloadableOp &, ToOcamlData *);
value ocaml_from_UnaryOp(const UnaryOp &, ToOcamlData *);
value ocaml_from_EffectOp(const EffectOp &, ToOcamlData *);
value ocaml_from_BinaryOp(const BinaryOp &, ToOcamlData *);
value ocaml_from_CastKeyword(const CastKeyword &, ToOcamlData *);


// Variable, CType hack
value ocaml_from_Variable(const Variable &, ToOcamlData *);
value ocaml_from_CType(const CType &, ToOcamlData *);


// value ocaml_from_(const  &, ToOcamlData *);

#endif // CC_OCAML_H
