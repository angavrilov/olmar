// static inline function implicitly returning int, old-style param decl

// originally found in package xview

// a.i:2:5: Parse error (state 254) at int

extern func1(param)
    int param;
{
    return param;
}

inline func2(param)
    int param;
{
    return param;
}
