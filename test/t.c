#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/select.h>
#include <stdio.h>

char * buf[1024 * 1024];

int main(){
  fd_set a,b,c;
  struct pollfd ufds[2];
  struct timeval t1, t2;
  int i, res;
  long fd_flags;

  fd_flags = fcntl(0, F_GETFL);
  if(fd_flags == -1)
    {
      perror("read descriptor flags");
      exit(1);
    }
  if(fcntl(0, F_SETFL, fd_flags | O_NONBLOCK) == -1)
    {
      perror("write descriptor flags");
      exit(1);
    }


  ufds[0].fd = 0;
  ufds[0].events = POLLIN | POLLPRI;
  ufds[1].fd = 1;
  ufds[1].events = POLLOUT;

  gettimeofday(&t1, NULL);
  for(i = 0; i < 100000; i++){
    

    /* 
     * res = poll(ufds, 2, 0);
     * if(__builtin_expect(res < 0, 0)){
     *   perror("poll");
     *   exit(1);
     * }
     */


    FD_ZERO(&a);
    FD_ZERO(&b);
    FD_ZERO(&c);
    /* FD_SET(0,&a); */
    FD_SET(1,&b);
    
    res = select(2, &a, &b, &c, NULL);
    if(res < 0){
      perror("select");
      exit(1);
    }

    /* 
     * res = read(0, buf, 100);
     * if(res < 0 && errno != EAGAIN){
     *   perror("read");
     *   exit(1);
     * }
     */
      
  }
  gettimeofday(&t2, NULL);
  printf("time %ld.%06ld : %s\n", t1.tv_sec, t1.tv_usec,
	 strerror(errno));

  printf("time %ld.%06ld : %s\n", t2.tv_sec, t2.tv_usec,
	 strerror(errno));
  printf("diff %ld.%06ld\n", 
	 t2.tv_sec - t1.tv_sec - (t2.tv_usec >= t1.tv_usec ? 0 : 1),
	 t2.tv_usec - t1.tv_usec + (t2.tv_usec >= t1.tv_usec ? 0 : 1000000)
	 );

  return 0;
}


 
