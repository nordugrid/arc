#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <globus_io.h>

#include <arc/globusutils/GlobusErrorUtils.h>

#include "HTTPSClient.h"

namespace Arc {
  
  Logger HTTPSClientConnector::logger(Logger::getRootLogger(), "HTTPSClientConnector");

  // ------------------ Globus -------------------------------
    
  HTTPSClientConnectorGlobus::HTTPSClientConnectorGlobus(const char* base,bool heavy_encryption,int timeout_,gss_cred_id_t cred_) try: base_url(base) {
    valid=false; connected=false;
    read_registered=false;
    write_registered=false;
    read_size=NULL;
    cred=cred_;
    timeout=timeout_; // 1 min.
    /* initialize globus io connection related objects */
    globus_io_tcpattr_init(&attr);
    globus_io_secure_authorization_data_initialize(&auth);
    globus_io_secure_authorization_data_set_callback(&auth,&authorization_callback,NULL);
    if(strcasecmp(base_url.Protocol().c_str(),"http") == 0) {
      globus_io_attr_set_secure_authentication_mode(&attr,
                GLOBUS_IO_SECURE_AUTHENTICATION_MODE_NONE,GSS_C_NO_CREDENTIAL);
      globus_io_attr_set_secure_authorization_mode(&attr,
                GLOBUS_IO_SECURE_AUTHORIZATION_MODE_NONE,GLOBUS_NULL);
      globus_io_attr_set_secure_channel_mode(&attr,
                                   GLOBUS_IO_SECURE_CHANNEL_MODE_CLEAR);
      globus_io_attr_set_secure_protection_mode(&attr,
                                   GLOBUS_IO_SECURE_PROTECTION_MODE_NONE);
      globus_io_attr_set_secure_delegation_mode(&attr,
                                   GLOBUS_IO_SECURE_DELEGATION_MODE_NONE);
    } else if(strcasecmp(base_url.Protocol().c_str(),"https") == 0) {
      globus_io_attr_set_secure_authentication_mode(&attr,
                GLOBUS_IO_SECURE_AUTHENTICATION_MODE_MUTUAL,cred);
      globus_io_attr_set_secure_authorization_mode(&attr,
                GLOBUS_IO_SECURE_AUTHORIZATION_MODE_HOST,GLOBUS_NULL);
      globus_io_attr_set_secure_channel_mode(&attr,
                                   GLOBUS_IO_SECURE_CHANNEL_MODE_SSL_WRAP);
      if(heavy_encryption) {
        globus_io_attr_set_secure_protection_mode(&attr,
                                   GLOBUS_IO_SECURE_PROTECTION_MODE_PRIVATE);
      } else {
        globus_io_attr_set_secure_protection_mode(&attr,
                                   GLOBUS_IO_SECURE_PROTECTION_MODE_SAFE);
      };
      globus_io_attr_set_secure_delegation_mode(&attr,
                                   GLOBUS_IO_SECURE_DELEGATION_MODE_NONE);
    } else if(strcasecmp(base_url.Protocol().c_str(),"httpg") == 0) {
      globus_io_attr_set_secure_authentication_mode(&attr,
                GLOBUS_IO_SECURE_AUTHENTICATION_MODE_GSSAPI,cred);
      globus_io_attr_set_secure_authorization_mode(&attr,
                GLOBUS_IO_SECURE_AUTHORIZATION_MODE_HOST,GLOBUS_NULL);
      globus_io_attr_set_secure_channel_mode(&attr,
                                   GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP);
      if(heavy_encryption) {
        globus_io_attr_set_secure_protection_mode(&attr,
                                   GLOBUS_IO_SECURE_PROTECTION_MODE_PRIVATE);
      } else {
        globus_io_attr_set_secure_protection_mode(&attr,
                                   GLOBUS_IO_SECURE_PROTECTION_MODE_SAFE);
      };
      globus_io_attr_set_secure_delegation_mode(&attr,
                                   GLOBUS_IO_SECURE_DELEGATION_MODE_FULL_PROXY);
    } else {
      return;
    };
    globus_io_attr_set_secure_proxy_mode(&attr,GLOBUS_IO_SECURE_PROXY_MODE_NONE);
    valid=true;
  } catch(std::exception e) {
    timeout=timeout_; // 1 min.
    valid=false; connected=false;
    /* initialize globus io connection related objects */
    globus_io_tcpattr_init(&attr);
    globus_io_secure_authorization_data_initialize(&auth);
    globus_io_secure_authorization_data_set_callback(&auth,&authorization_callback,NULL);
  }
  
