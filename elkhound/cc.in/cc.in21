// cc.in21
// demonstrate variable vs. class namespace

class Gronk {
};

//typedef class Gronk Gronk;    // implicit
//ERROR3: typedef class Whammy Gronk;    // implicit

int main()
{
  int Gronk;

  class Gronk *g;      // -> type
  return Gronk;        // -> int
}


typedef int x;
//ERROR1: typedef int x;
//ERROR2: typedef double x;

