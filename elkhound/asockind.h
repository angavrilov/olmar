// asockind.h
// AssocKind; pulled out on its own so I don't have dependency problems

#ifndef ASOCKIND_H
#define ASOCKIND_H

#include "str.h"      // string

enum AssocKind { AK_LEFT, AK_RIGHT, AK_NONASSOC, NUM_ASSOC_KINDS };

string toString(AssocKind k);

#endif // ASOCKIND_H
