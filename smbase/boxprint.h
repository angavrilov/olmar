// boxprint.h
// another pretty-printing module, this one based on the box model
// described at http://caml.inria.fr/FAQ/format-eng.html

#ifndef BOXPRINT_H
#define BOXPRINT_H

#include "str.h"          // stringBuilder
#include "astlist.h"      // ASTList
#include "array.h"        // ObjArrayStack
  

// manages the process of rendering a boxprint tree to a string
class BPRender {
public:
  // output string
  stringBuilder sb;

  // right margin column; defaults to 72
  int margin;

  // column for next output; equal to the number of characters
  // after the last newline in 'sb'
  int curCol;

public:
  BPRender();
  ~BPRender();

  // chars in the current line
  int getCurCol() const { return curCol; }

  // chars remaining on current line before the margin; might
  // be negative if the input didn't have enough breaks
  int remainder() const { return margin - getCurCol(); }

  // add some text (that doesn't include newlines) to the output
  void add(char const *text);
  
  // add a newline, plus indentation to get to column 'ind'
  void breakLine(int ind);

  // take the string out of the rendering engine, replacing it
  // with the empty string
  string takeString() {
    string ret(sb);
    sb.clear();
    return ret;
  }
};


// interface for elements in a boxprint tree
class BPElement {
public:
  // if no breaks are taken, compute the # of columns
  virtual int oneLineWidth()=0;

  // render this element as a string with newlines, etc.
  virtual void render(BPRender &mgr)=0;

  // true if this element is a BPBreak and is enabled; returns false
  // by default
  virtual bool isBreak() const;

  // deallocate the element
  virtual ~BPElement();
};


// leaf in the tree: text to print
class BPText : public BPElement {
public:
  string text;

public:
  BPText(char const *t);
  ~BPText();

  // BPElement funcs
  virtual int oneLineWidth();
  virtual void render(BPRender &mgr);
};


// leaf in the tree: a "break", which might end up being a
// space or a newline+indentation
class BPBreak : public BPElement {
public:
  // When true, this is a conditional break, and whether it is taken
  // or not depends on the prevailing break strategy of the box in
  // which it is located.  When false, the break is never taken, so
  // this is effectively just a space.
  bool enabled;

  // Nominally, when a break is taken, the indentation used is such
  // that the next char in the box is directly below the first char
  // in the box.  When this break is passed, however, it can add to
  // that nominal indent of 0; these adjustments accumulate as the
  // box is rendered.
  int indent;

public:
  BPBreak(bool e, int i);
  ~BPBreak();

  // BPElement funcs
  virtual int oneLineWidth();
  virtual void render(BPRender &mgr);
  virtual bool isBreak() const;
};


// kinds of boxes
enum BPKind {
  // enabled breaks are always taken
  BP_vertical,

  // enabled breaks are individually taken or not taken depending
  // on how much room is available; "hov"
  BP_sequence,

  // either all enabled breaks are taken, or none are taken; "h/v"
  BP_correlated,

  // # of kinds, also used to signal the end of a box in some cases
  NUM_BPKINDS
};

// internal node in the tree: a list of subtrees, some of which
// may be breaks
class BPBox : public BPElement {
public:
  // subtrees
  ASTList<BPElement> elts;

  // break strategy for this box
  BPKind kind;

public:
  BPBox(BPKind k);
  ~BPBox();

  // BPElement funcs
  virtual int oneLineWidth();
  virtual void render(BPRender &mgr);
};


// assists in the process of building a box tree by providing
// a number of syntactic shortcuts
class BPBuilder {
public:      // types
  // additional command besides BPKind
  enum Cmd {
    C_SPACE,       // insert disabled break
    C_BREAK,       // insert enabled break
  };

  // insert enabled break with indentation
  struct IBreak {
    int indent;
    IBreak(int i) : indent(i) {}
    // use default copy ctor
  };
  
  // operator sequence
  struct Op {
    char const *text;
    Op(char const *t) : text(t) {}
    // default copy ctor
  };

private:     // data
  // stack of open boxes; always one open vert box at the top
  ObjArrayStack<BPBox> boxStack;

public:      // data
  // convenient names for the box kinds
  static BPKind const vert;       // = BP_vertical
  static BPKind const seq;        // = BP_sequence
  static BPKind const hv;         // = BP_correlated ("h/v")
  static BPKind const end;        // = NUM_BPKINDS

  // names for additional commands
  static Cmd const sp;            // = C_SPACE
  static Cmd const br;            // = C_BREAK

private:     // funcs
  // innermost box being built
  BPBox *box() { return boxStack.top(); }

public:      // funcs
  BPBuilder();
  ~BPBuilder();

  // append another element to the current innermost box
  void append(BPElement *elt);

  // add BPText nodes to current box
  BPBuilder& operator<< (int i);
  BPBuilder& operator<< (char const *s);

  // open/close boxes
  BPBuilder& operator<< (BPKind k);

  // insert breaks
  BPBuilder& operator<< (Cmd c);

  // insert break with indentation
  IBreak ibr(int i) { return IBreak(i); }
  BPBuilder& operator<< (IBreak b);

  // op(text) is equivalent to sp << text << br
  Op op(char const *text) { return Op(text); }
  BPBuilder &operator << (Op o);

  // take the accumulated box tree out; all opened boxes must have
  // been closed; the builder is left in a state where it can be used
  // to build a new tree if desired, or it can be simply destroyed
  BPBox* /*owner*/ takeTree();
};


#endif // BOXPRINT_H
