// taking the address of a string (?!)

// originally found in package psys

// error: cannot take address of non-lvalue `char [5]'

// ERR-MATCH: address of non-lvalue `char

int main()
{
    &"blah";
}