  HTTPSClientConnectorGlobus::~HTTPSClientConnectorGlobus(void) {
    disconnect();
    globus_io_secure_authorization_data_destroy(&auth);
    globus_io_tcpattr_destroy(&attr);
  }
  
  bool HTTPSClientConnectorGlobus::credentials(gss_cred_id_t cred_) {
    if(cred_ == GSS_C_NO_CREDENTIAL) return false;
    gss_cred_id_t cred_old;
    globus_io_secure_authentication_mode_t mode;
    if(globus_io_attr_get_secure_authentication_mode(&attr,&mode,&cred_old) !=
                                         GLOBUS_SUCCESS) return false;
    if(globus_io_attr_set_secure_authentication_mode(&attr,mode,cred_) !=
                                         GLOBUS_SUCCESS) return false;
    cred=cred_;
    return true;
  }
  
  bool HTTPSClientConnectorGlobus::read(char* buf,unsigned int* size) {
    if(!connected) return false;
    globus_result_t res;
    unsigned int size_=size?*size:0;
    if(size) *size=0;
    if((buf == NULL) || (size_ == 0)) {
      // cancel request
      if(read_registered) {
        res=globus_io_cancel(&s,GLOBUS_FALSE);
        if(res != GLOBUS_SUCCESS) {
          logger.msg(ERROR, "globus_io_cancel failed: %s", GlobusResult(res).str());
          return false;
        };
        read_registered=false;
        write_registered=false;
      };
      return true;
    };
    if(read_registered) return false;
    read_size=size;
    read_registered=true;
    read_done=-1; cond.reset();
    res=globus_io_register_read(&s,(globus_byte_t*)buf,size_,1,
                                                       &read_callback,this);
    if(res != GLOBUS_SUCCESS) {
      read_registered=false;
      logger.msg(ERROR, "globus_io_register_read failed: %s", GlobusResult(res).str());
      return false;
    };
    return true;
  }
  
  bool HTTPSClientConnectorGlobus::write(const char* buf,unsigned int size) {
    if(!connected) return false;
    globus_result_t res;
    if((buf == NULL) || (size == 0)) {
      // cancel request
      if(write_registered) {
        res=globus_io_cancel(&s,GLOBUS_FALSE);
        if(res != GLOBUS_SUCCESS) {
          logger.msg(ERROR, "globus_io_cancel failed: %s", GlobusResult(res).str());
          return false;
        };
        read_registered=false;
        write_registered=false;
      };
      return true;
    };
    if(write_registered) return false;
    write_registered=true;
    write_done=-1; cond.reset();
    res=globus_io_register_write(&s,(globus_byte_t*)buf,size,&write_callback,this);
    if(res != GLOBUS_SUCCESS) {
      write_registered=false;
      logger.msg(ERROR, "globus_io_register_write failed: %s", GlobusResult(res).str());
      return false;
    };
    return true;
  }
  
  bool HTTPSClientConnectorGlobus::connect(void) {
    if(!valid) return false;
    if(connected) return true;
    globus_result_t res;
    read_registered=false; write_registered=false;
    read_done=-1; write_done=-1;
    cond.reset();
    if((res=globus_io_tcp_register_connect(
                      (char*)(base_url.Host().c_str()),base_url.Port(),
                      &attr,&general_callback,this,&s)) != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Connect to %s failed: %s", base_url.str(), GlobusResult(res).str());
      return false;
    };
    globus_thread_blocking_will_block(); // ????
    if(!cond.wait(timeout)) {  // timeout
      logger.msg(ERROR, "Connection to %s timed out after %i seconds", base_url.str(), timeout/1000);
      globus_io_cancel(&s,GLOBUS_FALSE);
      globus_io_close(&s);
      return false;
    };
    connected=true;
    return true;
  }
  
