// cc_tcheck.h            see license.txt for copyright and terms of use
// Functions exported by cc_tcheck.cc

// TODO: make this a member of Env, and delete this file

#ifndef CC_TCHECK_H
#define CC_TCHECK_H

Type *computeArraySizeFromCompoundInit(Env &env, SourceLoc tgt_loc, Type *tgt_type,
                                       Type *src_type, Initializer *init);

#endif // CC_TCHECK_H