// xml_type_id.h            see license.txt for copyright and terms of use

// identity management for serialization for the type system, template
// system, and variables.

#ifndef XML_TYPE_ID_H
#define XML_TYPE_ID_H

#include "xml_writer.h"         // identity_decl
#include "cc_type.h"            // types
#include "variable.h"           // variables

identity_decl(Type);
identity_decl(AtomicType);
identity_decl(CompoundType);
identity_decl(FunctionType::ExnSpec);
identity_decl(EnumType::Value);
identity_decl(BaseClass);
identity_decl(Scope);
identity_decl(Variable);
identity_decl(OverloadSet);
identity_decl(STemplateArgument);
identity_decl(TemplateInfo);
identity_decl(InheritedTemplateParams);

#endif
