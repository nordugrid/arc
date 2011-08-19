#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

static int Error(int code, const char *info) {
  printf("RESULT\n");
  printf("info:%s\n", info);
  printf("code:%d\n", code);
  return code;
}

int main(int argv, char** argc) {

  struct stat st;

  int rfd, sfd, stdinfd;
  FILE *rf, *sf, *stdinf;
  fd_set fs;

  char buf[2048];
  char replyfile[] = "/tmp/giis-fifo.XXXXXX";
  char stdinfile[] = "/tmp/stdinfile.XXXXXX";

  struct timeval timelimit = { 5, 0 };

  const char* infile;

  long flags;

  int counter;

  signal(SIGPIPE, SIG_IGN);

  if(argv != 2)
    return Error(53, "Wrong number of arguments");

  infile = argc[1];

  if(stat(infile, &st) != 0)
    return Error(53, "Could not stat server FIFO");

  if(!S_ISFIFO(st.st_mode))
    return Error(53, "Server FIFO is not a FIFO");

  stdinfd = mkstemp(stdinfile);
  if (stdinfd == -1)
    return Error(53, "Could not create temporary file for stdin");
  stdinf = fdopen(stdinfd, "w+");
  if (!stdinf) {
    close(stdinfd);
    unlink(stdinfile);
    return Error(53, "Could not open temporary file for stdin");
  }

  flags = fcntl(fileno(stdin), F_GETFL);
  fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);

  counter = 0;
  do {
    if(fgets(buf, 2048, stdin)) {
      fputs(buf, stdinf);
      counter = 0;
    }
    else if(errno == EAGAIN) {
      usleep(1000);
      counter = counter + 1;
      if (counter >= 1000) {
	fclose(stdinf);
	unlink(stdinfile);
	return Error(53, "Timeout while reading stdin");
      }
      continue;
    }
    else if(!feof(stdin)) {
      fclose(stdinf);
      unlink(stdinfile);
      return Error(53, "I/O Error while reading stdin");
    }
  } while(!feof(stdin));

  rewind(stdinf);

  if(*mktemp(replyfile) == '\0') {
    fclose(stdinf);
    unlink(stdinfile);
    return Error(53, "Could not create reply FIFO filename");
  }

  if(mkfifo(replyfile, S_IRUSR | S_IWUSR) == -1) {
    fclose(stdinf);
    unlink(stdinfile);
    return Error(53, "Could not create reply FIFO");
  }

  sfd = open(infile, O_WRONLY | O_NONBLOCK);
  if (sfd == -1) {
    fclose(stdinf);
    unlink(stdinfile);
    unlink(replyfile);
    return Error(53, "Could not open server FIFO");
  }
  if(lockf(sfd, F_LOCK, 0) != 0) {
    fclose(stdinf);
    unlink(stdinfile);
    close(sfd);
    unlink(replyfile);
    return Error(53, "Could not lock server FIFO");
  }
  sf = fdopen(sfd, "w");
  if (!sf) {
    fclose(stdinf);
    unlink(stdinfile);
    if(lockf(sfd, F_ULOCK, 0) != 0) {}
    close(sfd);
    unlink(replyfile);
    return Error(53, "Could not open server FIFO");
  }

  rfd = open(replyfile, O_RDONLY | O_NONBLOCK);
  if (rfd == -1) {
    fclose(stdinf);
    unlink(stdinfile);
    if(lockf(sfd, F_ULOCK, 0) != 0) {}
    fclose(sf);
    unlink(replyfile);
    return Error(53, "Could not open reply FIFO");
  }
  rf = fdopen(rfd, "r");
  if (!rf) {
    fclose(stdinf);
    unlink(stdinfile);
    if(lockf(sfd, F_ULOCK, 0) != 0) {}
    fclose(sf);
    close(rfd);
    unlink(replyfile);
    return Error(53, "Could not open reply FIFO");
  }

  fprintf(sf, "%s\n", replyfile);

  while(fgets(buf, 2048, stdinf)) {
    fputs(buf, sf);
    if(strncasecmp(buf, "timelimit:", 10) == 0) {
      timelimit.tv_sec = atoi(&buf[10]);
      if(timelimit.tv_sec == 0 || timelimit.tv_sec > 20)
	timelimit.tv_sec = 20;
    }
  }

  fclose(stdinf);
  unlink(stdinfile);

  fflush(sf);

  if(lockf(sfd, F_ULOCK, 0) != 0) {}
  fclose(sf);

  FD_ZERO(&fs);
  FD_SET(rfd, &fs);

  if(select(rfd + 1, &fs, NULL, NULL, &timelimit) <= 0) {
    fclose(rf);
    unlink(replyfile);
    return Error(3, "Time out waiting for reply");
  }

  do {
    if(fgets(buf, 2048, rf))
      fputs(buf, stdout);
    else if(errno == EAGAIN) {
      usleep(1000);
      continue;
    }
    else if(!feof(rf)) {
      fclose(rf);
      unlink(replyfile);
      return Error(53, "I/O Error while waiting for reply");
    }
  } while(!feof(rf));

  fclose(rf);
  unlink(replyfile);

  return 0;
}
