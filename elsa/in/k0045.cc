// 'operator delete' redeclared without throw()

// originally found in package drscheme_1:208-1

// a.ii:3:6: error: prior declaration of `operator delete' at <init>:1:1 had
// type `void ()(void *p) throw()', but this one uses `void ()(void
// */*anon*/)'

void operator delete(void *) {
}
