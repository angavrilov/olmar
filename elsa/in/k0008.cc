// addresses of labels and goto

// error: Parse error (state 499) at &&

int main()
{
    void *a = &&x;
    goto *a;
x:
    return 0;
}
