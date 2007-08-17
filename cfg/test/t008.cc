extern "C" {
int printf (__const char *__restrict __format, ...);
}


class A {
public:
  A(int i) { printf("A(%d)\n", i); };
};

void * operator new(unsigned int size) throw() { 
  printf("op new A %d\n", size);
  return (void *)0;
}


int main() {
  A *a = new A(5);
  printf("a is %s\n",
	 (a == (void *)0) ? "null" : "non null");
}
