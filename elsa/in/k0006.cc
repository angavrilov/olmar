// error: E_alignofType is not constEval'able

// originally found in package gettext

enum E {
    a = __alignof__(int)
};
