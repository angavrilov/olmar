#ifndef CC_QUAL_DUMMY_H
#define CC_QUAL_DUMMY_H

// Header file for things re-defined in cc_qual and not generated in
// cc.ast.  Daniel Wilkerson dsw@cs.berkeley.edu

// dummy global context for a cc_qual-walk
class QualEnv {};

void init_cc_qual(char *config_file);
void finish_cc_qual();

#endif // CC_QUAL_DUMMY_H
