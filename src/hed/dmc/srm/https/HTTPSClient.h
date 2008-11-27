#ifndef __HTTPS__CLIENT_H__
#define __HTTPS__CLIENT_H__

#include <string>

#include <globus_io.h>
#include <stdsoap2.h>

#include <arc/URL.h>
#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/DateTime.h>
#include <arc/globusutils/GSSCredential.h>

namespace Arc {

  typedef enum {
    HTTP_OK = 200,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_NOT_ALLOWED = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_ERROR = 500,
    HTTP_FAILURE = -1
  } HTTPError;
  
  class HTTPResponseHeader {
   private:
    bool keep_alive;
  
    bool content_length_passed; // Content-Length
    unsigned long long int content_length;
  
    bool content_range_passed;  // Content-Range
    unsigned long long int content_size;
    unsigned long long int content_start;
    unsigned long long int content_end;
  
    Time expires; // Expires
  
    Time last_modified; // Last-Modified
  
   public:
    HTTPResponseHeader(bool alive = true);
    void reset(bool alive = true);
    bool set(const char* name,const char* value);
    bool haveContentRange(void) const { return content_range_passed; };
    bool haveContentLength(void) const { return content_length_passed; };
    bool haveExpires(void) const { return expires != 0; };
    bool haveLastModified(void) const { return last_modified != 0; };
    unsigned long long int ContentLength(void) const {
      if(content_length_passed) return content_length;
      if(content_range_passed) return (content_end-content_start+1);
      return 0;
    };
    unsigned long long int ContentStart(void) const {
      if(content_range_passed) return content_start;
      return 0;
    };
    unsigned long long int ContentEnd(void) const {
      if(content_range_passed) return content_end;
      if(content_length_passed && content_length) return content_length-1;
      return 0;
    };
    unsigned long long int ContentSize(void) const {
      if(content_range_passed) return content_size;
      if(content_length_passed) return content_length;
      return 0;
    };
    Time Expires(void) const { return expires; };
    Time LastModified(void) const { return last_modified; };
    bool KeepAlive(void) const { return keep_alive; };
  };
  
  class HTTPSClient;
  class HTTPSClientSOAP;
  
  class HTTPSClientConnector {
   friend class HTTPSClient;
   friend class HTTPSClientSOAP;
   protected:
    /// Establish connection and context (if needed)
    virtual bool connect(void);
    /// Close connection to remote host
    virtual bool disconnect(void);
    /// Read all pending data
    virtual bool clear(void);
    /// Set buffer for reading data
    virtual bool read(char* buf = NULL,unsigned int* size = NULL);
    /// Set data to be sent
    virtual bool write(const char* buf = NULL,unsigned int size = 0);
    /// Transfer data set by read() and write(). Reset set buffers
    /// if operation complete.
    virtual bool transfer(bool& read,bool& write,int timeout);
    /// If network connection was closed 
    virtual bool eofread(void);
    /// If there is pending buffer set by write()
    virtual bool eofwrite(void);
    /// logger
    static Logger logger;
   public:
    HTTPSClientConnector(void);
    virtual ~HTTPSClientConnector(void);
    virtual bool credentials(gss_cred_id_t cred);
  };
  
  class HTTPSClientConnectorGlobus: public HTTPSClientConnector {
   private:
    bool valid;
    URL base_url;
    bool connected;
    gss_cred_id_t cred;
    int timeout;
    bool read_registered;
    bool write_registered;
    unsigned int* read_size;
    SimpleCondition cond;
    globus_io_handle_t s;
    globus_io_attr_t attr;
    globus_io_secure_authorization_data_t auth;
    int read_done;
    int write_done;
    static void general_callback(void *arg,globus_io_handle_t *handle,globus_result_t  result);
    static void read_callback(void *arg,globus_io_handle_t *handle,globus_result_t result,globus_byte_t *buf,globus_size_t nbytes);
    static void write_callback(void *arg,globus_io_handle_t *handle,globus_result_t result,globus_byte_t *buf,globus_size_t nbytes);
    static globus_bool_t authorization_callback(void* arg,globus_io_handle_t* h,globus_result_t result,char* identity,gss_ctx_id_t context_handle);
   protected:
    virtual bool connect(void);
    virtual bool disconnect(void);
    virtual bool read(char* buf = NULL,unsigned int* size = NULL);
    virtual bool write(const char* buf = NULL,unsigned int size = 0);
    virtual bool clear(void);
    virtual bool transfer(bool& read,bool& write,int timeout);
    virtual bool eofread(void);
    virtual bool eofwrite(void);
   public:
    HTTPSClientConnectorGlobus(const char* base,bool heavy_encryption,int timeout = 60000,gss_cred_id_t cred = GSS_C_NO_CREDENTIAL);
    virtual ~HTTPSClientConnectorGlobus(void);
    virtual bool credentials(gss_cred_id_t cred);
  };
  
