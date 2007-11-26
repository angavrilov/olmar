// conversions

short f(short s) {
  long l;
  l = s;
  return l;
}

long g(long l) {
  return f(l);
}
