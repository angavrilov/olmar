// treeadd-merged.c
// treeadd, all in one file

//#include <stdio.h>      // stderr, printf
//#include <stdarg.h>     // varargs stuff
//#include <stdlib.h>     // malloc

struct FILE {
  int whatever;
};

extern struct FILE *stderr;
int printf(char const *fmt, ...);
int fprintf(struct FILE *dest, char const *fmt, ...);

void *malloc(int size);
void *calloc(int size, int nelts);
void free(void *block);

extern int atoi(char const *src);
void exit(int code);



// ================= ssplain.h ======================
void * ssplain_malloc(int size);
void * ssplain_calloc(int nelems, int size);
void ssplain_alloc_stats();

/* All these functions */
//#pragma boxvararg_printf("chatting", 1)
//void chatting(char *s, ...);


// ===================== tree.h ==========================
/* For copyright information, see olden_v1.0/COPYRIGHT */

/* tree.h
 */

typedef struct tree {
    int		val;
    struct tree *left, *right;
} tree_t;

extern tree_t *TreeAlloc (int level);

extern int level;
extern int iters;


// ======================== args.c =======================
void filestuff()
{}

int level;
int iters;

int dealwithargs(int argc, char *argv[])
{
  if (argc > 2)
    iters = atoi(argv[2]);
  else
    iters = 1;

  if (argc > 1)
    level = atoi(argv[1]);
  else
    level = 5;

  return level;
}


// ======================== node.c =======================
int dealwithargs(int argc, char *argv[]);

typedef struct {
    long 	level;
} startmsg_t;

int TreeAdd (tree_t *t);
extern tree_t *TreeAlloc (int level);

int main (int argc, char *argv[])
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
        thmprv_invariant(true);
	fprintf(stderr, "Iteration %d...", i);
	result = TreeAdd (root);
	fprintf(stderr, "done\n");
      }

    printf("Received result of %d\n",result);
    return 0;
}

/* TreeAdd:
 */
int TreeAdd (tree_t *t)
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
} /* end of TreeAdd */


// ======================= par-alloc.c ======================
tree_t *TreeAlloc (int level)
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
      right=TreeAlloc(level-1);
      newp->val = 1;
      newp->left = (struct tree *) left;
      newp->right = (struct tree *) right;
      return newp;
    }
}


// ======================== ssplain.c ====================

#ifndef BEFOREBOX
static unsigned long bytes_allocated = 0;
static unsigned long allocations = 0;

void*
ssplain_malloc(int size)
{
  allocations++;
  bytes_allocated+=size;
  return malloc(size);
}

void*
ssplain_calloc(int nelems, int size)
{
  void *p;
  allocations++;
  bytes_allocated+= nelems * size;
  p =  calloc(nelems, size);
  if(! p) { printf("Cannot allocate\n"); exit(3); }
  return p;
}

void
ssplain_alloc_stats()
{
  printf("Allocation stats: %ld bytes allocated in %ld allocations\n",
         bytes_allocated, allocations);
}
#endif