  class HTTPSClientConnectorGSSAPI: public HTTPSClientConnector {
   private:
    bool valid;
    URL base_url;
    int s;
    gss_cred_id_t cred;
    gss_ctx_id_t context;
    int timeout;
    int read_SSL_token(void** val,int timeout);
    int do_read(char* buf,int size,int& timeout);
    int do_write(char* buf,int size,int& timeout);
    char* read_buf;
    unsigned int read_size;
    unsigned int* read_size_result;
    bool read_eof_flag;
    const char* write_buf;
    unsigned int write_size;
    bool check_host_cert;
   protected:
    virtual bool connect(void);
    virtual bool disconnect(void);
    virtual bool clear(void);
    virtual bool read(char* buf = NULL,unsigned int* size = NULL);
    virtual bool write(const char* buf = NULL,unsigned int size = 0);
    virtual bool transfer(bool& read,bool& write,int timeout);
    virtual bool eofread(void);
    virtual bool eofwrite(void);
   public:
    HTTPSClientConnectorGSSAPI(const char* base,bool heavy_encryption,int timeout = 60000,gss_cred_id_t cred = GSS_C_NO_CREDENTIAL,bool check_host=true);
    virtual ~HTTPSClientConnectorGSSAPI(void);
    virtual bool credentials(gss_cred_id_t cred);
  };
  
  
  class HTTPSClient {
   friend class HTTPSClientSOAP;
   private:
    static Logger logger;
    HTTPSClientConnector* c;
    URL base_url;
    std::string proxy_hostname;
    unsigned int proxy_port;
    int timeout;
    bool valid;
    bool connected;
    char answer_buf[256];
    unsigned int answer_size;
    unsigned int answer_count;
    int answer_code;
    std::string answer_reason;
    HTTPResponseHeader fields;
    GSSCredential * cred;
    int parse_response(void);
    void clear_input(void);
    void analyze_response_line(char* line);
    int read_response_header(void);
    int skip_response_entity(void);
    int make_header(const char* path,unsigned long long int offset,unsigned long long int size,unsigned long long int fd_size,std::string& header);
    int GET_header(const char* path,unsigned long long int offset,unsigned long long int size);
   public:
    typedef int (*get_callback_t)(unsigned long long offset,unsigned long long size,unsigned char** buf,unsigned long long* bufsize,void* arg);
    typedef int (*put_callback_t)(unsigned long long offset,unsigned long long *size,char* buf);
    HTTPSClient(const char* base,bool heavy_encryption = true,bool gssapi_server = false, int timeout=60000, bool check_host_cert = true);
    virtual ~HTTPSClient(void);
    operator bool(void) { return valid; };
    bool credentials(const char* filename);
    int connect(void);
    int disconnect(void);
    int PUT(const char* path,unsigned long long int offset,unsigned long long int size,const unsigned char* buf,unsigned long long int fd_size,bool wait = true);
    int GET(const char* path,unsigned long long int offset,unsigned long long int size,get_callback_t callback,void* arg,unsigned char* buf = NULL,unsigned long long int bufsize = 0);
    bool keep_alive(void) const { return fields.KeepAlive(); };
    // unsigned long long int size(void) const { return fields.ContentSize(); };
    const HTTPResponseHeader& response(void) { return fields; };
  };
  
  class HTTPSClientSOAP: public HTTPSClient {
   private:
    struct ::soap *soap;
    struct Namespace* namespaces;
    static int local_fsend(struct soap *sp, const char* buf, size_t l);
    static size_t local_frecv(struct soap* sp, char* buf, size_t l);
    static int local_fopen(struct soap* sp, const char* endpoint, const char* host, int port);
    static int local_fclose(struct soap* sp);
    std::string soap_url;
   public:
    HTTPSClientSOAP(const char* base,struct soap *sp,bool gssapi_server = false, int soap_timeout = 60, bool check_host_cert = true);
    ~HTTPSClientSOAP(void);
    const char* SOAP_URL(void) { return soap_url.c_str(); };
    std::string SOAP_URL(const char* path);
    void reset(void);
    void AddSOAPNamespaces(struct Namespace* namespaces);
    const struct Namespace* Namespaces(void);
  };

} // namespace Arc

#endif /*  __HTTPS__CLIENT_H__ */
