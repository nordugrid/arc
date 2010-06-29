#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <globus_common.h>
#include <globus_io.h>
#include <globus_ftp_control.h>

#include "fileroot.h"
#include "names.h"
#include "commands.h"
#include "misc/canonical_dir.h"
#include <arc/globusutils/GlobusErrorUtils.h>
#include "misc/proxy.h"

/* timeout if nothing happened during 10 minutes */
#define FTP_TIMEOUT 600

#define oilog(int) std::cerr

extern unsigned long long int max_data_buffer_size;
extern unsigned long long int default_data_buffer_size;

#ifndef __DONT_USE_FORK__
static int fork_done;
static globus_mutex_t fork_lock;
static globus_cond_t  fork_cond;
#endif

GridFTP_Commands_timeout* timeouter = NULL;

extern int make_list_string(const DirEntry &entr,GridFTP_Commands::list_mode_t mode,unsigned char* buf,int size,const char *prefix);

int GridFTP_Commands::send_response(const char* response) {
  globus_result_t res;
  response_done=0;
  {
    std::string s = response;
    for(std::string::size_type n=0;;)
      if((n=s.find('\r'))==std::string::npos) {break;} else {s[n]='\\';};
    for(std::string::size_type n=0;;)
      if((n=s.find('\n'))==std::string::npos) {break;} else {s[n]='\\';};
    oilog(log_id)<<"response: "<<s<<std::endl;
  };
  res = globus_ftp_control_send_response(&handle,response,&response_callback,this);
  if(res != GLOBUS_SUCCESS) {
    oilog(log_id)<<"Send response failed:"<<Arc::GlobusResult(res).str()<<'\n';
    globus_mutex_lock(&response_lock);
    response_done=2;
    globus_cond_signal(&response_cond);
    globus_mutex_unlock(&response_lock);
    return 1;
  };
  return 0;
}

int GridFTP_Commands::wait_response(void) {
  int  res;
  globus_abstime_t timeout;
  last_action_time=time(NULL);
  GlobusTimeAbstimeSet(timeout,0,100000);
  globus_mutex_lock(&response_lock);
  while(!response_done) {
    globus_cond_timedwait(&response_cond,&response_lock,&timeout);
    res=(response_done != 1);
    last_action_time=time(NULL);
  };
  response_done=0;
  globus_mutex_unlock(&response_lock);
  return res;
}

void GridFTP_Commands::response_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  globus_mutex_lock(&(it->response_lock));
  if(error != GLOBUS_SUCCESS) { 
    oilog(it->log_id)<<"Response sending error\n"; 
    it->response_done=2;
  }
  else {
    it->response_done=1;
  };
  globus_cond_signal(&(it->response_cond));
  globus_mutex_unlock(&(it->response_lock));
}

void GridFTP_Commands::close_callback(void *arg,globus_ftp_control_handle_t *handle,globus_object_t *error, globus_ftp_control_response_t *ftp_response) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  if(it) {
    oilog(it->log_id)<<"Closed connection\n";
    delete it;
  };
  // GridFTP_Commands::response_callback(arg,handle,error);
}

#if GLOBUS_IO_VERSION>=5
static void io_close_cb(void* callback_arg,globus_io_handle_t* handle,globus_result_t result) {
}
#endif


#ifndef __DONT_USE_FORK__

static void io_listen_cb(void* arg,globus_io_handle_t* handle,globus_result_t result) {
  globus_mutex_lock(&fork_lock);
  fork_done=1;
  globus_cond_signal(&fork_cond);
  globus_mutex_unlock(&fork_lock);
}

GridFTP_Commands::close_semaphor_t::close_semaphor_t(void) {
}

GridFTP_Commands::close_semaphor_t::~close_semaphor_t(void) {
  globus_mutex_lock(&fork_lock);
// oilog(it->log_id)<<"Signaling exit condition"<<std::endl;
  fork_done=1;
  globus_cond_signal(&fork_cond);
  globus_mutex_unlock(&fork_lock);
}

int GridFTP_Commands::new_connection_callback(void* arg,int sock) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;

  globus_mutex_init(&fork_lock,GLOBUS_NULL);
  globus_cond_init(&fork_cond,GLOBUS_NULL);

  // Convert the socket to a globus IO structure.
  globus_result_t res;
  globus_io_attr_t attr;
  globus_io_tcpattr_init(&attr);
  globus_io_attr_set_socket_oobinline(&attr, GLOBUS_TRUE);
  globus_io_attr_set_tcp_nodelay(&attr, GLOBUS_TRUE);
  res = globus_io_tcp_posix_convert(sock,&attr,&(it->handle.cc_handle.io_handle));
  if(res != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"Socket conversion failed: "<<Arc::GlobusResult(res)<<std::endl;
    return -1;
  };
  it->handle.cc_handle.cc_state=GLOBUS_FTP_CONTROL_CONNECTED;

  fork_done = 0;
  globus_ftp_control_local_prot(&(it->handle),GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
//  globus_ftp_control_local_prot(&(it->handle),GLOBUS_FTP_CONTROL_PROTECTION_CLEAR);
  it->data_dcau.mode=GLOBUS_FTP_CONTROL_DCAU_SELF;
  it->data_dcau.subject.subject=NULL;
  globus_ftp_control_local_dcau(&(it->handle),&(it->data_dcau),GSS_C_NO_CREDENTIAL);
  globus_ftp_control_local_mode(&(it->handle),GLOBUS_FTP_CONTROL_MODE_STREAM);
  globus_ftp_control_local_type(&(it->handle),GLOBUS_FTP_CONTROL_TYPE_IMAGE,0);

  // Call accept callback as if Globus called it
  accepted_callback(it, &(it->handle), GLOBUS_SUCCESS);

  globus_mutex_lock(&fork_lock);
  while(!fork_done) {
    globus_cond_wait(&fork_cond,&fork_lock);
  };
  globus_mutex_unlock(&fork_lock);

  globus_cond_destroy(&fork_cond);
  globus_mutex_destroy(&fork_lock);

  return 0;
}
#else
void GridFTP_Commands::new_connection_callback(void* arg,globus_ftp_control_server_t *server_handle,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  globus_ftp_control_local_prot(&(it->handle),GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
//  globus_ftp_control_local_prot(&(it->handle),GLOBUS_FTP_CONTROL_PROTECTION_CLEAR);
  it->data_dcau.mode=GLOBUS_FTP_CONTROL_DCAU_SELF;
  it->data_dcau.subject.subject=NULL;
  globus_ftp_control_local_dcau(&(it->handle),&(it->data_dcau),GSS_C_NO_CREDENTIAL);
  globus_ftp_control_local_mode(&(it->handle),GLOBUS_FTP_CONTROL_MODE_STREAM);
  globus_ftp_control_local_type(&(it->handle),GLOBUS_FTP_CONTROL_TYPE_IMAGE);
  if(globus_ftp_control_server_accept(server_handle,&(it->handle),&accepted_callback,it) != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"Accept failed\n";
  };
}
#endif

void GridFTP_Commands::accepted_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  if(error != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"Accept failed: "<<error<<std::endl;
    delete it;
    return;
  };
  int remote_host[4] = { 0, 0, 0, 0 };
  unsigned short remote_port = 0;
  globus_io_tcp_get_remote_address(&(handle->cc_handle.io_handle),remote_host,&remote_port);
  oilog(it->log_id)<<"Accepted connection from "<<
        (unsigned int)(remote_host[0])<<"."<<
        (unsigned int)(remote_host[1])<<"."<<
        (unsigned int)(remote_host[2])<<"."<<
        (unsigned int)(remote_host[3])<<":"<<remote_port<<std::endl;
  it->send_response("220 Server ready\r\n");
  if(globus_ftp_control_server_authenticate(&(it->handle),GLOBUS_FTP_CONTROL_AUTH_REQ_GSSAPI,&authenticate_callback,it) != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"Authenticate in commands failed\n";
    delete it; 
    return;
  };
}

