// prover.h
// abstract interface to a theorem prover

#ifndef PROVER_H
#define PROVER_H

// try to prove predicate represented as an sexp 'str', 
// and report success or failure
bool runProver(char const *str);

#endif // PROVER_H
