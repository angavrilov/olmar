// taking the address of a string (?!)

// originally found in package psys

// error: cannot take address of non-lvalue `char [5]'

int main()
{
    &"blah";
}
