// "and" and "or" keywords

// originally found in package battleball

//  Parse error (state 910) at <name>: and

// ERR-MATCH: Parse error.*at.* and$

int main()
{
    return (true and false) or (false and true);
}
