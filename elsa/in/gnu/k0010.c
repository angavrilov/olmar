// nested K&R function

// i/xalan_1.8-4/ElemApplyTemplates.cpp.8c892e8b0da809df12dd69739b9228a7.ii:28377:2:
// error: duplicate member declaration of `transformChild' in class
// xalanc_1_8::ElemApplyTemplates; previous at
// i/xalan_1.8-4/ElemApplyTemplates.cpp.8c892e8b0da809df12dd69739b9228a7.ii:28370:2

// Assertion failed: unimplemented: nested K&R function definition
// (d00aa531-ca12-4d72-9caf-ca9db9941179), file gnu.gr line 144

// ERR-MATCH: d00aa531-ca12-4d72-9caf-ca9db9941179

void foo (void) {
  void bar (x) int x;
  {
  }
}
