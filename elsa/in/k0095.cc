// friend 'main' in a class

// originally found in package 'cccc'

// Assertion failed: prior->hasFlag(DF_EXTERN_C), file cc_env.cc line 3527
// Failure probably related to code near a.ii:4:14
// current location stack:
//   a.ii:4:14
//   a.ii:3:1

// ERR-MATCH: Assertion failed: prior->hasFlag[(]DF_EXTERN_C[)], file cc_env.cc line 3527

class C {
  friend int main(int argc, char** argv);
};
