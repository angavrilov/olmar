/* weimer
 * Wed Sep 26 14:09:47 PDT 2001
 *
 * example test for BLAST: can you handle variables
 * with similar names?
 *
 * ERROR should not be reachable
 */
// NUMERRORS 1
int bar(int x)          // 10
  thmprv_pre( int pre_x=x; true )
  thmprv_post( result == (pre_x*2)+1 )
{
    x = x * 2;          // 20
    return x+1;         // return 21
}

int foo(int x)          // 5
  thmprv_pre( int pre_x=x; true )
  thmprv_post( result == (pre_x*2)*2 + 1 + 1 )
{
    x = x * 2;          // 10
    x = bar(x);         // bar(10) = 21
    return x+1;         // return 22
}

int main()
{
    int x = 5;

    x = foo(x);         // foo(5) = 22

    thmprv_assert (x == 22);
    thmprv_assert (x == 23);    // ERROR(1)

    return 0;
}
