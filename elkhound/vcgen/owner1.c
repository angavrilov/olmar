// owner1.c
// experiments with owner/serf
              
#define NULLOWNER 0
#define DEAD 1
#define OWNING 2

struct OwnerPtrMeta {      // name is special
  // I think I want to model owner ptrs as tuples generally, so
  // here's the pointer part
  int *ptr;

  // when I see <obj>.<field>, I need to connect <field> with some
  // declared field; so this struct will provide the declaration
  // I need
  int state;
};

int offset(int *ptr);
int *object(int *ptr);
int length(int *obj);

int * owner allocFunc()
  thmprv_post(result.ptr != (int*)0 &&
              offset(result.ptr) == 0 &&
              length(object(result.ptr)) == 4 &&
              result.state == OWNING);

void deallocFunc(int * owner q)
  thmprv_pre(q.state == OWNING);

int main()
{
  int * owner p;
  p.state = DEAD;
  thmprv_assert(p.state == DEAD);

  //p = (int * owner)0;
  p.ptr = (int *)0;
  p.state = NULLOWNER;

  thmprv_assert(p.state != OWNING);    // otherwise leak
  p = allocFunc();
  //p.ptr = allocFunc();
  //p.state = OWNING;

  // use
  thmprv_assert(p.state == OWNING);
  // *p = 6;
  *(p.ptr) = 6;

  thmprv_assert(p.state == OWNING);
  //int x = *p;
  int x = *(p.ptr);

  deallocFunc(p);
  p.state = DEAD;

  // function return
  thmprv_assert(p.state != OWNING);    // otherwise leak
  return 0;
}




