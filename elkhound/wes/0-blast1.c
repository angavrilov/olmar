int id(int w)
{
    return w;
}
int inc2(int w)
{
    return w+2;
} 
int inc3(int w) 
{
    return w+3;
} 

main()
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
        assert(0); 

    return 0;

}