  bool HTTPSClientConnectorGlobus::disconnect(void) {
    if(!connected) return true;
    globus_io_cancel(&s,GLOBUS_FALSE);
    globus_io_close(&s);
    connected=false;
    return true;
  }
  
  bool HTTPSClientConnectorGlobus::transfer(bool& read,bool& write,int timeout) {
    read=false; write=false;
    if((!read_registered) && (!write_registered)) return true;
    for(;;) {
      if(read_registered && (read_done!=-1)) {
        read_registered=false; read=(read_done==0); break;
      };
      if(write_registered && (write_done!=-1)) {
        write_registered=false; write=(write_done==0); break;
      };
      if(!cond.wait(timeout)) return false; // timeout
    };
    return true;
  }
  
  
  bool HTTPSClientConnectorGlobus::clear(void) {
    if(!valid) return false;
    globus_byte_t buf[256];
    globus_size_t l;
    for(;;) {
      if(globus_io_read(&s,buf,256,0,&l) != GLOBUS_SUCCESS) return false;
      if(l == 0) break;
      std::string buf_str;
      for(globus_size_t n=0;n<l;n++) buf_str += buf[n];
      logger.msg(VERBOSE, "clear_input: %s", buf_str);
    };
    return true;
  }
  
  void HTTPSClientConnectorGlobus::general_callback(void *arg,globus_io_handle_t *handle,globus_result_t  result) {
    HTTPSClientConnectorGlobus* it = (HTTPSClientConnectorGlobus*)arg;
    if(result != GLOBUS_SUCCESS) logger.msg(ERROR, "Globus error: %s", GlobusResult(result).str());
    it->cond.signal();
  }
  
  void HTTPSClientConnectorGlobus::read_callback(void *arg,globus_io_handle_t *handle,globus_result_t result,globus_byte_t *buf,globus_size_t nbytes) {
    HTTPSClientConnectorGlobus* it = (HTTPSClientConnectorGlobus*)arg;
    int res = 0;
    if(result != GLOBUS_SUCCESS) {
      globus_object_t* err = globus_error_get(result);
      char* tmp=globus_object_printable_to_string(err);
      if(strstr(tmp,"end-of-file") != NULL) {
        logger.msg(VERBOSE, "Connection closed");
        res=2; // eof
      } else {
        logger.msg(ERROR, "Globus error (read): %s", tmp);
        res=1;
      };
      free(tmp); globus_object_free(err);
    } else {
      std::string buf_str;
      for(globus_size_t n=0;n<nbytes;n++) buf_str += buf[n];
      logger.msg(VERBOSE, "*** Server response: %s", buf_str);
      if(it->read_size) *(it->read_size)=nbytes;
    };
    it->cond.lock();
    it->read_done=res; it->cond.signal_nonblock();
    it->cond.unlock();
  }
  
  void HTTPSClientConnectorGlobus::write_callback(void *arg,globus_io_handle_t *handle,globus_result_t result,globus_byte_t *buf,globus_size_t nbytes) {
    HTTPSClientConnectorGlobus* it = (HTTPSClientConnectorGlobus*)arg;
    int res = 0;
    if(result != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Globus error (write): %s", GlobusResult(result).str());
      res=1;
    } else {
      std::string buf_str;
      for(globus_size_t n=0;n<nbytes;n++) buf_str += buf[n];
      logger.msg(VERBOSE, "*** Client request: %s", buf_str);
    };
    it->cond.lock();
    it->write_done=res; it->cond.signal_nonblock();
    it->cond.unlock();
  }
  
  globus_bool_t HTTPSClientConnectorGlobus::authorization_callback(void* arg,globus_io_handle_t* h,globus_result_t result,char* identity,gss_ctx_id_t context_handle) {
    logger.msg(VERBOSE, "Authenticating: %s", identity);
    return GLOBUS_TRUE;
  }
  
