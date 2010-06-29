#include <arpa/inet.h>

#include <globus_common.h>
#include <globus_ftp_control.h>
#include <globus_gsi_credential.h>
#include <globus_openssl.h>

#include <arc/Logger.h>

#include "fileroot.h"
#include "commands.h"
//#include "../config/daemon.h"
//#include "../misc/log_time.h"
#include "run/run.h"

#include "conf.h"

#define DEFAULT_MAX_BUFFER_SIZE (10*65536)
#define DEFAULT_BUFFER_SIZE (65536)
#define DEFAULT_MAX_CONECTIONS (100)
#define DEFAULT_GRIDFTP_PORT 2811
#define DEFAULT_LOG_FILE "/var/log/gridftpd.log"
#define DEFAULT_PID_FILE "/var/run/gridftpd.pid"

#define oilog(int) std::cerr

GridFTP_Commands *client;
static int max_connections = 0;
static volatile int curr_connections = 0;
unsigned long long int max_data_buffer_size = 0;
unsigned long long int default_data_buffer_size = 0;
unsigned int firewall_interface[4] = { 0, 0, 0, 0 };

static Arc::Logger logger(Arc::Logger::getRootLogger(), "gridftpd");

/* new connection */
#ifndef __DONT_USE_FORK__
void new_conn_callback(int sock) {
  /* set signal handlers to run child processes */
  gridftpd::Run run; // run.reinit();
  /* initiate random number generator */
  srand(getpid() + getppid() + time(NULL));
  if((globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_FTP_CONTROL_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_GSI_CREDENTIAL_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_GSI_GSS_ASSIST_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_OPENSSL_MODULE) != GLOBUS_SUCCESS)) {
    olog<<"Activation failed\n";
    globus_module_deactivate_all();
    close(sock);
    exit(1);
  };
  run.reinit(false);

  client = new GridFTP_Commands(getpid(),firewall_interface);    
  client->new_connection_callback((void*)client,sock);
  close(sock);
  oilog(getpid())<<"Child exited"<<std::endl;
  _exit(0);
  globus_module_deactivate(GLOBUS_OPENSSL_MODULE);
  globus_module_deactivate(GLOBUS_GSI_GSS_ASSIST_MODULE);
  globus_module_deactivate(GLOBUS_GSI_CREDENTIAL_MODULE);
  globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
  globus_module_deactivate(GLOBUS_COMMON_MODULE);
  exit(0);
}
#else
void new_conn_callback(void* arg,globus_ftp_control_server_t *handle,globus_object_t *error) {
  if(error != GLOBUS_SUCCESS) {
    olog<<"Globus connection error\n"; return;
  };
  olog<<"New connection\n";
  client = new GridFTP_Commands(cur_connections,firewall_interface);
//  olog<<"New connection - calling client\n";
  client->new_connection_callback((void*)client,handle,error);
}
#endif

void serv_stop_callback(void* arg,globus_ftp_control_server_t *handle,globus_object_t *error) {
  olog<<"Server stoped\n";
}

static volatile int chid = -1;
static volatile int server_done = 0;
static void (*sig_old_chld)(int) = SIG_ERR;

void sig_chld(int signum) {
  int status;
  for(;;) {
    int id=waitpid(-1,&status,WNOHANG);
    if((id == 0) || (id == -1)) break;
    curr_connections--;
    if(curr_connections < -10) curr_connections=-10;
  };
}

#ifdef __USE_RESURECTION__
void sig_term(int signum) {
// olog<<"SIGTERM detected"<<std::endl;
  if(chid == -1) return;
  if(chid == 0) {
    server_done = 1;
    globus_cond_signal(&server_cond);
    if(sig_old_term == SIG_ERR) return;
    if(sig_old_term == SIG_IGN) return;
    if(sig_old_term == SIG_DFL) return;
    (*sig_old_term)(signum);
  }
  else {
    kill(chid,SIGTERM);
  };
};

int main_internal(int argc,char** argv);

