// undefined instanceless unions repeating members

// originally found in package buildtool

// error: duplicate definition for `i' of type `int'

int main()
{
    union {int i;};
    union {int i;};
}