  bool HTTPSClientConnectorGlobus::eofread(void) {
    return (read_done==2);
  }
  
  bool HTTPSClientConnectorGlobus::eofwrite(void) {
    return (!write_registered);
  }
  
  // --------------- GSSAPI ---------------
   
  static std::string gss_error_string(OM_uint32 maj_status,OM_uint32 min_status) {
    std::string message;
    OM_uint32 major_status = 0;
    OM_uint32 minor_status = 0;
    OM_uint32 m_context = 0;
    gss_buffer_desc buf;
    for(;;) {
      buf.length=0; buf.value=NULL;
      major_status=gss_display_status(&minor_status,maj_status,
                     GSS_C_GSS_CODE,GSS_C_NULL_OID,&m_context,&buf);
      if(buf.value != NULL) {
        if(!message.empty()) message+="; ";
        message+=(char*)(buf.value);
        gss_release_buffer(&minor_status,&buf);
      };
      if(m_context == 0) break;
    };
    for(;;) {
      buf.length=0; buf.value=NULL;
      major_status=gss_display_status(&minor_status,min_status,
                     GSS_C_MECH_CODE,GSS_C_NULL_OID,&m_context,&buf);
      if(buf.value != NULL) {
        if(!message.empty()) message+="; ";
        message+=(char*)(buf.value);
        gss_release_buffer(&minor_status,&buf);
      };
      if(m_context == 0) break;
    };
    return message;
  }
  
  HTTPSClientConnectorGSSAPI::HTTPSClientConnectorGSSAPI(const char* base,bool heavy_encryption,int timeout_,gss_cred_id_t cred_,bool check_host) try: base_url(base), check_host_cert(check_host) {
    s=-1;
    cred=cred_;
    timeout=timeout_;
    context=GSS_C_NO_CONTEXT;
    valid=true;
  } catch(std::exception e) {
    valid=false;
  }
  
  bool HTTPSClientConnectorGSSAPI::connect(void) {
    if(!valid) return false;
    if(s != -1) return true;
    read_buf=NULL; read_size=0; read_size_result=NULL;
    write_buf=NULL; write_size=0;
    read_eof_flag=false;
  
    struct hostent* host;
    struct hostent  hostbuf;
    int    errcode;
  #ifndef _AIX
    char   buf[BUFSIZ];
    if(gethostbyname_r(base_url.Host().c_str(),&hostbuf,buf,sizeof(buf),
                                          &host,&errcode) != 0) return false;
  #else
    struct hostent_data buf[BUFSIZ];
    if((errcode=gethostbyname_r(base_url.Host().c_str(),
                                    (host=&hostbuf),buf)) != 0) return false;
  #endif
    if( (host == NULL) ||
        (host->h_length < sizeof(struct in_addr)) ||
        (host->h_addr_list[0] == NULL) ) {
      logger.msg(ERROR, "Host not found: %s", base_url.Host());
      return false;
    };
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(base_url.Port());
    memcpy(&addr.sin_addr,host->h_addr_list[0],sizeof(struct in_addr));
  
    s=::socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==-1) {
      char buf[1024];
      char* str = strerror_r(errno,buf,sizeof(buf));
      logger.msg(ERROR, "Socket creation failed: %s", (str?str:""));
      return false;
    };
  
    if(::connect(s,(struct sockaddr *)&addr,sizeof(addr))==-1) {
      char buf[1024];
      char* str = strerror_r(errno,buf,sizeof(buf));
      logger.msg(ERROR, "Connection to server failed: %s", (str?str:""));
      ::close(s); s=-1;
      return false;
    };
  
    OM_uint32 major_status = 0;
    OM_uint32 minor_status = 0;
  
