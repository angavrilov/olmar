// This example from cqual/examples.
$tainted char *getenv(const char *name);
int printf($untainted const char *fmt, ...);

int main(void)
{
  // this fails unless -DDISTINCT_CVATOMIC_TYPES is on when built.
  char *s, *t;
  s = getenv("LD_LIBRARY_PATH");
  t = s;
  printf(t);
}