void GridFTP_Commands::authenticate_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_ftp_control_auth_info_t *result) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  if((result == GLOBUS_NULL) || (error != GLOBUS_SUCCESS)) {
    oilog(it->log_id)<<"Authentication failure\n";
    oilog(it->log_id)<<error<<std::endl;
    if(it->send_response("535 Authentication failed\r\n") == 0) {
      it->wait_response();
    };
    delete it;
    return;
  };
  oilog(it->log_id)<<"User subject: "<<result->auth_gssapi_subject<<std::endl;
  oilog(it->log_id)<<"Encrypt: "<<(int)(result->encrypt)<<std::endl;
  it->delegated_cred=result->delegated_credential_handle;
//
//const char* fname = write_cert_chain(result->auth_gssapi_context);
//if(fname) std::cerr<<"Credentials chain stored in "<<fname<<std::endl;
//
  if(it->froot.config(result,handle) != 0) {
    oilog(it->log_id)<<"User has no proper configuration associated\n";
    if(it->send_response("535 Not allowed\r\n") == 0) {
      it->wait_response();
    };
    delete it;
    return;
  };
  if(it->froot.nodes.size() == 0) {
    oilog(it->log_id)<<"User has empty virtual directory tree.\nEither user has no authorised plugins or there are no plugins configured at all.\n";
    if(it->send_response("535 Nothing to serve\r\n") == 0) {
      it->wait_response();
    };
    delete it;
    return;
  };
  it->send_response("235 Authentication successful\r\n");
  /* Set defaults */
//  globus_ftp_control_local_prot(&(it->handle),GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
//  globus_ftp_control_local_prot(&(it->handle),GLOBUS_FTP_CONTROL_PROTECTION_CLEAR);
  it->data_dcau.mode=GLOBUS_FTP_CONTROL_DCAU_SELF;
  it->data_dcau.subject.subject=NULL;
  globus_ftp_control_local_dcau(&(it->handle),&(it->data_dcau),it->delegated_cred);
  globus_ftp_control_local_mode(&(it->handle),GLOBUS_FTP_CONTROL_MODE_STREAM);
  globus_ftp_control_local_type(&(it->handle),GLOBUS_FTP_CONTROL_TYPE_IMAGE,0);
  if(globus_ftp_control_read_commands(&(it->handle),&commands_callback,it) != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"Read commands in authenticate failed\n";
    delete it;
    return;
  };
}

int parse_integers(char* string,int args[],int margs) {
  char* cp = string;
  char* np;
  int n=0;
  if((*cp)==0) return n;
  for(;;) {
    np=cp; cp=strchr(np,',');
    if(cp!=NULL) { (*cp)=0; cp++; };
    if(n<margs) args[n]=atoi(np);
    n++; 
    if(cp==NULL) break;
  };
  return n;
}
 
int parse_semicolon(char* string,char* args[],int margs) {
  char* cp = string;
  int n=0;
  for(;;) {
    if(n<margs) args[n]=cp;
    cp=strchr(cp,';'); 
    if(cp==NULL) break;
    (*cp++)=0; n++; 
  };
  return n;
}

char* get_arg(char* string) {
  char* cp=strchr(string,' '); 
  if(cp==NULL) return NULL;
  for(;(*cp) == ' ';cp++);
  if(((*cp) == '\r') || ((*cp) == '\n') || ((*cp) == 0)) return NULL;
  char* np = cp+strcspn(cp,"\r\n");
  (*np)=0;
  return cp;
}

int parse_args(char* string,char* args[],int margs) {
  char* cp = string;
  char* np;
  int n=0;
  for(;;) {
    np=cp;
    cp=strchr(np,' '); 
    if(cp==NULL) cp=strchr(np,'\r'); 
    if(cp==NULL) cp=strchr(np,'\n');
    if(cp==NULL) break;
    for(;(*cp) == ' ';cp++) { if(n<margs) (*cp)=0; };
    if(((*cp) == '\r') || ((*cp) == '\n') || ((*cp) == 0)) { (*cp)=0; break; };
    if(n<margs) args[n]=cp; n++;
  };
  return n;
}

void fix_string_arg(char* s) {
  if(s == NULL) return;
  char* s_=strchr(s,'\n');
  if(s_ == NULL) return;
  (*s_)=0;
  s_=strchr(s,'\r');
  if(s_ == NULL) return;
  (*s_)=0;
}

#define CHECK_TRANSFER { \
  if(it->transfer_mode) { \
    it->send_response("421 Service not available\r\n"); break; \
  }; \
  /* Globus data handle may still be in CLOSING state because \
     it takes some time to close socket through globus_io. \
     Check for such situation and give Globus 10 sec. to recover. */ \
  time_t s_time = time(NULL); \
  while(true) { \
    if(handle->dc_handle.state != GLOBUS_FTP_DATA_STATE_CLOSING) break; \
    if(((unsigned int)(time(NULL)-s_time)) > 10) break;  \
    sleep(1); \
  }; \
  if(handle->dc_handle.state == GLOBUS_FTP_DATA_STATE_CLOSING) { \
    it->send_response("421 Timeout waiting for service to become available\r\n"); break; \
  }; \
}
  

