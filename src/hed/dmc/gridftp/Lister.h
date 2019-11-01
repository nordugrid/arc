// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_LISTER_H__
#define __ARC_LISTER_H__

#include <list>
#include <string>

#include <arc/data/DataStatus.h>
#include <arc/data/FileInfo.h>
#include <arc/URL.h>
#include <arc/Thread.h>
#include <arc/globusutils/GSSCredential.h>

#include <globus_ftp_control.h>

#define LISTER_MAX_RESPONSES 3

namespace ArcDMCGridFTP {

  using namespace Arc;

  class Lister {
  private:
    bool inited;
    bool facts;
    char readbuf[4096];
    globus_cond_t cond;
    globus_mutex_t mutex;
    globus_ftp_control_handle_t *handle;
    std::list<FileInfo> fnames;
    globus_ftp_control_response_t resp[LISTER_MAX_RESPONSES];
    int resp_n;
    enum callback_status_t {
      CALLBACK_NOTREADY = 0,
      CALLBACK_DONE = 1,
      CALLBACK_ERROR = 2,
      CALLBACK_TIMEDOUT = 3
    };
    callback_status_t callback_status;
    callback_status_t data_callback_status;
    callback_status_t close_callback_status;
    globus_off_t list_shift;
    bool connected;
    bool pasv_set;
    bool data_activated;
    bool free_format;
    unsigned short int port;
    std::string host;
    std::string username;
    std::string userpass;
    std::string path;
    std::string scheme;
    std::string urlstr;
    GSSCredential* credential;

    static unsigned int const max_timeout = (60*20); // 20 minutes is really long
    callback_status_t wait_for_callback(unsigned int to = max_timeout);
    callback_status_t wait_for_data_callback(unsigned int to = max_timeout);
    callback_status_t wait_for_close_callback(unsigned int to = max_timeout);
    void resp_destroy();

    static void resp_callback(void *arg, globus_ftp_control_handle_t *h,
                              globus_object_t *error,
                              globus_ftp_control_response_t *response);
    static void close_callback(void *arg, globus_ftp_control_handle_t *h,
                              globus_object_t *error,
                              globus_ftp_control_response_t *response);
    static void simple_callback(void *arg, globus_ftp_control_handle_t *h,
                                globus_object_t *error);
    static void list_read_callback(void *arg,
                                   globus_ftp_control_handle_t *hctrl,
                                   globus_object_t *error,
                                   globus_byte_t *buffer, globus_size_t length,
                                   globus_off_t offset, globus_bool_t eof);
    static void list_conn_callback(void *arg,
                                   globus_ftp_control_handle_t *hctrl,
                                   unsigned int stripe_ndx,
                                   globus_bool_t reused,
                                   globus_object_t *error);

    static std::map<void*,Lister*> callback_args;
    static Glib::Mutex callback_args_mutex;
    static void* remember_for_callback(Lister* it);
    static Lister* recall_for_callback(void* arg);
    static void forget_about_callback(void* arg);
    void* callback_arg;

    globus_ftp_control_response_class_t
    send_command(const char *command, const char *arg, bool wait_for_response,
                 char **sresp = NULL, int *code = NULL, char delim = 0);
    DataStatus setup_pasv(globus_ftp_control_host_port_t& pasv_addr);
    DataStatus handle_connect(const URL& url);
    DataStatus transfer_list(void);
    void close_connection();
    Lister(Lister const&);
    Lister& operator=(Lister const&);

  public:
    Lister();
    ~Lister();
    void set_credential(GSSCredential* cred) { credential = cred; };
    DataStatus retrieve_dir_info(const URL& url,bool names_only = false);
    DataStatus retrieve_file_info(const URL& url,bool names_only = false);
    operator bool() {
      return inited;
    }
    std::list<FileInfo>::iterator begin() {
      return fnames.begin();
    }
    std::list<FileInfo>::iterator end() {
      return fnames.end();
    }
    int size() const {
      return fnames.size();
    }
  };

} // namespace ArcDMCGridFTP

#endif // __ARC_LISTER_H__
