// cc.in23
// exception specs on functions

int foo() throw();

int foo() throw()
{
  return 3;
}


class Exc;

void bar() throw(Exc);

//ERROR(1): void bar();    // conflicting declaration