/* main procedure */
void GridFTP_Commands::commands_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,union globus_ftp_control_command_u *command) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  it->last_action_time=time(NULL);
  if(command == GLOBUS_NULL) {
    oilog(it->log_id)<<"Control connection (probably) closed\n";
    if(error) {
      oilog(it->log_id)<<error<<std::endl;
    };
    it->make_abort();
    delete it;
    return;
  }
//  oilog(it->log_id)<<"Raw command: "<<command->base.raw_command<<std::endl;
#ifndef HAVE_FTP_COMMAND_MLSD
#define GLOBUS_FTP_CONTROL_COMMAND_MLSD \
    ((globus_ftp_control_command_code_t)(GLOBUS_FTP_CONTROL_COMMAND_UNKNOWN+1))
#define GLOBUS_FTP_CONTROL_COMMAND_MLST \
    ((globus_ftp_control_command_code_t)(GLOBUS_FTP_CONTROL_COMMAND_UNKNOWN+2))
  if(command->code == GLOBUS_FTP_CONTROL_COMMAND_UNKNOWN) {
    if(!strncasecmp("MLSD",command->base.raw_command,4)) {
      command->code=GLOBUS_FTP_CONTROL_COMMAND_MLSD;
      const char* arg = get_arg(command->base.raw_command);
      if(arg == NULL) { arg=""; };
      command->list.string_arg=(char*)arg;
    }
    else if(!strncasecmp("MLST",command->base.raw_command,4)) {
      command->code=GLOBUS_FTP_CONTROL_COMMAND_MLST;
      const char* arg = get_arg(command->base.raw_command);
      if(arg == NULL) { arg=""; };
      command->list.string_arg=(char*)arg;
    };
  };
