// t0334.cc
// initialization by conversion
// from qt/src/kernel/qfont_x11.cpp

class QChar {
};

class QCharRef {
public:
  operator QChar ();
};

void foo()
{
  QCharRef r;
  QChar c = r;
}
