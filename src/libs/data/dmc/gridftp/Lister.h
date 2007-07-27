#ifndef __ARC_LISTER_H__
#define __ARC_LISTER_H__

#include <string>
#include <list>

#include "common/DateTime.h"

#include <globus_ftp_control.h>

#define LISTER_MAX_RESPONSES 3

namespace Arc {

  class Time;
  class URL;

  class ListerFile {
   public:
    typedef enum {
      file_type_unknown = 0,
      file_type_file = 1,
      file_type_dir = 2
    } Type;
   private:
    std::string name;
    unsigned long long int size;
    Time created;
    Type type;
   public:
    ListerFile(const std::string& name) : name(name), size(-1), created(-1),
					  type(file_type_unknown) {};
    ~ListerFile(void) {};

    const std::string& GetName() const {
      return name;
    };
    const char *GetLastName() const;
    bool CheckSize() const {
      return (size != -1);
    };
    unsigned long long int GetSize() const {
      return size;
    };
    bool CheckCreated() const {
      return (created != -1);
    };
    Time GetCreated() const {
      return created;
    };
    bool CheckType() const {
      return (type != file_type_unknown);
    };
    Type GetType() const {
      return type;
    };
    bool SetAttributes(const char *facts);
  };

  class Lister {
   private:
    bool inited;
    bool facts;
    char readbuf[4096];
    globus_cond_t cond;
    globus_mutex_t mutex;
    globus_ftp_control_handle_t *handle;
    std::list<ListerFile> fnames;
    globus_ftp_control_response_t resp[LISTER_MAX_RESPONSES];
    int resp_n;
    typedef enum callback_status_t {
      CALLBACK_NOTREADY = 0,
      CALLBACK_DONE = 1,
      CALLBACK_ERROR = 2
    };
    callback_status_t callback_status;
    callback_status_t data_callback_status;
    globus_off_t list_shift;
    bool connected;
    unsigned short int port;
    std::string host;
    std::string username;
    std::string userpass;
    std::string path;
    std::string scheme;

    callback_status_t wait_for_callback();
    callback_status_t wait_for_data_callback();
    void resp_destroy();
    static void resp_callback(void *arg, globus_ftp_control_handle_t *h,
                              globus_object_t *error,
                              globus_ftp_control_response_t *response);
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
    globus_ftp_control_response_class_t
    send_command(const char *command, const char *arg, bool wait_for_response,
                 char **sresp, char delim = 0);
    int setup_pasv(globus_ftp_control_host_port_t& pasv_addr);

   public:
    Lister();
    ~Lister();
    int retrieve_dir(const URL& url);
    operator bool() {
      return inited;
    };
    std::list<ListerFile>::iterator begin() {
      return fnames.begin();
    };
    std::list<ListerFile>::iterator end() {
      return fnames.end();
    };
    int size() const {
      return fnames.size();
    };
    int close_connection();
  };

} // namespace Arc

#endif // __ARC_LISTER_H__