#endif
  switch(command->code) {
    case GLOBUS_FTP_CONTROL_COMMAND_AUTH: {
      it->send_response("534 Reauthentication is not supported\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_FEAT: {
      it->send_response("211- Features supported\r\n FEAT\r\n AUTH\r\n ERET\r\n SBUF\r\n DCAU\r\n SPAS\r\n SPOR\r\n SIZE\r\n MDTM\r\n MLSD\r\n MLST\r\n211 End\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_USER: {
      fix_string_arg(command->user.string_arg);
  oilog(it->log_id)<<"Command USER "<<command->user.string_arg<<std::endl;
      it->send_response("230 No need for username\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_PASS: {
      it->send_response("230 No need for password\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_CDUP: {
  oilog(it->log_id)<<"Command CDUP\n";
      command->code=GLOBUS_FTP_CONTROL_COMMAND_CWD;
      command->cwd.string_arg=(char*)"..";
    };
    case GLOBUS_FTP_CONTROL_COMMAND_CWD: {
      fix_string_arg(command->cwd.string_arg);
  oilog(it->log_id)<<"Command CWD "<<command->cwd.string_arg<<std::endl;
      std::string pwd = command->cwd.string_arg;
      if(it->froot.cwd(pwd) == 0) {
        pwd = "250 \""+pwd+"\" is current directory\r\n";
        it->send_response(pwd.c_str());
      }
      else {
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 Can't change to this directory.\r\n");
        };
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_MKD: {
      fix_string_arg(command->mkd.string_arg);
  oilog(it->log_id)<<"Command MKD "<<command->mkd.string_arg<<std::endl;
      std::string pwd = command->mkd.string_arg;
      if(it->froot.mkd(pwd) == 0) {
        it->send_response("250 MKD command ok.\r\n");
      }
      else {
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 Can't make this directory.\r\n");
        };
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_PWD: {
      std::string pwd;
      pwd = "257 \""+(it->froot.cwd())+"\" is current directory\r\n";
      it->send_response(pwd.c_str());
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_SIZE: {
      fix_string_arg(command->size.string_arg);
  oilog(it->log_id)<<"Command SIZE "<<command->size.string_arg<<std::endl;
      unsigned long long size;
      if(it->froot.size(command->size.string_arg,&size) != 0) {
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 Size for object not available.\r\n");
        };
        break;
      };
      char buf[200]; sprintf(buf,"213 %llu\r\n",size);
      it->send_response(buf);
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_SBUF: {
      CHECK_TRANSFER;
  oilog(it->log_id)<<"Command SBUF: "<<command->sbuf.buffer_size<<std::endl;
      // Because Globus wants SBUF to apply for all following data 
      // connections, there is no way to reset to system defaults.
      // Let's make a little extension
      globus_ftp_control_tcpbuffer_t tcpbuf;
      if(command->sbuf.buffer_size == 0) {
        tcpbuf.mode=GLOBUS_FTP_CONTROL_TCPBUFFER_DEFAULT;
      } else if(command->sbuf.buffer_size < 0) {
        it->send_response("501 Wrong argument for SBUF\r\n"); break;
      } else {
        tcpbuf.mode=GLOBUS_FTP_CONTROL_TCPBUFFER_FIXED;
        tcpbuf.fixed.size=command->sbuf.buffer_size;
      };
      if(globus_ftp_control_local_tcp_buffer(&(it->handle),&tcpbuf) !=
                                                         GLOBUS_SUCCESS) {
        it->send_response("501 SBUF argument can't be accepted\r\n"); break;
      };
      it->send_response("200 Accepted TCP buffer size\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_MLST: {
      fix_string_arg(command->list.string_arg);
      oilog(it->log_id)<<"Command MLST "<<command->list.string_arg<<std::endl;
      DirEntry info;
      if(it->froot.checkfile(command->list.string_arg,info,DirEntry::full_object_info) != 0) {
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 Information for object not available\r\n");
        };
        break;
      };
      char buf[1024];
      const char* str1="250-Information follows\r\n ";
      const char* str2="250 Information finished\r\n";
      int str1l=strlen(str1);
      int str2l=strlen(str2);
      strcpy(buf,str1);
      make_list_string(info,list_mlsd_mode,(unsigned char*)(buf+str1l),
                       1024-str1l-str2l,"");
      strcat(buf,str2);
      it->send_response(buf);
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_DELE: {
      fix_string_arg(command->dele.string_arg);
      oilog(it->log_id)<<"Command DELE "<<command->dele.string_arg<<std::endl;
      std::string file = command->dele.string_arg;
      if(it->froot.rm(file) == 0) {
        it->send_response("250 File deleted.\r\n");
      }
      else {
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 Can't delete this file.\r\n");
        };
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_RMD: {
      fix_string_arg(command->rmd.string_arg);
      oilog(it->log_id)<<"Command RMD "<<command->rmd.string_arg<<std::endl;
      std::string dfile = command->rmd.string_arg;
      if(it->froot.rmd(dfile) == 0) {
        it->send_response("250 Directory deleted.\r\n");
      }
      else {
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 Can't delete this directory.\r\n");
        };
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_TYPE: {
  oilog(it->log_id)<<"Command TYPE "<<(char)(command->type.type)<<std::endl;
      CHECK_TRANSFER;
      if(command->type.type==GLOBUS_FTP_CONTROL_TYPE_NONE) {
        it->send_response("504 Unsupported type\r\n");
      }
      else {
        globus_ftp_control_local_type(&(it->handle),command->type.type,0);
        it->send_response("200 Type accepted\r\n");
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_MODE: {
  oilog(it->log_id)<<"Command MODE "<<(char)(command->mode.mode)<<std::endl;
      CHECK_TRANSFER;
      if((command->mode.mode!=GLOBUS_FTP_CONTROL_MODE_STREAM) &&
         (command->mode.mode!=GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK)) {
        it->send_response("504 Unsupported mode\r\n");
      }
      else {
        globus_ftp_control_local_mode(&(it->handle),command->mode.mode);
        it->send_response("200 Mode accepted\r\n");
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_ABOR: {
  oilog(it->log_id)<<"Command ABOR\n";
      globus_mutex_lock(&(it->abort_lock));
      if(!(it->transfer_mode)) { 
        globus_mutex_unlock(&(it->abort_lock));
        it->send_response("226 Abort not needed\r\n");
      }
      else {
        globus_mutex_unlock(&(it->abort_lock));
        it->make_abort();
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_REST: { /* for the beginning stream mode only */
      fix_string_arg(command->rest.string_arg);
  oilog(it->log_id)<<"Command REST "<<command->rest.string_arg<<std::endl;
      CHECK_TRANSFER;
      it->virt_restrict=false;
      if(sscanf(command->rest.string_arg,"%llu",&(it->virt_offset)) != 1) {
        it->virt_offset=0;
        it->send_response("501 Wrong parameter\r\n"); break;
      };
      it->send_response("350 Restore pointer accepted\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_SPAS:
    case GLOBUS_FTP_CONTROL_COMMAND_PASV: {
      oilog(it->log_id)<<"Command PASV/SPAS\n";
      CHECK_TRANSFER;
      globus_ftp_control_host_port_t node;
      memset(&node,0,sizeof(node));
      char buf[200];
      if(command->code == GLOBUS_FTP_CONTROL_COMMAND_PASV) {
        globus_result_t res_tmp;
        if((res_tmp=globus_ftp_control_local_pasv(&(it->handle),&node))
                                                             !=GLOBUS_SUCCESS){
          oilog(it->log_id)<<"local_pasv failed\n";
          oilog(it->log_id)<<Arc::GlobusResult(res_tmp)<<std::endl;
          it->send_response("553 Failed to allocate port for data transfer\r\n"); break;
        };
        if(it->firewall[0]) { // replace address
          node.host[0]=it->firewall[0];
          node.host[1]=it->firewall[1];
          node.host[2]=it->firewall[2];
          node.host[3]=it->firewall[3];
        };
        sprintf(buf,"227 Entering Passive Mode (%i,%i,%i,%i,%i,%i)\r\n",
        node.host[0], node.host[1], node.host[2], node.host[3],
        (node.port & 0x0FF00) >> 8,
        node.port & 0x000FF);
      }
      else {
        globus_result_t res_tmp;
        if((res_tmp=globus_ftp_control_local_spas(&(it->handle),&node,1))
                                                             !=GLOBUS_SUCCESS){
          oilog(it->log_id)<<"local_spas failed\n";
          oilog(it->log_id)<<Arc::GlobusResult(res_tmp)<<std::endl;
          it->send_response("553 Failed to allocate port for data transfer\r\n"); break;
        };
        if(it->firewall[0]) { // replace address
          node.host[0]=it->firewall[0];
          node.host[1]=it->firewall[1];
          node.host[2]=it->firewall[2];
          node.host[3]=it->firewall[3];
        };
        sprintf(buf,"229-Entering Passive Mode\r\n %i,%i,%i,%i,%i,%i\r\n229 End\r\n",
        node.host[0], node.host[1], node.host[2], node.host[3],
        (node.port & 0x0FF00) >> 8,
        node.port & 0x000FF);
      };
      it->data_conn_type=GRIDFTP_CONNECT_PASV;
      it->send_response(buf);
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_PORT: {
  oilog(it->log_id)<<"Command PORT\n";
      CHECK_TRANSFER;
      globus_ftp_control_host_port_t node;
      node=command->port.host_port;
      globus_result_t res_tmp = globus_ftp_control_local_port(&(it->handle),&node);
      if(res_tmp != GLOBUS_SUCCESS) {
        oilog(it->log_id)<<"local_port failed\n";
        oilog(it->log_id)<<Arc::GlobusResult(res_tmp)<<std::endl;
        it->send_response("553 Failed to accept port for data transfer\r\n");
        break;
      };
      it->data_conn_type=GRIDFTP_CONNECT_PORT;
      it->send_response("200 Node accepted\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_MLSD:
    case GLOBUS_FTP_CONTROL_COMMAND_NLST:
    case GLOBUS_FTP_CONTROL_COMMAND_LIST: {
      fix_string_arg(command->list.string_arg);
      if(command->code == GLOBUS_FTP_CONTROL_COMMAND_MLSD) {
        oilog(it->log_id)<<"Command MLSD "<<command->list.string_arg<<std::endl;
      } else if(command->code == GLOBUS_FTP_CONTROL_COMMAND_NLST) {
        oilog(it->log_id)<<"Command NLST "<<command->list.string_arg<<std::endl;
      } else {
        oilog(it->log_id)<<"Command LIST "<<command->list.string_arg<<std::endl;
      };
      CHECK_TRANSFER;
      DirEntry::object_info_level mode;
      if(command->code == GLOBUS_FTP_CONTROL_COMMAND_LIST) {
        it->list_mode=list_list_mode;
        mode=DirEntry::basic_object_info;
      }
      else if(command->code == GLOBUS_FTP_CONTROL_COMMAND_NLST) {
        it->list_mode=list_nlst_mode;
        mode=DirEntry::minimal_object_info;
      }
      else {
        it->list_mode=list_mlsd_mode;
        mode=DirEntry::full_object_info;
      };
      it->dir_list.clear();
      it->list_name_prefix="";
      int res = it->froot.readdir(command->list.string_arg,it->dir_list,mode);
      // 1 - error
      if(res == 1) { // error, most probably no such dir
        if(it->froot.error.length()) {
          it->send_response("450 "+it->froot.error+"\r\n"); break;
        } else {
          it->send_response("450 Object unavailable.\r\n"); break;
        };
      };
      // -1 - file
      if((res == -1) &&
         ( (it->list_mode == list_mlsd_mode) ||
           (it->list_mode == list_nlst_mode) ) ) {
        // MLSD and NLST are for directories only
        it->send_response("501 Object is not a directory.\r\n"); break;
      };
      // 0 - directory
      if(it->data_conn_type == GRIDFTP_CONNECT_PORT) {
        it->send_response("150 Opening connection for list.\r\n");
        it->transfer_mode=true;
        it->transfer_abort=false;
        globus_ftp_control_data_connect_write(&(it->handle),&list_connect_retrieve_callback,it);
      }
      else if(it->data_conn_type == GRIDFTP_CONNECT_PASV) {
        it->send_response("150 Opening connection for list.\r\n");
        it->transfer_mode=true;
        it->transfer_abort=false;
        globus_ftp_control_data_connect_write(&(it->handle),&list_connect_retrieve_callback,it);
      }
      else { it->send_response("501 PORT or PASV command needed\r\n"); };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_ERET: {
      fix_string_arg(command->eret.string_arg);
  oilog(it->log_id)<<"Command ERET "<<command->eret.string_arg<<std::endl;
      char* args[4];
      if(parse_args(command->eret.raw_command,args,4)<4) {
        it->send_response("500 parsing failed\r\n"); break;
      };
      if(strcmp(args[0],"P")) {
        it->send_response("500 mark parsing failed\r\n"); break;
      };
      char* ep;
      it->virt_restrict=true;
      it->virt_offset=strtoull(args[1],&ep,10);
      if((*ep) != 0) {
        it->send_response("500 offset parsing failed\r\n"); break;
      };
      it->virt_size=strtoull(args[2],&ep,10);
      if((*ep) != 0) {
        it->send_response("500 size parsing failed\r\n"); break;
      };
      if(it->froot.open(args[3],GRIDFTP_OPEN_RETRIEVE)!=0) {
        /* failed to open */
        if(it->froot.error.length()) {
          it->send_response("550 "+it->froot.error+"\r\n");
        } else {
          it->send_response("550 File unavailable.\r\n");
        }; break;
      };
    };
    case GLOBUS_FTP_CONTROL_COMMAND_RETR: {
      if(command->code == GLOBUS_FTP_CONTROL_COMMAND_RETR) {
        fix_string_arg(command->retr.string_arg);
  oilog(it->log_id)<<"Command RETR "<<command->retr.string_arg<<std::endl;
        CHECK_TRANSFER;
        /* try to open file */
        if(it->froot.open(command->retr.string_arg,GRIDFTP_OPEN_RETRIEVE)!=0) {
          /* failed to open */
          if(it->froot.error.length()) {
            it->send_response("550 "+it->froot.error+"\r\n"); break;
          } else {
            it->send_response("550 File unavailable.\r\n"); break;
          }; break;
        };
        it->virt_restrict=false;
      };
      if(it->data_conn_type == GRIDFTP_CONNECT_PORT) {
        it->send_response("150 Opening connection.\r\n");
        it->transfer_abort=false;
        it->transfer_mode=true;
        globus_ftp_control_data_connect_write(&(it->handle),&data_connect_retrieve_callback,it);
      }
      else if(it->data_conn_type == GRIDFTP_CONNECT_PASV) {
        it->send_response("150 Opening connection.\r\n");
        it->transfer_abort=false;
        it->transfer_mode=true;
        globus_ftp_control_data_connect_write(&(it->handle),&data_connect_retrieve_callback,it);
      }
      else { it->send_response("502 PORT or PASV command needed\r\n"); };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_STOR: {
      fix_string_arg(command->stor.string_arg);
  oilog(it->log_id)<<"Command STOR "<<command->stor.string_arg<<std::endl;
      CHECK_TRANSFER;
      /* try to open file */
      if(it->froot.open(command->stor.string_arg,GRIDFTP_OPEN_STORE) != 0) {
        it->file_size=0;
        /* failed to open */
        if(it->froot.error.length()) {
          it->send_response("553 "+it->froot.error+"\r\n");
        } else {
          it->send_response("553 File not allowed.\r\n");
        }; break;
      };
      it->file_size=0;
      if(it->data_conn_type == GRIDFTP_CONNECT_PORT) {
        it->send_response("150 Opening connection.\r\n");
        it->transfer_abort=false;
        it->transfer_mode=true;
        globus_ftp_control_data_connect_read(&(it->handle),&data_connect_store_callback,it);
      }
      else if(it->data_conn_type == GRIDFTP_CONNECT_PASV) {
        it->send_response("150 Opening connection.\r\n");
        it->transfer_abort=false;
        it->transfer_mode=true;
        globus_ftp_control_data_connect_read(&(it->handle),&data_connect_store_callback,it);
      }
      else { it->send_response("501 PORT or PASV command needed\r\n"); };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_ALLO: {
  oilog(it->log_id)<<"Command ALLO "<<command->allo.size<<"\n";
      it->file_size=0;
      char* args[4];
      int n = parse_args(command->allo.raw_command,args,4);
      if( (n==0) || (n==4) || (n==2) || ((n==3) && (strcmp(args[1],"R"))) ) {
        it->send_response("500 parsing failed\r\n"); break;
      };
      char *e;
      it->file_size=strtoull(args[0],&e,10);
      if((*e) != 0) {
        it->file_size=0;
        it->send_response("500 parsing failed\r\n"); break;
      };
      if(n == 3) {
        it->file_size=strtoull(args[2],&e,10);
        if((*e) != 0) {
          it->file_size=0;
          it->send_response("500 parsing failed\r\n"); break;
        };
      };
      it->send_response("200 Size accepted\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_OPTS: {
  oilog(it->log_id)<<"Command OPTS\n";
      CHECK_TRANSFER;
      if(!strncasecmp(command->opts.cmd_name,"RETR",4)) {
  oilog(it->log_id)<<"Command OPTS RETR\n";
        char* args[3];
        char* val;
        int v;
        int i;
        int n=parse_semicolon(command->opts.cmd_opts,args,3);
        if(n>3) n=3;
        globus_ftp_control_parallelism_t dp;
        dp.mode=GLOBUS_FTP_CONTROL_PARALLELISM_NONE;
        for(i=0;i<n;i++) {
  oilog(it->log_id)<<"Option: "<<args[i]<<std::endl;
          val=strchr(args[i],'=');
          if(!val) {
            it->send_response("500 Syntax failure\r\n"); break;
          };
          (*val++)=0;
          if(!strcasecmp(args[i],"parallelism")) {
            int argn[3];
            if((v=parse_integers(val,argn,3)) != 3) {
              it->send_response("500 parsing failed\r\n"); break;
            };
            if(argn[0]<=0) {
              it->send_response("500 bad value\r\n"); break;
            };
            if(argn[0]>50) {
              it->send_response("500 too big value\r\n"); break;
            };
            dp.mode=GLOBUS_FTP_CONTROL_PARALLELISM_FIXED;
            dp.fixed.size=argn[0];
            continue;
          }
          else {
            it->send_response("501 Sorry, option not supported\r\n"); break;
          };
        };
        if(i<n) break;
        globus_ftp_control_local_parallelism(&(it->handle),&dp);
/*
        it->data_buffer_num=dp.fixed.size*2+1;
        it->data_buffer_size=default_data_buffer_size;
        if(it->data_buffer_num > 41) it->data_buffer_num=41;
        if(it->data_buffer_num < 3) it->data_buffer_num=3;
        if((it->data_buffer_num * it->data_buffer_size)>max_data_buffer_size) { 
          it->data_buffer_size=max_data_buffer_size/it->data_buffer_num;
        };
        if(it->data_buffer_size=0) { it->data_buffer_size=1; };
*/
        it->send_response("200 New options are valid\r\n");
      }
      else {
        it->send_response("501 OPTS for command is not supported\r\n"); break;
      };
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_NOOP: {
  oilog(it->log_id)<<"Command NOOP\n";
      it->send_response("200 Doing nothing.\r\n");
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_QUIT: {
  oilog(it->log_id)<<"Command QUIT\n";
      it->make_abort();
      if(it->send_response("221 Quitting.\r\n") == 0) {
        it->wait_response();
      };
      ////oilog(it->log_id)<<"Deleting client\n";
      //globus_ftp_control_force_close(&(it->handle),&close_callback,it);
      //// delete it;
      oilog(it->log_id)<<"Closing connection\n";
      if(globus_ftp_control_force_close(&(it->handle),&close_callback,it)
                                       != GLOBUS_SUCCESS) { 
        oilog(it->log_id)<<"Warning: failed to close, deleting client\n";
        delete it;
      //} else {
      //  it->wait_response();
      };
      //delete it;
      return;
    }; break;
    case GLOBUS_FTP_CONTROL_COMMAND_UNKNOWN:
    default: {
      fix_string_arg(command->base.raw_command);
      if(!strncasecmp("DCAU",command->base.raw_command,4)) {
        char* args[2];
        int n = parse_args(command->base.raw_command,args,2);
        oilog(it->log_id)<<"Command DCAU: "<<n<<" '"<<args[0]<<"'\n";
        if((n < 1) || (n > 2) || (strlen(args[0]) != 1)) {
          it->send_response("500 Wrong number of arguments\r\n"); break;
        };
        if(args[0][0] == 'T') args[0][0]='A';
        if((args[0][0] == GLOBUS_FTP_CONTROL_DCAU_NONE) ||
           (args[0][0] == GLOBUS_FTP_CONTROL_DCAU_SELF)) {
           if(n != 1) { it->send_response("500 Do not need a subject\r\n"); break; }
        }
        else if(args[0][0] == GLOBUS_FTP_CONTROL_DCAU_SUBJECT) {
           if(n != 2) { it->send_response("500 Need an argument\r\n"); break; }
        }
        else {
          it->send_response("504 Unsupported authentication type\r\n"); break;
        };

        it->data_dcau.mode=(globus_ftp_control_dcau_mode_t)(args[0][0]);
        if(n>1) {
          if(it->data_dcau.subject.subject) free(it->data_dcau.subject.subject);
          it->data_dcau.subject.subject = strdup(args[1]);
        };
        globus_ftp_control_local_dcau(&(it->handle),&(it->data_dcau),it->delegated_cred);
        it->send_response("200 Authentication type accepted\r\n");
      }
      else if(!strncasecmp("PBSZ",command->base.raw_command,4)) {
        CHECK_TRANSFER;
        char* args[1];
        int n = parse_args(command->base.raw_command,args,1);
  oilog(it->log_id)<<"Command PBZS: "<<args[0]<<std::endl;
        if(n > 1) { it->send_response("501 Need only one argument\r\n"); break; };
        unsigned long pbsz;
        unsigned long npbsz;
        pbsz=atoi(args[0]);
        if((n <= 0) || (n>1000000)) {  /* let's not support TOO BIG buffers */
          it->send_response("501 Wrong number\r\n"); break; 
        };
  oilog(it->log_id)<<"Setting pbsz to "<<pbsz<<std::endl;
        globus_ftp_control_local_pbsz(&(it->handle),pbsz);
        globus_ftp_control_get_pbsz(&(it->handle),&npbsz);
        if(pbsz == npbsz) {
          it->send_response("200 Accepted buffer size\r\n");
        }
        else {
          char buf[200]; sprintf(buf,"200 PBSZ=%lu\r\n",npbsz);
          it->send_response(buf);
        };
/*        it->data_buffer_size=npbsz;  */
      }
      else if(!strncasecmp("PROT",command->base.raw_command,4)) {
        CHECK_TRANSFER;
        char* args[1];
        int n = parse_args(command->base.raw_command,args,1);
  oilog(it->log_id)<<"Command PROT: "<<args[0]<<std::endl;
        if(n > 1) { it->send_response("501 Need only one argument\r\n"); break; };
        if(strlen(args[0]) != 1) { it->send_response("504 Protection level is not supported\r\n"); break; };
        bool allow_protection = true;
        switch(args[0][0]) {
          case GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE:
            if(!it->froot.heavy_encryption) allow_protection=false;
          case GLOBUS_FTP_CONTROL_PROTECTION_SAFE:
          case GLOBUS_FTP_CONTROL_PROTECTION_CONFIDENTIAL:
          case GLOBUS_FTP_CONTROL_PROTECTION_CLEAR: {
            if(allow_protection) {
              it->send_response("200 Protection mode accepted\r\n");
	      globus_ftp_control_local_prot(&(it->handle),
                              (globus_ftp_control_protection_t)args[0][0]);
            } else {
              it->send_response("504 Protection level is not allowed\r\n");
            };
          }; break;
          default: {
            it->send_response("504 Protection level is not supported\r\n");
          };
        };
      }
      else if(!strncasecmp("MDTM",command->base.raw_command,4)) {
        char* arg = get_arg(command->base.raw_command);
  oilog(it->log_id)<<"Command MDTM"<<(arg?arg:"<empty>")<<std::endl;
        if(arg == NULL) { it->send_response("501 Need name\r\n"); break; };
        time_t t;
        struct tm tt;
        struct tm *tp;
        if(it->froot.time(arg,&t) != 0) {
          if(it->froot.error.length()) {
            it->send_response("550 "+it->froot.error+"\r\n");
          } else {
            it->send_response("550 Time for object not available.\r\n");
          };
          break;
        };
        tp=gmtime_r(&t,&tt);
        if(tp==NULL) {
          it->send_response("550 Time for object not available\r\n");
          break;
        };
        char buf[200]; sprintf(buf,"213 %04u%02u%02u%02u%02u%02u\r\n",tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
        it->send_response(buf);
      }
      else {
        oilog(it->log_id)<<"Raw command: "<<command->base.raw_command<<std::endl;
        it->send_response("500 Do not understand\r\n");
      };
    };
  };
}

void GridFTP_Commands::free_data_buffer(void) {
  if(data_buffer == NULL) return;
  for(unsigned int i = 0;i<data_buffer_num;i++) {
    free(data_buffer[i].data);
  };
  free(data_buffer); data_buffer=NULL;
}


void GridFTP_Commands::compute_data_buffer(void) {
  globus_ftp_control_parallelism_t dp;
  if(globus_ftp_control_get_parallelism(&handle,&dp) != GLOBUS_SUCCESS) {
    data_buffer_num=3;
    data_buffer_size=default_data_buffer_size;
    return;
  };
  if(dp.mode != GLOBUS_FTP_CONTROL_PARALLELISM_FIXED) {
    data_buffer_num=3;
    data_buffer_size=default_data_buffer_size;
    return;
  };
  data_buffer_num=dp.fixed.size*2+1;
  data_buffer_size=default_data_buffer_size;
  if(data_buffer_num > 41) data_buffer_num=41;
  if(data_buffer_num < 3) data_buffer_num=3;
  if((data_buffer_num * data_buffer_size) > max_data_buffer_size) {
    data_buffer_size=max_data_buffer_size/data_buffer_num;
  };
  if(data_buffer_size==0) { data_buffer_size=1; };
  return;
}

bool GridFTP_Commands::allocate_data_buffer(void) {
  free_data_buffer();
  data_buffer=(data_buffer_t*)malloc(sizeof(data_buffer_t)*data_buffer_num);
  if(data_buffer == NULL) return false;
  unsigned int i;
  for(i = 0;i<data_buffer_num;i++) {
    data_buffer[i].used=0;
    data_buffer[i].data=(unsigned char*)malloc(data_buffer_size);
    if(data_buffer[i].data == NULL) {
      oilog(log_id)<<"ERROR: Failed to allocate memory for buffer"<<std::endl;
      break;
    };
  };
  if(i == 0) {
    free(data_buffer); data_buffer=NULL; return false;
  };
  oilog(log_id)<<"Allocated "<<i<<" buffers "<<data_buffer_size<<" bytes each."<<std::endl;
  data_buffer_num=i;
  return true;
}

void GridFTP_Commands::abort_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  oilog(it->log_id)<<"abort_callback: start"<<std::endl;
  globus_mutex_lock(&(it->abort_lock));
  if(error != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"abort_callback: Globus error: "<<error<<std::endl;
  };
  /* check for flag just in case */
  if(it->transfer_abort) {
    it->send_response("226 Abort finished\r\n");
  };
  it->transfer_abort=false;
  it->transfer_mode=false;
  globus_cond_broadcast(&(it->abort_cond)); /* many threads can be waiting */
  globus_mutex_unlock(&(it->abort_lock));
}

/* perform data transfer abort */
void GridFTP_Commands::make_abort(bool already_locked,bool wait_abort) {
  oilog(log_id)<<"make_abort: start"<<std::endl;
  if(!already_locked) globus_mutex_lock(&abort_lock);
  if(!transfer_mode) {   /* leave if not transfering */
    globus_mutex_unlock(&abort_lock);
    return;
  };
  bool t = transfer_mode;
  if(!transfer_abort) { /* not aborting yet */
    if(globus_ftp_control_data_force_close(&handle,abort_callback,this) 
                  == GLOBUS_SUCCESS) {
      transfer_abort=true;
    } else {
      oilog(log_id)<<"Failed to abort data connection - ignoring and recovering"<<std::endl;
      globus_mutex_unlock(&abort_lock);
      abort_callback(this,&handle,GLOBUS_SUCCESS);
      globus_mutex_lock(&abort_lock);
    };
  };
  last_action_time=time(NULL);
  if(wait_abort) while(transfer_abort) {
    oilog(log_id)<<"make_abort: wait for abort flag to be reset"<<std::endl;
    globus_cond_wait(&abort_cond,&abort_lock);
  };
  if(t) {
    /* transfer_mode=false; */
    /* close (if) opened files */
    froot.close(false);
    virt_offset=0;
    virt_restrict=false;
  };
  oilog(log_id)<<"make_abort: leaving"<<std::endl;
  globus_mutex_unlock(&abort_lock);
}

/* check for globus error, print it and abort connection if necessary */
/* This function should always be called from data transfer callbacks
   with data_lock locked */
bool GridFTP_Commands::check_abort(globus_object_t *error) {
  globus_mutex_lock(&abort_lock);
  if(transfer_abort || (!transfer_mode)) { 
    /* abort in progress or not transfering anymore */
    globus_mutex_unlock(&abort_lock);
    return true;  /* just leave telling to stop registering buffers */
  };
  if((error != GLOBUS_SUCCESS)) {
    oilog(log_id)<<"check_abort: have Globus error"<<std::endl;
    oilog(log_id)<<"Abort request caused by transfer error"<<std::endl;
    oilog(log_id)<<"Globus error: "<<error<<std::endl;
    /* TODO !!!!!!!!!!! should be only one 426 !!!!!!!!!! */
    oilog(log_id)<<"check_abort: sending 426"<<std::endl;
    send_response("426 Transfer terminated.\r\n");
    globus_mutex_unlock(&data_lock); /* release other waiting threads */
    make_abort(true,false);
    globus_mutex_lock(&data_lock);
    return true;
  };
  globus_mutex_unlock(&abort_lock);
  return false;
}

/* same as make_abort, but is called from data transfer callbacks */
/* This function should always be called from data transfer callbacks
   with data_lock locked */
void GridFTP_Commands::force_abort(void) {
  globus_mutex_lock(&abort_lock);
  if(transfer_abort || (!transfer_mode)) {
    /* abort in progress or not transfering anymore*/
    globus_mutex_unlock(&abort_lock);
    return; 
  };
  oilog(log_id) << "Abort request caused by error in transfer function"<<std::endl;
  /* TODO !!!!!!!!!!! should be only one 426 !!!!!!!!!! */
  if(froot.error.length()) {
    send_response("426 "+froot.error+"\r\n");
  } else {
    send_response("426 Transfer terminated.\r\n");
  };
  globus_mutex_unlock(&data_lock); /* release other waiting threads */
  make_abort(true,false);
  globus_mutex_lock(&data_lock);
  return;
}

GridFTP_Commands::GridFTP_Commands(int n,unsigned int* f) {
  log_id=n;
  firewall[0]=0; firewall[1]=0; firewall[2]=0; firewall[3]=0;
  if(f) memcpy(firewall,f,sizeof(firewall));
  globus_mutex_init(&response_lock,GLOBUS_NULL);
  globus_cond_init(&response_cond,GLOBUS_NULL);
  response_done=0;
  globus_mutex_init(&abort_lock,GLOBUS_NULL);
  globus_cond_init(&abort_cond,GLOBUS_NULL);
  data_done=0;
  globus_mutex_init(&data_lock,GLOBUS_NULL);
  data_buffer=NULL;
  data_buffer_size=default_data_buffer_size;
  data_buffer_num=3;
  globus_ftp_control_handle_init(&handle);
  data_dcau.mode=GLOBUS_FTP_CONTROL_DCAU_DEFAULT;
  data_dcau.subject.subject=NULL;
  data_conn_type=GRIDFTP_CONNECT_NONE;
  virt_offset=0;
  virt_restrict=false;
  transfer_mode=false;
  transfer_abort=false;
  file_size=0;
  last_action_time=time(NULL);
  /* harmless race condition here */
  if(!timeouter) {
    GridFTP_Commands_timeout* t = new GridFTP_Commands_timeout;
    if(!timeouter) { timeouter=t; } else { delete t; };
  };
  timeouter->add(*this);
};

GridFTP_Commands::~GridFTP_Commands(void) {
/* here all connections should be closed and all callbacks unregistered */
//  oilog(it->log_id)<<"Destroying client\n";
  globus_mutex_destroy(&response_lock);
  globus_cond_destroy(&response_cond);
  globus_mutex_destroy(&abort_lock);
  globus_cond_destroy(&abort_cond);
  globus_ftp_control_handle_destroy(&handle);
  timeouter->remove(*this);
#ifndef __DONT_USE_FORK__
  delete timeouter;
/*
  globus_mutex_lock(&fork_lock);
// oilog(it->log_id)<<"Signaling exit condition"<<std::endl;
  fork_done=1;
  globus_cond_signal(&fork_cond);
  globus_mutex_unlock(&fork_lock);
*/
#endif
};


GridFTP_Commands_timeout::GridFTP_Commands_timeout(void) {
  exit_cond_flag=false;
  cond_flag=false;
  globus_mutex_init(&lock,GLOBUS_NULL);
  globus_cond_init(&cond,GLOBUS_NULL);
  globus_cond_init(&exit_cond,GLOBUS_NULL);
  if(globus_thread_create(&timer_thread,NULL,&timer_func,(void*)this)!=0){
    olog << "Failed to start timer thread - timeout won't work" << std::endl;
    globus_mutex_destroy(&lock);
    globus_cond_destroy(&cond);
    globus_cond_destroy(&exit_cond);
    exit_cond_flag=true;
  };
}

GridFTP_Commands_timeout::~GridFTP_Commands_timeout(void) {
  if(exit_cond_flag) return;
  cond_flag=true;
  globus_mutex_lock(&lock);
  globus_cond_signal(&cond);
  while(!exit_cond_flag) {
    globus_cond_wait(&exit_cond,&lock);
  };
  globus_mutex_unlock(&lock);
  globus_mutex_destroy(&lock);
  globus_cond_destroy(&cond);
  globus_cond_destroy(&exit_cond);
}

void GridFTP_Commands_timeout::remove(const GridFTP_Commands& cmd) {
  if(exit_cond_flag) return;
  globus_mutex_lock(&lock);
  for(std::list<GridFTP_Commands*>::iterator i=cmds.begin();i!=cmds.end();++i) {
    if(&cmd == (*i)) { i=cmds.erase(i); } else { ++i; };
  };
  globus_mutex_unlock(&lock);
}

void GridFTP_Commands_timeout::add(GridFTP_Commands& cmd) {
  if(exit_cond_flag) return;
  globus_mutex_lock(&lock);
  cmds.push_back(&cmd);
  globus_mutex_unlock(&lock);
}

void* GridFTP_Commands_timeout::timer_func(void* arg) {
  GridFTP_Commands_timeout* it = (GridFTP_Commands_timeout*)arg;
  globus_mutex_lock(&(it->lock));
  for(;;) {
    if(it->cond_flag) { /* destructor */
      break;
    };    
    time_t curr_time = time(NULL);
    time_t next_wakeup = curr_time + FTP_TIMEOUT;
    for(std::list<GridFTP_Commands*>::iterator i=it->cmds.begin();
                                          i!=it->cmds.end();++i) {
      if((*i)->last_action_time != (time_t)(-1)) {
        time_t time_passed = curr_time - (*i)->last_action_time;
        if(time_passed >= FTP_TIMEOUT) {  /* cancel connection */
          oilog((*i)->log_id)<<"Killing connection due to timeout"<<std::endl;
#if GLOBUS_IO_VERSION<5
          shutdown((*i)->handle.cc_handle.io_handle.fd,2);
#else
          globus_io_register_close(&((*i)->handle.cc_handle.io_handle),&io_close_cb,NULL);
#endif
          (*i)->last_action_time=(time_t)(-1);
        }
        else {
          time_passed = curr_time + (FTP_TIMEOUT - time_passed);
          if(time_passed < next_wakeup) next_wakeup = time_passed;
        };
      };
    };
    curr_time = time(NULL);
    if(next_wakeup < curr_time) { next_wakeup=curr_time; };
    globus_abstime_t timeout;
    GlobusTimeAbstimeSet(timeout,next_wakeup-curr_time,0);
    globus_cond_timedwait(&(it->cond),&(it->lock),&timeout);
  };
  it->exit_cond_flag=true;
  globus_cond_signal(&(it->exit_cond));
  globus_mutex_unlock(&(it->lock));
  return NULL;
};

