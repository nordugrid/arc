#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <globus_common.h>
#include <globus_ftp_control.h>
#include <globus_gsi_credential.h>
#include <globus_openssl.h>

#include <arc/Logger.h>
#include <arc/Run.h>
#include <arc/Utils.h>
#include <arc/StringConv.h>

#include "fileroot.h"
#include "commands.h"
#include "conf.h"

#define DEFAULT_MAX_BUFFER_SIZE (10*65536)
#define DEFAULT_BUFFER_SIZE (65536)
#define DEFAULT_MAX_CONECTIONS (100)
#define DEFAULT_GRIDFTP_PORT 2811
#define DEFAULT_LOG_FILE "/var/log/arc/gridftpd.log"
#define DEFAULT_PID_FILE "/run/gridftpd.pid"

GridFTP_Commands *client;
static int max_connections = 0;
static volatile int started_connections = 0;
static volatile int finished_connections = 0;
unsigned long long int max_data_buffer_size = 0;
unsigned long long int default_data_buffer_size = 0;
unsigned int firewall_interface[4] = { 0, 0, 0, 0 };

static Arc::Logger logger(Arc::Logger::getRootLogger(), "gridftpd");

#define PROTO_NAME(ADDR) ((ADDR->ai_family==AF_INET6)?"IPv6":"IPv4")

