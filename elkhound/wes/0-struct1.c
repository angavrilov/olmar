struct bonus_evilness {
  int a;
  int b;
  int c;
} ;

struct qp_queue {
	unsigned long head;
	struct bonus_evilness evil;
	unsigned long tail;
};

struct qp_queue q1 = { 111, { 99, 88, 77}, 222 } ; 

int main() {
  struct qp_queue q2;

  struct qp_queue *qp;
  int x, y;
  int *ip;

  x = q1.evil.a;
  x = q1.evil.b;
  x = q1.evil.c;

  ip = &(q1.evil.b);
  ip[2] = ip[2] + ip[-1];

  x = q1.tail;

  assert(x == (222+99));

  return x;
}
