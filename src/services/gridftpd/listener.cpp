#include <arpa/inet.h>

#include <globus_common.h>
#include <globus_ftp_control.h>
#include <globus_gsi_credential.h>
#include <globus_openssl.h>

#include <arc/Logger.h>

#include "run/run.h"
#include "fileroot.h"
#include "commands.h"
#include "conf.h"

#define DEFAULT_MAX_BUFFER_SIZE (10*65536)
#define DEFAULT_BUFFER_SIZE (65536)
#define DEFAULT_MAX_CONECTIONS (100)
#define DEFAULT_GRIDFTP_PORT 2811
#define DEFAULT_LOG_FILE "/var/log/gridftpd.log"
#define DEFAULT_PID_FILE "/var/run/gridftpd.pid"

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
    logger.msg(Arc::ERROR, "Activation failed");
    globus_module_deactivate_all();
    close(sock);
    exit(1);
  };
  run.reinit(false);

  client = new GridFTP_Commands(getpid(),firewall_interface);    
  client->new_connection_callback((void*)client,sock);
  close(sock);
  logger.msg(Arc::INFO, "Child exited");
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
    logger.msg(Arc::ERROR, "Globus connection error"); return;
  };
  logger.msg(Arc::INFO, "New connection");
  client = new GridFTP_Commands(cur_connections,firewall_interface);
  client->new_connection_callback((void*)client,handle,error);
}
#endif

void serv_stop_callback(void* arg,globus_ftp_control_server_t *handle,globus_object_t *error) {
  logger.msg(Arc::INFO, "Server stopped");
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
    logger.msg(Arc::ERROR, "Error: failed to set handler for SIGTERM");
    return -1;
  };
  for(;;) {
    logger.msg(Arc::INFO, "Starting controlled process");
    if((chid=fork()) != 0) {
      if(chid == -1) {
        logger.msg(Arc::ERROR, "fork failed");
        return -1;
      };
      int status;
      if(wait(&status) == -1) {
        logger.msg(Arc::ERROR, "wait failed - killing child");
        kill(chid,SIGKILL); return -1;
      };
      logger.msg(Arc::INFO, "Child exited");
      if(WIFSIGNALED(status)) {
        logger.msg(Arc::INFO, "Killed with signal: "<<(int)(WTERMSIG(status)));
        if(WTERMSIG(status) == SIGSEGV) {
          logger.msg(Arc::INFO, "Restarting after segmentation violation.");
          logger.msg(Arc::INFO, "Waiting 1 minute");
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

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::INFO);

#ifndef __DONT_USE_FORK__
  signal(SIGTERM,&sig_term_fork);
  sig_old_chld=signal(SIGCHLD,&sig_chld);
  if(sig_old_chld == SIG_ERR) {
    perror("");
    logger.msg(Arc::ERROR, "Error: failed to set handler for SIGCHLD");
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
      case ':': { logger.msg(Arc::ERROR, "Missing argument"); return 1; };
      case '?': { logger.msg(Arc::ERROR, "Unknown option"); return 1; };
      case 'h': {
        fprintf(stdout,"gridftpd [-p port_to_listen] [-c config_file] [-n maximal_connections] [-b default_buffer_size] [-B maximal_buffer_size] %s.\n",daemon.short_help()); 
         return 0;
      }; 
      case 'p': {
        if(sscanf(optarg,"%hu",&server_port) != 1) {
          logger.msg(Arc::ERROR, "Wrong port number");
          return 1;
        };
      }; break;
      case 'c': {
        config_file=optarg;
      }; break;
      case 'n': {
        if((sscanf(optarg,"%i",&max_connections) != 1) ||
           (max_connections < 0)) {
          logger.msg(Arc::ERROR, "Wrong number of connections");
          return 1;
        };
      }; break;
      case 'b': {
        if((sscanf(optarg,"%Lu",&default_data_buffer_size) != 1) ||
           (default_data_buffer_size < 1)) {
          logger.msg(Arc::ERROR, "Wrong buffer size");
          return 1;
        };
      }; break;
      case 'B': {
        if((sscanf(optarg,"%Lu",&max_data_buffer_size) != 1) ||
           (max_data_buffer_size < 1)) {
          logger.msg(Arc::ERROR, "Wrong maximal buffer size");
          return 1;
        };
      }; break;
      default: break;
    };
  };

  //if(config_file) nordugrid_config_loc=config_file;
  // Read configuration (for daemon commands and port)
  FileRoot::ServerParams params;
  if(FileRoot::config(daemon,&params) != 0) {
    logger.msg(Arc::ERROR, "Failed reading configuration");
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
    logger.msg(Arc::ERROR, "Failed to create socket"); exit(-1);
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
    logger.msg(Arc::ERROR, "bind failed"); exit(-1);
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
  logger.msg(Arc::INFO, "Listen started");
  for(;;) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int sock = accept(handle,(sockaddr*)&addr,&addrlen);
    if(sock == -1) {
      if(!server_done) logger.msg(Arc::ERROR, "Accept failed: %s", strerror(errno));
      break;
    };
    logger.msg(Arc::INFO, "Have connections: %i, max: %i", curr_connections, max_connections);
    if((curr_connections < max_connections) || (max_connections == 0)) {
      logger.msg(Arc::INFO, "New connection");
      switch (fork()) {
        case -1: {
          logger.msg(Arc::ERROR, "Fork failed: %s", strerror(errno));
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
      logger.msg(Arc::ERROR, "Refusing connection: Connection limit exceeded");
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
    logger.msg(Arc::ERROR, "Activation failed");
    globus_module_deactivate_all();
    goto exit;
  };
  if(globus_ftp_control_server_handle_init(&handle) != GLOBUS_SUCCESS) {
    logger.msg(Arc::ERROR, "Init failed"); goto exit_active;
  };
  if(globus_ftp_control_server_listen(&handle,&server_port,&new_conn_callback,NULL) != GLOBUS_SUCCESS) {
    logger.msg(Arc::ERROR, "Listen failed"); goto exit_inited;
  };

  logger.msg(Arc::INFO, "Listen started");

  globus_mutex_init(&server_lock,GLOBUS_NULL);
  globus_cond_init(&server_cond,GLOBUS_NULL);
  server_done=0;
  globus_mutex_lock(&(server_lock));
  while(!(server_done)) {
    globus_cond_wait(&(server_cond),&(server_lock));
  };
  globus_mutex_unlock(&(server_lock));

  logger.msg(Arc::INFO, "Listen finished");

  globus_mutex_destroy(&server_lock);
  globus_cond_destroy(&server_cond);

  logger.msg(Arc::INFO, "Stopping server");

  globus_ftp_control_server_stop(&handle,&serv_stop_callback,NULL);
exit_inited:
  logger.msg(Arc::INFO, "Destroying handle");
  globus_ftp_control_server_handle_destroy(&handle);
exit_active:
  logger.msg(Arc::INFO, "Deactivating modules");
  globus_module_deactivate(GLOBUS_OPENSSL_MODULE);
  globus_module_deactivate(GLOBUS_GSI_GSS_ASSIST_MODULE);
  globus_module_deactivate(GLOBUS_GSI_CREDENTIAL_MODULE);
  globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
  globus_module_deactivate(GLOBUS_COMMON_MODULE);
exit:
#endif
  logger.msg(Arc::INFO, "Exiting");
  return 0;
}
