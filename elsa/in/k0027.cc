// reprSize of a sizeless array

// originally found in package krb5

typedef int S[1];
const S array[2] = {};

int foo()
{
    int size = sizeof(array);
}
