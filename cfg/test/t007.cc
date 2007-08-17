extern "C" {
int printf (__const char *__restrict __format, ...);
}

class Z {
public:
  Z(int i) { printf("Z(%d)\n", i); };
};

Z z[4] = { Z(1), Z(2), Z(3), Z(4) };

int main() {}
