// arrays of no size are assumed to have size one in gcc, but not g++

int a[];
int f() {
  sizeof a;                     // thought it seems to not tolerate this; whatever
}
int a[3];
int g() {
  sizeof a;
}
