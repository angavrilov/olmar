// blast1.c
// basically testing ability to see into functions

int id(int w)
  thmprv_post result == w;
{
    return w;
}
int inc2(int w)
  thmprv_post result == w+2;
{
    return w+2;
}
int inc3(int w)
  thmprv_post result == w+3;
{
    return w+3;
}

// I can't just change the 'assert' calls below to thmprv_assert,
// because the argument expression to thmprv_assert is interpreted
// as having function *symbols*, not function *applications*; this
// indirection means I evaluate applications before considering
// truth or falsity
void assert(int b)
  thmprv_pre b;
{}

int main()
{
    int x, y;

    x = 0;
    y = 1;

    x = x+1;

    assert(x == y);

    assert(x == id(x));

    assert(x+2 == inc2(x));

    assert(x+2 != inc3(x));

    x = y;
    x = x + 2;
    x = x + 3;

    assert(x == y+5);

    x = 0;

    if (x)
        assert(0);     // proved via inconsistent assumptions

    return 0;

}
