// paths.cc
// code for paths.h

#include "paths.h"       // this module

#include "c.ast.gen.h"   // C AST
#include "sobjlist.h"    // SObjList
#include "objlist.h"     // ObjList


// prototypes
void findPathRoots(SObjList<Statement /*const*/> &list, TF_func const *func);
void findPathRoots(SObjList<Statement /*const*/> &list, Statement const *stmt);
void printPaths(TF_func const *func);
void findPathsFrom(SObjList<Statement /*const*/> &path,
                   Statement const *node, bool isContinue);
void printPath(SObjList<Statement /*const*/> &path, char const *label);


void findPathRoots(SObjList<Statement /*const*/> &list, TF_func const *func)
{
  list.append(func->body);
  findPathRoots(list, func->body);
}


void findPathRoots(SObjList<Statement /*const*/> &list, Statement const *stmt)
{
  ASTSWITCHC(Statement, stmt) {
    ASTCASEC(S_label, l) {
      findPathRoots(list, l->s);
    }
    ASTNEXTC(S_case, c) {
      findPathRoots(list, c->s);
    }
    ASTNEXTC(S_caseRange, c) {
      findPathRoots(list, c->s);
    }
    ASTNEXTC(S_default, d) {
      findPathRoots(list, d->s);
    }
    ASTNEXTC(S_compound, c) {
      FOREACH_ASTLIST(Statement, c->stmts, iter) {
        findPathRoots(list, iter.data());
      }
    }
    ASTNEXTC(S_if, i) {
      findPathRoots(list, i->thenBranch);
      findPathRoots(list, i->elseBranch);
    }
    ASTNEXTC(S_switch, s) {
      findPathRoots(list, s->branches);
    }
    ASTNEXTC(S_while, w) {
      findPathRoots(list, w->body);
    }
    ASTNEXTC(S_doWhile, d) {
      findPathRoots(list, d->body);
    }
    ASTNEXTC(S_for, f) {
      findPathRoots(list, f->body);
    }
    ASTNEXTC(S_invariant, i) {
      list.append(const_cast<S_invariant*>(i));       // action!
    }
    ASTENDCASEC
  }
}


void printPaths(TF_func const *func)
{
  // enumerate the roots
  SObjList<Statement /*const*/> roots;
  findPathRoots(roots, func);

  // enumerate all paths from each root
  SFOREACH_OBJLIST(Statement, roots, iter) {
    SObjList<Statement> path;

    findPathsFrom(path, iter.data(), false /*isContinue*/);
  }
}

void findPathsFrom(SObjList<Statement /*const*/> &path,
                   Statement const *node, bool isContinue)
{
  if (path.contains(node)) {
    printPath(path, stringc << "CIRCULAR path back to " << (void*)node);
    return;
  }

  path.prepend(const_cast<Statement*>(node));

  if (node->kind() == Statement::S_INVARIANT &&
      path.count() > 1) {
    // we've reached an invariant point, so the path stops here
    printPath(path, "path to invariant");
  }

  else {
    // retrieve all successors of this node
    VoidList successors;
    node->getSuccessors(successors, isContinue);

    if (successors.isEmpty()) {
      printPath(path, "path to return");
    }
    else {
      // consider each choice
      for (VoidListIter iter(successors); !iter.isDone(); iter.adv()) {
        void *np = iter.data();
        findPathsFrom(path, nextPtrStmt(np), nextPtrContinue(np));
      }
    }
  }

  path.removeAt(0);
}

void printPath(SObjList<Statement /*const*/> &path, char const *label)
{
  cout << label << ":\n";
  path.reverse();
  SFOREACH_OBJLIST(Statement, path, iter) {
    cout << "  " << (void*)(iter.data()) << "\n";
  }
  path.reverse();
}














