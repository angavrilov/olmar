// treeadd-merged.c
// treeadd, all in one file

//#include <stdio.h>      // stderr, printf
//#include <stdarg.h>     // varargs stuff
//#include <stdlib.h>     // malloc



// ------------- declarations for my predicates/functions -------
thmprv_predicate int okSelOffsetRange(int mem, int offset, int len);
thmprv_predicate int okSelOffset(int mem, int offset);
thmprv_predicate int freshObj(int *obj, int *mem);
int *sub(int index, int *rest);
int firstIndexOf(int *ptr);
int *appendIndex(int *ptr, int index);
thmprv_predicate int okSel(int mem, int index);

thmprv_predicate int treeNode(int mem, int addr);


// -------- stuff from headers ------------
struct FILE {
  int whatever;
};

extern struct FILE *stderr;


extern int level;    // needed for printf postcondition..

int printf(char const *fmt, ...)
  thmprv_pre(
    int pre_mem = mem;
    int pre_level = level;
    okSelOffset(mem, fmt)
  )
  thmprv_post(pre_mem == mem && pre_level == level)
;


int fprintf(struct FILE *dest, char const *fmt, ...)
  thmprv_pre(int pre_mem=mem;
    okSelOffset(mem, fmt)
  )
  thmprv_post(pre_mem == mem)
;


void *malloc(int size)
  thmprv_pre(int *pre_mem = mem; size >= 0)
  thmprv_post(
    thmprv_exists(
      int address;
      // the returned pointer is a toplevel address only
      address != 0 &&
      result == sub(address, 0 /*whole*/) &&
      // pointer points to new object
      freshObj(address, mem) &&
      // of at least 'size' bytes
      okSelOffsetRange(mem, appendIndex(result, 0), size) &&
      // and does not modify anything reachable from pre_mem
      pre_mem == mem
    )
  );


void free(void *block);


extern int atoi(char const *src)
  thmprv_pre(
    int pre_mem = mem;
    okSelOffset(mem, src)     // just check that first char is readable..
  )
  thmprv_post(
    pre_mem == mem            // no side effects
  )
;


void exit(int code)
  thmprv_post(false)
;


// ===================== tree.h ==========================
/* For copyright information, see olden_v1.0/COPYRIGHT */

/* tree.h
 */

typedef struct tree {
    int		val;
    struct tree *left, *right;
} tree_t;

//extern tree_t *TreeAlloc (int level);

extern int level;
extern int iters;


// ======================== args.c =======================
void filestuff()
  thmprv_pre(int pre_mem=mem; true)
  thmprv_post(pre_mem == mem)
{}

int level;     // must be >= 0
int iters;

int dealwithargs(int argc, char **argv)
  thmprv_pre(
    okSelOffsetRange(mem, argv, argc) &&
    thmprv_forall(int i; 0<=i && i<argc ==> okSelOffset(mem, argv[i]))
  )
  thmprv_post(
    level >= 0
  )
{
  if (argc > 2)
    iters = atoi(argv[2]);
  else
    iters = 1;

  if (argc > 1)
    level = atoi(argv[1]);
  else
    level = 5;

  if (level < 0) {
    printf("level must be >= 0; you gave %d\n", level);
    exit(2);
  }

  return level;
}


// ======================= par-alloc.c ======================
// HACK: my vcgen has a bug if I call the same procedure I'm in,
// and this prototype is enough separation to work around it
tree_t *TreeAlloc (int level)
  thmprv_pre(int pre_mem=mem;
    level >= 0
  )
  thmprv_post(
    // the returned pointer is a toplevel address only
    result == sub(firstIndexOf(result), 0 /*whole*/) &&
    // and that address can be called a tree node
    treeNode(mem, firstIndexOf(result)) &&
    // if we don't return NULL..
    (result != (tree_t*)0 ==> (
      // pointer points to new object
      freshObj(firstIndexOf(result), mem) &&
      // at least as big as a tree_t
      okSelOffsetRange(mem, appendIndex(result, 0), sizeof(tree_t))
    )) &&
    // and does not modify anything reachable from pre_mem
    pre_mem == mem
  )
;

tree_t *TreeAlloc (int level)
  thmprv_pre(int pre_mem=mem;
    level >= 0
  )
  thmprv_post(
    // the returned pointer is a toplevel address only
    result == sub(firstIndexOf(result), 0 /*whole*/) &&
    // and that address can be called a tree node
    treeNode(mem, firstIndexOf(result)) &&
    // if we don't return NULL..
    (result != (tree_t*)0 ==> (
      // pointer points to new object
      freshObj(firstIndexOf(result), mem) &&
      // at least as big as a tree_t
      okSelOffsetRange(mem, appendIndex(result, 0), sizeof(tree_t))
    )) &&
    // and does not modify anything reachable from pre_mem
    pre_mem == mem
  )
{

  if (level == 0)
    {
      return (tree_t*)0;
    }
  else
    {
      struct tree *newp, *right, *left;

      newp = (struct tree *) malloc(sizeof(tree_t));
      left = TreeAlloc(level-1);
      right = TreeAlloc(level-1);
      newp->val = 1;
      newp->left = left;
      newp->right = right;

      // this one lemma is needed to prove the postcondition..      
      thmprv_assert(okSel(mem, firstIndexOf(newp)));
      thmprv_assume(okSel(mem, firstIndexOf(newp)));

      return newp;
    }
}


// ======================== node.c =======================
int TreeAdd (tree_t *t)
  thmprv_pre(
    int pre_mem = mem; 
    treeNode(mem, firstIndexOf(t)) &&
    t == sub(firstIndexOf(t), 0 /*whole*/)
  )
  thmprv_post(
    mem == pre_mem
  )
;

int TreeAdd (tree_t *t)
  thmprv_pre(
    int pre_mem = mem;
    treeNode(mem, firstIndexOf(t)) &&
    t == sub(firstIndexOf(t), 0 /*whole*/)
  )
  thmprv_post(
    mem == pre_mem
  )
{
  if (t == (tree_t*)0)
    {
      return 0;
    }
  else
    {
      int leftval;
      int rightval;
      tree_t *tleft, *tright;
      int value;

      tleft = t->left;
      leftval = TreeAdd(tleft);
      tright = t->right;
      rightval = TreeAdd(tright);

      value = t->val;
      return leftval + rightval + value;
    }
}


int main (int argc, char **argv)
  thmprv_pre(
    okSelOffsetRange(mem, argv, argc) &&
    thmprv_forall(int i; 0<=i && i<argc ==> okSelOffset(mem, argv[i]))
  )
{
    tree_t	*root;
    int i, result = 0;

    filestuff();
    (void)dealwithargs(argc, argv);

    printf("Treeadd with %d levels\n", level);

    printf("About to enter TreeAlloc\n");
    root = TreeAlloc (level);
    printf("About to enter TreeAdd\n");

    for (i = 0; i < iters; i++)
      {
        thmprv_invariant(
          treeNode(mem, firstIndexOf(root)) &&
          root == sub(firstIndexOf(root), 0 /*whole*/)
        );
	fprintf(stderr, "Iteration %d...", i);
	result = TreeAdd (root);
	fprintf(stderr, "done\n");
      }

    printf("Received result of %d\n",result);
    return 0;
}