    //if(cred == GSS_C_NO_CREDENTIAL) {
    //  major_status = gss_acquire_cred(&minor_status,
    //                                  GSS_C_NO_NAME,
    //                                  0,
    //                                  GSS_C_NULL_OID_SET,
    //                                  GSS_C_ACCEPT,
    //                                  &cred,
    //                                  NULL,
    //                                  NULL);
    //  if (major_status != GSS_S_COMPLETE) {
    //    logger.msg(ERROR, "Failed to acquire local credentials");
    //    ::close(s); s=-1;
    //    return false;
    //  };
    //};
  
    OM_uint32 init_sec_min_stat;
    OM_uint32 ret_flags =  0;
    gss_name_t remote_name = GSS_C_NO_NAME;
    gss_OID oid = GSS_C_NO_OID; // ??????????
    gss_buffer_desc recv_tok;
    gss_buffer_desc send_tok;
    int context_flags = GSS_C_CONF_FLAG | GSS_C_MUTUAL_FLAG | GSS_C_INTEG_FLAG | GSS_C_DELEG_FLAG; // GSS_C_GLOBUS_SSL_COMPATIBLE
  #ifdef HAVE_GLOBUS_GSS_ASSIST_AUTHORIZATION_HOST_NAME
    globus_gss_assist_authorization_host_name((char*)(base_url.Host().c_str()),&remote_name);
  #else
    {
      gss_buffer_desc name_tok;
      name_tok.value = (void*)(base_url.Host().c_str());
      name_tok.length = base_url.Host().length() + 1;
      major_status=gss_import_name(&minor_status,&name_tok,
                                   GSS_C_NT_HOSTBASED_SERVICE,&remote_name);
      // if(GSS_ERROR(major_status))
    };
  #endif
    // check if we want to do service host cert checks
    if(!check_host_cert) {
      remote_name=GSS_C_NO_NAME;
      // can't do delegation with no target host
      context_flags = GSS_C_CONF_FLAG | GSS_C_MUTUAL_FLAG | GSS_C_INTEG_FLAG;
    };
  