int main(int argc,char** argv) {
  globus_module_deactivate_all();
  setpgrp();
  sig_old_term=signal(SIGTERM,&sig_term);
  if(sig_old_term == SIG_ERR) {
    perror("");
    olog<<"Error: failed to set handler for SIGTERM"<<std::endl;
    return -1;
  };
  for(;;) {
    olog<<"Starting controlled process"<<std::endl;
    if((chid=fork()) != 0) {
      if(chid == -1) {
        olog<<"fork failed"<<std::endl;
        return -1;
      };
      int status;
      if(wait(&status) == -1) {
        olog<<"wait failed - killing child"<<std::endl;
        kill(chid,SIGKILL); return -1;
      };
      olog<<"Child exited"<<std::endl;
      if(WIFSIGNALED(status)) {
        olog<<"Killed with signal: "<<(int)(WTERMSIG(status))<<std::endl;
        if(WTERMSIG(status) == SIGSEGV) {
          olog<<"Restarting after segmentation violation."<<std::endl;
          olog<<"Waiting 1 minute"<<std::endl;
          sleep(60);
          continue;
        };
      };
      return WEXITSTATUS(status);
    };
    break;
  };
  return main_internal(argc,argv);
}

int main_internal(int argc,char** argv) {
#else
void sig_term_fork(int signum) {
  int static passed = 0;
  if(passed) return;
// olog<<"SIGTERM detected"<<std::endl;
  server_done=1;
  passed=1;
  kill(0,SIGTERM);
};

int main(int argc,char** argv) {
#ifndef __DONT_USE_FORK__
  globus_module_deactivate_all();
#endif
  setpgrp();
#endif

#ifndef __DONT_USE_FORK__
  signal(SIGTERM,&sig_term_fork);
  sig_old_chld=signal(SIGCHLD,&sig_chld);
  if(sig_old_chld == SIG_ERR) {
    perror("");
    olog<<"Error: failed to set handler for SIGCHLD"<<std::endl;
    return -1;
  };

  int handle;
  struct sockaddr_in myaddr;
#else
  globus_ftp_control_server_t handle;
  /* initiate random number generator */
  srand(getpid() + getppid() + time(NULL));
#endif
  unsigned short server_port=0;
  int n;
  gridftpd::Daemon daemon;

  while((n=daemon.getopt(argc,argv,"hp:c:n:b:B:")) != -1) {
    switch(n) {
      case '.': { return 1; };
      case ':': { olog<<"Missing argument\n"; return 1; };
      case '?': { olog<<"Unknown option\n"; return 1; };
      case 'h': {
        fprintf(stdout,"gridftpd [-p port_to_listen] [-c config_file] [-n maximal_connections] [-b default_buffer_size] [-B maximal_buffer_size] %s.\n",daemon.short_help()); 
         return 0;
      }; 
      case 'p': {
        if(sscanf(optarg,"%hu",&server_port) != 1) {
          olog<<"Wrong port number\n";
          return 1;
        };
      }; break;
      case 'c': {
        config_file=optarg;
      }; break;
      case 'n': {
        if((sscanf(optarg,"%i",&max_connections) != 1) ||
           (max_connections < 0)) {
          olog<<"Wrong number of connections\n";
          return 1;
        };
      }; break;
      case 'b': {
        if((sscanf(optarg,"%Lu",&default_data_buffer_size) != 1) ||
           (default_data_buffer_size < 1)) {
          olog<<"Wrong buffer size\n";
          return 1;
        };
      }; break;
      case 'B': {
        if((sscanf(optarg,"%Lu",&max_data_buffer_size) != 1) ||
           (max_data_buffer_size < 1)) {
          olog<<"Wrong maximal buffer size\n";
          return 1;
        };
      }; break;
      default: break;
    };
  };
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  //if(config_file) nordugrid_config_loc=config_file;
  // Read configuration (for daemon commands and port)
  FileRoot::ServerParams params;
  if(FileRoot::config(daemon,&params) != 0) {
    olog<<"Failed reading configuration\n";
    return 1;
  };
  if(server_port == 0) server_port=params.port;
  if(server_port == 0) server_port=DEFAULT_GRIDFTP_PORT;
  if(max_connections == 0) max_connections=params.max_connections;
  if(max_connections == 0) max_connections=DEFAULT_MAX_CONECTIONS;
  if(max_data_buffer_size == 0) max_data_buffer_size=params.max_buffer;
  if(max_data_buffer_size == 0) max_data_buffer_size=DEFAULT_MAX_BUFFER_SIZE;
  if(default_data_buffer_size == 0) default_data_buffer_size=params.default_buffer;
  if(default_data_buffer_size == 0) default_data_buffer_size=DEFAULT_BUFFER_SIZE;
  firewall_interface[0]=params.firewall[0];
  firewall_interface[1]=params.firewall[1];
  firewall_interface[2]=params.firewall[2];
  firewall_interface[3]=params.firewall[3];


#ifndef __DONT_USE_FORK__
  if((handle=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) == -1) {
    olog<<"Failed to create socket\n"; exit(-1);
  };
  {
    int on = 1;
    setsockopt(handle,SOL_SOCKET,SO_REUSEADDR,(void*)(&on),sizeof(on));
  };
  memset(&myaddr,0,sizeof(myaddr));
  myaddr.sin_family=AF_INET;
  myaddr.sin_port=htons(server_port);
  myaddr.sin_addr.s_addr=INADDR_ANY;
  if(bind(handle,(struct sockaddr *)&myaddr,sizeof(myaddr)) == -1) {
    olog<<"bind failed\n"; exit(-1);
  };
  daemon.logfile(DEFAULT_LOG_FILE);
  daemon.pidfile(DEFAULT_PID_FILE);
  if(daemon.daemon(false) != 0) {
    perror("daemonization failed");
    return 1;
  };
  if(listen(handle,128) == -1) {
    perror("listen failed");
    return 1;
  };
  olog<<"Listen started\n";
  for(;;) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int sock = accept(handle,(sockaddr*)&addr,&addrlen);
    if(sock == -1) {
      if(!server_done) olog<<"Accept failed: "<<strerror(errno)<<std::endl;
      break;
    };
    olog<<"Have connections: "<<curr_connections<<", max: "<<max_connections<<std::endl;
    if((curr_connections < max_connections) || (max_connections == 0)) {
      olog<<"New connection"<<std::endl;
      switch (fork()) {
        case -1: {
          olog<<"Fork failed: "<<strerror(errno)<<std::endl;
        }; break;
        case 0: {
          /* child */
          close(handle);
          new_conn_callback(sock);
        }; break;
        default: {
          /* parent */
          curr_connections++;
        }; break;
      };
    } else {
      /* it is probaly better to close connection immediately */
      olog<<"Refusing connection: Connection limit exceeded"<<std::endl;
    };
    close(sock);
  };
  close(handle);
#else
  if(daemon.daemon() != 0) {
    perror("daemonization failed");
     return 1;
  };
  Run run;
  if((globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_FTP_CONTROL_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_GSI_CREDENTIAL_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_GSI_GSS_ASSIST_MODULE) != GLOBUS_SUCCESS) ||
     (globus_module_activate(GLOBUS_OPENSSL_MODULE) != GLOBUS_SUCCESS)) {
    olog<<"Activation failed\n";
    globus_module_deactivate_all();
    goto exit;
  };
  if(globus_ftp_control_server_handle_init(&handle) != GLOBUS_SUCCESS) {
    olog<<"Init failed\n"; goto exit_active;
  };
  if(globus_ftp_control_server_listen(&handle,&server_port,&new_conn_callback,NULL) != GLOBUS_SUCCESS) {
    olog<<"Listen failed\n"; goto exit_inited;
  };

  olog<<"Listen started\n";

  globus_mutex_init(&server_lock,GLOBUS_NULL);
  globus_cond_init(&server_cond,GLOBUS_NULL);
  server_done=0;
  globus_mutex_lock(&(server_lock));
  while(!(server_done)) {
    globus_cond_wait(&(server_cond),&(server_lock));
  };
  globus_mutex_unlock(&(server_lock));

  olog<<"Listen finished\n";

  globus_mutex_destroy(&server_lock);
  globus_cond_destroy(&server_cond);

  olog<<"Stopping server\n";

  globus_ftp_control_server_stop(&handle,&serv_stop_callback,NULL);
exit_inited:
  olog<<"Destroying handle\n";
  globus_ftp_control_server_handle_destroy(&handle);
exit_active:
  olog<<"Deactivating modules\n";
  globus_module_deactivate(GLOBUS_OPENSSL_MODULE);
  globus_module_deactivate(GLOBUS_GSI_GSS_ASSIST_MODULE);
  globus_module_deactivate(GLOBUS_GSI_CREDENTIAL_MODULE);
  globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
  globus_module_deactivate(GLOBUS_COMMON_MODULE);
exit:
#endif
  olog<<"Exiting\n";
  return 0;
}
