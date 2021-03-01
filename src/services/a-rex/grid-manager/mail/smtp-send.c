/*
   Simple program to mail information piped to stdin to address 'to' 
  from address 'from'. It tries to connect directly to SMTP server 
  responsible for destination address.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#include <resolv.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#define SMTP_PORT 25

typedef union {
  HEADER hdr;
  unsigned char buf[8192];
} answer_t;

void usage(void) {
  fprintf(stdout,"smtp-send from to\n");
  exit(1);
}

int send_mail(char* mail_server ,char* mail_from,char* mail_to) {
  char buf[256];
  int s,i;
  FILE* S;
  int err_code;
  char my_hostname[256];
  struct addrinfo *res = NULL;
  struct addrinfo *r = NULL;

  memset(my_hostname,0,256);  
  gethostname(my_hostname,255);
  if(getaddrinfo(mail_server,NULL,NULL,&res) != 0) return 2;
  if(res == NULL) return 2;
  for(r=res;r;r=r->ai_next) {
    if(r->ai_addr == NULL) continue;
    if(r->ai_socktype != SOCK_STREAM) continue;
    if(r->ai_protocol != IPPROTO_TCP) continue;
    if(r->ai_family == AF_INET) {
      ((struct sockaddr_in*)(r->ai_addr))->sin_port=htons(SMTP_PORT);
      break;
    };
    if(r->ai_family == AF_INET6) {
      ((struct sockaddr_in6*)(r->ai_addr))->sin6_port=htons(SMTP_PORT);
      break;
    };
  };
  if(!r) {
    freeaddrinfo(res);
    return 2;
  };
  s=socket(r->ai_family,r->ai_socktype,r->ai_protocol);
  if(s==-1) {
    freeaddrinfo(res);
    return 2;
  };
  if(connect(s,r->ai_addr,r->ai_addrlen)==-1) {
    freeaddrinfo(res);
    close(s);
    return 2;
  };
  freeaddrinfo(res);
  if((S=fdopen(s,"r+")) == NULL) {
    close(s);
    return 2;
  };
  if(fscanf(S,"%i%*[^\n]",&err_code) != 1) { fclose(S); return 2; }; fgetc(S);
  if( err_code != 220 ) { fclose(S); return 2; };
  fprintf(S,"HELO %s\r\n",my_hostname); fflush(S);
  if(fscanf(S,"%i%*[^\n]",&err_code) != 1) { fclose(S); return 2; }; fgetc(S);
  if( err_code != 250 ) { fclose(S); return 2; };
  fprintf(S,"MAIL FROM: <%s>\r\n",mail_from); fflush(S);
  if(fscanf(S,"%i%*[^\n]",&err_code) != 1) { fclose(S); return 2; }; fgetc(S);
  if( err_code != 250 ) { fclose(S); return 2; };
  fprintf(S,"RCPT TO: <%s>\r\n",mail_to); fflush(S);
  if(fscanf(S,"%i%*[^\n]",&err_code) != 1) { fclose(S); return 2; }; fgetc(S);
  if( err_code != 250 ) { fclose(S); return 2; };
  fprintf(S,"DATA\r\n"); fflush(S);
  if(fscanf(S,"%i%*[^\n]",&err_code) != 1) { fclose(S); return 2; }; fgetc(S);
  if( err_code != 354 ) { fclose(S); return 2; };
  /* read from stdin and send to socket */
  for(;;) {
    buf[0]=0;
    if((i=fscanf(stdin,"%255[^\n]",buf)) == EOF) break; 
    if(fscanf(stdin,"%*[^\n]") > 0) {}; fgetc(stdin);
    if(!strcmp(".",buf)) { fputc(' ',S); };
    fprintf(S,"%s\r\n",buf); fflush(S);
  };
  fprintf(S,".\r\n"); fflush(S);
  if(fscanf(S,"%i%*[^\n]",&err_code) != 1) { fclose(S); return 1; }; fgetc(S);
  if( err_code != 250 ) { fclose(S); return 1; };
  fprintf(S,"QUIT\r\n"); fflush(S);
  fclose(S);
  return 0;
}

