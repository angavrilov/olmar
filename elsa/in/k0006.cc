// error: E_alignofType is not constEval'able

enum E {
    a = __alignof__(int)
};