/* new connection */
#ifndef __DONT_USE_FORK__
void new_conn_callback(int sock) {
  /* initiate random number generator */
  srand(getpid() + getppid() + time(NULL));
#ifdef HAVE_GLOBUS_THREAD_SET_MODEL
  globus_thread_set_model("pthread");
#endif
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

void serv_stop_callback(void* /* arg */,globus_ftp_control_server_t* /* handle */,globus_object_t* /* error */) {
  logger.msg(Arc::INFO, "Server stopped");
}

static volatile int server_done = 0;
static void (*sig_old_chld)(int) = SIG_ERR;

void sig_chld(int /* signum */) {
  int old_errno = errno;
  int status;
  for(;;) {
    int id=waitpid(-1,&status,WNOHANG);
    if((id == 0) || (id == -1)) break;
    ++finished_connections;
  };
  errno = old_errno;
}

#ifdef __USE_RESURECTION__
void sig_term(int signum) {
  int old_errno = errno;
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
  errno = old_errno;
};

int main_internal(int argc,char** argv);

int main(int argc,char** argv) {
  char const * log_time_format = ::getenv("ARC_LOGGER_TIME_FORMAT");
  if(log_time_format) {
    if(strcmp(log_time_format,"USER") == 0) {
      Arc::Time::SetFormat(Arc::UserTime);
    } else if(strcmp(log_time_format,"USEREXT") == 0) {
      Arc::Time::SetFormat(Arc::UserExtTime);
    } else if(strcmp(log_time_format,"ELASTIC") == 0) {
      Arc::Time::SetFormat(Arc::ElasticTime);
    } else if(strcmp(log_time_format,"MDS") == 0) {
      Arc::Time::SetFormat(Arc::MDSTime);
    } else if(strcmp(log_time_format,"ASC") == 0) {
      Arc::Time::SetFormat(Arc::ASCTime);
    } else if(strcmp(log_time_format,"ISO") == 0) {
      Arc::Time::SetFormat(Arc::ISOTime);
    } else if(strcmp(log_time_format,"UTC") == 0) {
      Arc::Time::SetFormat(Arc::UTCTime);
    } else if(strcmp(log_time_format,"RFC1123") == 0) {
      Arc::Time::SetFormat(Arc::RFC1123Time);
    } else if(strcmp(log_time_format,"EPOCH") == 0) {
      Arc::Time::SetFormat(Arc::EpochTime);
    };
  };
  // temporary stderr destination until configuration is read and used in daemon.daemon()
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::INFO);

  globus_module_deactivate_all();
  setpgid(0,0);
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
void sig_term_fork(int /* signum */) {
  int old_errno = errno;
  int static passed = 0;
  if(passed) _exit(-1);
  server_done=1;
  passed=1;
  kill(0,SIGTERM);
  errno = old_errno;
}

int main(int argc,char** argv) {
#ifndef __DONT_USE_FORK__
  globus_module_deactivate_all();
#endif
  setpgid(0,0);
#endif
  char const * log_time_format = ::getenv("ARC_LOGGER_TIME_FORMAT");
  if(log_time_format) {
    if(strcmp(log_time_format,"USER") == 0) {
      Arc::Time::SetFormat(Arc::UserTime);
    } else if(strcmp(log_time_format,"USEREXT") == 0) {
      Arc::Time::SetFormat(Arc::UserExtTime);
    } else if(strcmp(log_time_format,"ELASTIC") == 0) {
      Arc::Time::SetFormat(Arc::ElasticTime);
    } else if(strcmp(log_time_format,"MDS") == 0) {
      Arc::Time::SetFormat(Arc::MDSTime);
    } else if(strcmp(log_time_format,"ASC") == 0) {
      Arc::Time::SetFormat(Arc::ASCTime);
    } else if(strcmp(log_time_format,"ISO") == 0) {
      Arc::Time::SetFormat(Arc::ISOTime);
    } else if(strcmp(log_time_format,"UTC") == 0) {
      Arc::Time::SetFormat(Arc::UTCTime);
    } else if(strcmp(log_time_format,"RFC1123") == 0) {
      Arc::Time::SetFormat(Arc::RFC1123Time);
    } else if(strcmp(log_time_format,"EPOCH") == 0) {
      Arc::Time::SetFormat(Arc::EpochTime);
    };
  };
  // temporary stderr destination until configuration is read and used in daemon.daemon()
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

  std::list<int> handles;
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
  unsigned int addrs_num = 0;
  {
    struct addrinfo hint;
    struct addrinfo *info = NULL;
    memset(&hint, 0, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP; // ?
    hint.ai_flags = AI_PASSIVE; // looking for bind'able adresses
    // hint.ai_family = AF_INET;
    // hint.ai_family = AF_INET6;
    int ret = getaddrinfo(NULL, Arc::tostring(server_port).c_str(), &hint, &info);
    if (ret != 0) {
      std::string err_str = gai_strerror(ret);
      logger.msg(Arc::ERROR, "Failed to obtain local address: %s",err_str); exit(-1);
    };
    for(struct addrinfo *info_ = info;info_;info_=info_->ai_next) {
      ++addrs_num;
      int s = socket(info_->ai_family,info_->ai_socktype,info_->ai_protocol);
      if(s == -1) {
        std::string e = Arc::StrError(errno);
        logger.msg(Arc::WARNING, "Failed to create socket(%s): %s",PROTO_NAME(info_),e);
      };
      {
        int on = 1;
        setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(void*)(&on),sizeof(on));
      };
#ifdef IPV6_V6ONLY
      if(info_->ai_family == AF_INET6) {
        int v = 1;
        // Some systems (Linux for example) make v6 support v4 too
        // by default. Some don't. Make it same for everyone -
        // separate sockets for v4 and v6.
        if(setsockopt(s,IPPROTO_IPV6,IPV6_V6ONLY,&v,sizeof(v)) != 0) {
          std::string e = Arc::StrError(errno);
          logger.msg(Arc::WARNING, "Failed to limit socket to IPv6: %s",e);
          close(s); continue;
        };
      };
#endif
      if(bind(s,info_->ai_addr,info_->ai_addrlen) == -1) {
        std::string e = Arc::StrError(errno);
        logger.msg(Arc::WARNING, "Failed to bind socket(%s): %s",PROTO_NAME(info_),e);
        close(s); continue;
      };
      if(listen(s,128) == -1) {
        std::string e = Arc::StrError(errno);
        logger.msg(Arc::WARNING, "Failed to listen on socket(%s): %s",PROTO_NAME(info_),e);
        close(s); continue;
      };
      handles.push_back(s);
    };
  };
  if(handles.empty()) {
    logger.msg(Arc::ERROR, "Not listening to anything"); exit(-1);
  };
  if(handles.size() < addrs_num) {
    logger.msg(Arc::WARNING, "Some addresses failed. Listening on %u of %u.",(unsigned int)handles.size(),addrs_num);
  };
  daemon.logfile(DEFAULT_LOG_FILE);
  daemon.pidfile(DEFAULT_PID_FILE);
  if(daemon.daemon(false) != 0) {
    perror("daemonization failed");
    return 1;
  };
  logger.msg(Arc::INFO, "Listen started");
  for(;;) {
    fd_set ifds;
    fd_set efds;
    FD_ZERO(&ifds);
    FD_ZERO(&efds);
    int maxfd = -1;
    for(std::list<int>::iterator handle = handles.begin();handle != handles.end();++handle) {
      FD_SET(*handle,&ifds);
      FD_SET(*handle,&efds);
      if(*handle > maxfd) maxfd = *handle;
    };
    if(maxfd < 0) {
      if(!server_done) logger.msg(Arc::ERROR, "No valid handles left for listening");
      break;
    };
    int r = select(maxfd+1,&ifds,NULL,&efds,NULL);
    if(r == -1) {
      if(errno == EINTR) continue;
      if(!server_done) logger.msg(Arc::ERROR, "Select failed: %s", Arc::StrError(errno));
      break;
    };
    std::list<int>::iterator handle = handles.begin();
    for(;handle != handles.end();++handle) {
      if(FD_ISSET(*handle,&ifds) || FD_ISSET(*handle,&efds)) break;
    };
    if(handle == handles.end()) { // ???
      continue; 
    };
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int sock = accept(*handle,(sockaddr*)&addr,&addrlen);
    if(sock == -1) {
      if(!server_done) logger.msg(Arc::ERROR, "Accept failed: %s", Arc::StrError(errno));
      if(errno == EBADF) { // handle becomes bad
        close(*handle);
        handles.erase(handle);
      };
    };
    int curr_connections = started_connections - finished_connections;
    logger.msg(Arc::INFO, "Have connections: %i, max: %i", curr_connections, max_connections);
    if((curr_connections < max_connections) || (max_connections == 0)) {
      logger.msg(Arc::INFO, "New connection");
      switch (fork()) {
        case -1: {
          logger.msg(Arc::ERROR, "Fork failed: %s", Arc::StrError(errno));
        }; break;
        case 0: {
          /* child */
          for(std::list<int>::iterator handle = handles.begin();handle != handles.end();++handle) {
            close(*handle);
          };
          handles.clear();
          Arc::Run::AfterFork();
          new_conn_callback(sock);
        }; break;
        default: {
          /* parent */
          ++started_connections;
        }; break;
      };
    } else {
      /* it is probaly better to close connection immediately */
      logger.msg(Arc::ERROR, "Refusing connection: Connection limit exceeded");
    };
    close(sock);
  };
  for(std::list<int>::iterator handle = handles.begin();handle != handles.end();++handle) {
    close(*handle);
  };
  handles.clear();
#else
  if(daemon.daemon() != 0) {
    perror("daemonization failed");
     return 1;
  };
#ifdef HAVE_GLOBUS_THREAD_SET_MODEL
  globus_thread_set_model("pthread");
#endif
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
