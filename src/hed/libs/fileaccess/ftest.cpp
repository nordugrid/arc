#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(void) {
  int sendpipe[2];
  int recvpipe[2];
  pipe(sendpipe);
  pipe(recvpipe);
  if(fork() == 0) {
    close(0);
    close(1);
    dup2(sendpipe[0],0);
    dup2(recvpipe[1],1);
    execl("./arc-file-access","arc-file-access","0","1",NULL);
    return -1;
  }
  int n;

  n=2*sizeof(n); write(sendpipe[1],&n,sizeof(n)); // size
  n=1; write(sendpipe[1],&n,sizeof(n)); // cmd
  n=1000; write(sendpipe[1],&n,sizeof(n)); // uid
  n=100; write(sendpipe[1],&n,sizeof(n)); // gid
  read(recvpipe[0],&n,sizeof(n)); if(n != 2*sizeof(n)) return -1;
  read(recvpipe[0],&n,sizeof(n)); if(n != 1) return -1;
  read(recvpipe[0],&n,sizeof(n)); if(n != 0) return -1; // res
  read(recvpipe[0],&n,sizeof(n)); if(n != 0) return -1; // err

  n=sizeof(mode_t)+sizeof(int)+12; write(sendpipe[1],&n,sizeof(n)); // size
  n=2; write(sendpipe[1],&n,sizeof(n)); // cmd
  mode_t m=0777; write(sendpipe[1],&m,sizeof(m)); // mode
  char s[] = "/tmp/testdir"; n=12; write(sendpipe[1],&n,sizeof(n)); write(sendpipe[1],s,12); // dir
  read(recvpipe[0],&n,sizeof(n)); if(n != 2*sizeof(n)) return -1;
  read(recvpipe[0],&n,sizeof(n)); if(n != 2) return -1;
  read(recvpipe[0],&n,sizeof(n)); if(n != 0) return -1; // res
  read(recvpipe[0],&n,sizeof(n)); if(n != 0) return -1; // err

  return 0;
}