    recv_tok.length=0; recv_tok.value=NULL;
    send_tok.length=0; send_tok.value=NULL;
    for(;;) {
      major_status = gss_init_sec_context(&init_sec_min_stat,
                                          cred,
                                          &context,
                                          remote_name,
                                          oid,
                                          context_flags,
                                          0,
                                          NULL,
                                      recv_tok.value?&recv_tok:GSS_C_NO_BUFFER,
                                          NULL,
                                          &send_tok,
                                          &ret_flags,
                                          NULL);
      if(recv_tok.value) { free(recv_tok.value); recv_tok.value=NULL; };
      if((major_status!=GSS_S_COMPLETE) &&
         (major_status!=GSS_S_CONTINUE_NEEDED)) {
        logger.msg(ERROR, "Failed to authenticate: %s", gss_error_string(major_status,init_sec_min_stat));
        ::close(s); s=-1; break;
      };
      if(context == NULL) {
        logger.msg(ERROR, "Failed to create GSI context: %s", gss_error_string(major_status,init_sec_min_stat));
        ::close(s); s=-1; break;
      };
      if(send_tok.length != 0) {
        int to = timeout;
        if(do_write((char*)(send_tok.value),send_tok.length,to) == -1) {
          ::close(s); s=-1; break;
        };
        gss_release_buffer(&minor_status,&send_tok); send_tok.length=0;
      };
      if(major_status==GSS_S_COMPLETE) break;
      int l=read_SSL_token(&(recv_tok.value),timeout);
      if(l <= 0) {
        logger.msg(ERROR, "Failed to read SSL token during authentication");
        if(context != GSS_C_NO_CONTEXT)
          gss_delete_sec_context(&minor_status,&context,GSS_C_NO_BUFFER);
        context=GSS_C_NO_CONTEXT;
        ::close(s); s=-1;
        return false;
      };
      recv_tok.length=l;
    };
    if(s == -1) {
      if(context != GSS_C_NO_CONTEXT) {
        gss_delete_sec_context(&minor_status,&context,GSS_C_NO_BUFFER);
        context=GSS_C_NO_CONTEXT;
      };
    };
    if(recv_tok.value) { free(recv_tok.value); recv_tok.value=NULL; };
    if(send_tok.length != 0) gss_release_buffer(&minor_status,&send_tok);
    if(remote_name != GSS_C_NO_NAME) gss_release_name(&minor_status,&remote_name);
    if(s == -1) {
      /// ??????????????
    };
    return (s != -1);
  }
  
  bool HTTPSClientConnectorGSSAPI::disconnect(void) {
    if(s == -1) return true;
    ::close(s); s=-1;
    OM_uint32 minor_status = 0;
    if(context != GSS_C_NO_CONTEXT)
      gss_delete_sec_context(&minor_status,&context,GSS_C_NO_BUFFER);
    context=GSS_C_NO_CONTEXT;
    return true;
  }  
  
  bool HTTPSClientConnectorGSSAPI::read(char* buf,unsigned int* size) {
    if(s == -1) return false;
    read_size=size?*size:0;
    read_size_result=size;
    if(size) *size=0;
    read_buf=buf;
    return true;
  }
  
  bool HTTPSClientConnectorGSSAPI::write(const char* buf,unsigned int size) {
    if(s == -1) return false;
    write_size=size;
    write_buf=buf;
    return true;
  }
  
  bool HTTPSClientConnectorGSSAPI::transfer(bool& read,bool& write,int timeout) {
    read=false; write=false;
    if(write_buf) {
      OM_uint32 major_status;
      OM_uint32 minor_status;
      gss_buffer_desc send_tok;
      gss_buffer_desc data_tok;
      int conf_state;
      data_tok.length=write_size;
      data_tok.value=(void*)write_buf;
      logger.msg(VERBOSE, "*** Client request: %s", (char*)(data_tok.value));
      //for(globus_size_t n=0;n<data_tok.length;n++) odlog_(VERBOSE)<<((char*)(data_tok.value))[n];
      //odlog_(VERBOSE));
      major_status = gss_wrap(&minor_status,
                              context,
                              0,
                              GSS_C_QOP_DEFAULT,
                              &data_tok,
                              &conf_state,
                              &send_tok);
      if(major_status != GSS_S_COMPLETE) {
        logger.msg(ERROR, "Failed wrapping GSI token: %s", gss_error_string(major_status,minor_status));
        return false;
      };
      int to = timeout;
      int r = do_write((char*)(send_tok.value),send_tok.length,to);
      gss_release_buffer(&minor_status,&send_tok);
      write_buf=NULL; write_size=0; write=(r!=-1);
      return true;
    };
    if(read_buf) {
      gss_buffer_desc recv_tok;
      gss_buffer_desc data_tok = GSS_C_EMPTY_BUFFER;
      OM_uint32 major_status;
      OM_uint32 minor_status;
      int ll = read_SSL_token(&(recv_tok.value),timeout);
      if(ll == 0) { read_eof_flag=true; read=false; return true; };
      if(ll == -1) { read=false; return true; };
      recv_tok.length=ll;
      major_status = gss_unwrap(&minor_status,
                                context,
                                &recv_tok,
                                &data_tok,
                                NULL,
                                NULL);
      free(recv_tok.value);
      if(major_status != GSS_S_COMPLETE) {
        logger.msg(ERROR, "Failed unwrapping GSI token: %s", gss_error_string(major_status,minor_status));
        return false;
      };
      logger.msg(VERBOSE, "*** Server response: %s", (char*)(data_tok.value));
      //for(globus_size_t n=0;n<data_tok.length;n++) odlog_(VERBOSE)<<((char*)(data_tok.value))[n];
      //odlog_(VERBOSE));
  
      if(data_tok.length > read_size) {
        logger.msg(ERROR, "Unwrapped data does not fit into buffer");
        return false;
      };
      memcpy(read_buf,data_tok.value,data_tok.length);
      if(read_size_result) (*read_size_result)=data_tok.length;
      gss_release_buffer(&minor_status,&data_tok);
      read_buf=NULL; read_size=0; read_size_result=NULL; read=true;
      return true;
    };
    return true;
  }
  
  bool HTTPSClientConnectorGSSAPI::eofread(void) {
    return read_eof_flag;
  }
  
  bool HTTPSClientConnectorGSSAPI::eofwrite(void) {
    return (write_buf == NULL);
  }
  
  static unsigned int timems(void) {
    struct timeval tv;
    struct timezone tz;
    if(gettimeofday(&tv,&tz) != 0) return (time(NULL)*1000);
    return (tv.tv_sec*1000+tv.tv_usec/1000);
  }
  
  static bool waitsocket(int r,int w,int& timeout) {
    unsigned int t_start = timems();
    int dt = 0;
    if(timeout == -1) return true; // for blocking operation
    for(;;) {
      fd_set rs; FD_ZERO(&rs); if(r>=0) FD_SET(r,&rs);
      fd_set ws; FD_ZERO(&ws); if(w>=0) FD_SET(w,&ws);
      struct timeval t;
      t.tv_sec=(timeout-dt)/1000; t.tv_usec=(timeout-dt-t.tv_sec*1000)*1000;
      int n = ::select((r>w?r:w)+1,&rs,&ws,NULL,&t);
      if(n > 0) break;
      if((n == -1) && (errno != EINTR)) break; // to cause error on read/write
      dt=timems()-t_start;
      if(dt >= timeout) { timeout=0; return false; };
    };
    dt=timems()-t_start; 
    if(dt > timeout) dt=timeout; timeout-=dt;
    return true;
  }
  
  int HTTPSClientConnectorGSSAPI::do_read(char* buf,int size,int& timeout) {
    int n = size;
    for(;n;) {
      if(!waitsocket(s,-1,timeout)) return -1;
      int l = ::recv(s,buf,n,0);
      if((l == -1) && (errno != EINTR)) return -1;
      if(l == 0) {
        if(n == size) return 0;
        return -1;
      };
      buf+=l; n-=l;
    };
    return size;
  }
  
  int HTTPSClientConnectorGSSAPI::do_write(char* buf,int size,int& timeout) {
    int n = size;
    for(;n;) {
      if(!waitsocket(-1,s,timeout)) return -1;
      int l = ::send(s,buf,n,0);
      if((l == -1) && (errno != EINTR)) return -1;
      buf+=l; n-=l;
    };
    return size;
  }
  
  int HTTPSClientConnectorGSSAPI::read_SSL_token(void** val,int timeout) {
    unsigned char header[5]; // SSL V3 header
    (*val)=NULL;
    int l = do_read((char*)header,sizeof(header),timeout);
    if(l == 0) return 0;
    if(l < 0) return -1;
    if(header[0] == (unsigned char)0x80) {
      /* SSL V2 - 2 byte header */
      l=((unsigned int)(header[1]))-3;
    } else if((header[0] >= 20) &&
              (header[0] <= 26) &&
              (header[1] == 3) &&
              (header[2] == 0 || header[2] == 1)) {
           /* SSL V3 */
      l=(((unsigned int)(header[3])) << 8) | ((unsigned int)(header[4]));
    } else {
      logger.msg(ERROR, "Urecognized SSL token received");
      return -1;
    };
    unsigned char* data = (unsigned char*)malloc(l+5);
    if(data == NULL) return -1;
    memcpy(data,header,5);
    if(l) {
      int ll = do_read((char*)(data+5),l,timeout);
      if(ll <= 0) {
        free(data);
        return -1;
      };
    };
    (*val)=data;
    return (l+5);
  }
  
  bool HTTPSClientConnectorGSSAPI::clear(void) {
    gss_buffer_desc recv_tok;
    int l;
    while(true) {
      l=read_SSL_token(&(recv_tok.value),0);
      if(l <= 0) return true;
      if(recv_tok.value) free(recv_tok.value);
    };
  }
  
  bool HTTPSClientConnectorGSSAPI::credentials(gss_cred_id_t cred_) {
    cred=cred_;
    return true;
  }
  
  HTTPSClientConnectorGSSAPI::~HTTPSClientConnectorGSSAPI(void) {
    disconnect();
  }
  } // namespace Arc