int connect_mail(char* domain,char* mail_from,char* mail_to) { 
  char mxbuf[1024];
  unsigned short mxtype;
  unsigned short mxpref;
  answer_t answer;
  int l,na,nq;
  unsigned char *sp;
  unsigned char *cp;
  unsigned char *ep;
  HEADER *hp;
  int err_code = 2;

  fprintf(stdout,"Searching for domain %s\n",domain);
  if((l=res_search(domain,C_IN,T_MX,answer.buf,sizeof(answer))) == -1) {
    fprintf(stderr,"Query failed\n");
    return 2;
  };
  hp = &(answer.hdr);
  sp = answer.buf;
  cp = answer.buf + HFIXEDSZ;
  ep = answer.buf + l;
  nq=ntohs(hp->qdcount);
  for(;nq>0;nq--) {
    if((l=dn_skipname(cp,ep)) == -1) {
      fprintf(stderr,"skipname failed\n");
      return 2;
    };
    cp+=l+QFIXEDSZ;
  };
  na=ntohs(hp->ancount);
  for(;(na>0) && (cp<ep);na++) {
    if((l=dn_expand(sp,ep,cp,mxbuf,sizeof(mxbuf))) == -1) {
      fprintf(stderr,"expand failed\n");
      return 2;
    };
    cp+=l;
    GETSHORT(mxtype,cp);
    cp+=INT16SZ+INT32SZ;
    GETSHORT(l,cp);
    if(mxtype != T_MX) {
      cp+=l; continue;
    };
    GETSHORT(mxpref,cp);
    if((l=dn_expand(sp,ep,cp,mxbuf,sizeof(mxbuf))) == -1) {
      fprintf(stderr,"expand failed\n");
      return 2;
    };
    fprintf(stdout,"Trying to send through %s\n",mxbuf);
    if((err_code=send_mail(mxbuf,mail_from,mail_to)) == 0) {
      fprintf(stdout,"Sending finished\n");
      break;
    }; 
    fprintf(stdout,"Sending failed\n");
    cp+=l;
    if(err_code == 1) break;
  };
  if(err_code == 2) {
    fprintf(stdout,"Trying to send through %s\n",mxbuf);
    if((err_code=send_mail(domain,mail_from,mail_to)) == 0) {
      fprintf(stdout,"Sending finished\n");
    } else {
      fprintf(stdout,"Sending failed\n");
    };
  };
  return err_code;
}

int main(int argc,char** argv) {
  char* mail_from;
  char* mail_to;
  char* domain_rcpt;
  char* domain_snd;
  int err_code;
  
  if(argc != 3) { usage(); };
  mail_from=argv[1];  
  mail_to=argv[2];  

  domain_rcpt=strchr(mail_to,'@');
  if(domain_rcpt == NULL) {
    fprintf(stderr,"Bad recipient address (%s)\n", mail_to);
    exit(1);
  };
  domain_rcpt++;
  if(strlen(domain_rcpt) == 0) {
    fprintf(stderr,"Recepient address does not contain domain (%s)\n", mail_to);
    exit(1);
  };

  domain_snd=strchr(mail_from,'@');
  if(domain_snd != NULL) {
    domain_snd++;
    if(strlen(domain_snd) == 0) domain_snd=NULL;
  };

  if(res_init() == -1) {
    fprintf(stderr,"Failed to init resolver\n");
    exit(1);
  };

  err_code = connect_mail(domain_rcpt,mail_from,mail_to);
  if((err_code == 2) && (domain_snd != NULL)) {
    err_code = connect_mail(domain_snd,mail_from,mail_to);
  };
  if(err_code != 0) {
    fprintf(stderr,"Sending mail failed\n");
  };
  exit(err_code);
}


