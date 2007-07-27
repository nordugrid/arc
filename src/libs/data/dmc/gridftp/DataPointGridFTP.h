#ifndef __ARC_DATAPOINTGRIDFTP_H__
#define __ARC_DATAPOINTGRIDFTP_H__

#include <string>
#include <list>

#include <globus_common.h>
#include <globus_ftp_client.h>

#include "data/DataPointDirect.h"
#include <glibmm.h>

namespace Arc {

  class URL;

  class DataPointGridFTP : public DataPointDirect {
   private:
    static Logger logger;
    bool ftp_active;
    globus_ftp_client_handle_t ftp_handle;
    globus_ftp_client_operationattr_t ftp_opattr;
    globus_thread_t ftp_control_thread;
    int ftp_threads;

    Glib::Cond cond;
    Glib::Mutex mutex;
    int condstatus;

    bool ftp_eof_flag;

    static void ftp_complete_callback
      (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error);
    static void ftp_get_complete_callback
      (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error);
    static void ftp_put_complete_callback
      (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error);
    static void ftp_read_callback
      (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error,
       globus_byte_t *buffer, globus_size_t length, globus_off_t offset,
       globus_bool_t eof);
    static void ftp_check_callback
      (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error,
       globus_byte_t *buffer, globus_size_t length, globus_off_t offset,
       globus_bool_t eof);
    static void ftp_write_callback
      (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error,
       globus_byte_t *buffer, globus_size_t length, globus_off_t offset,
       globus_bool_t eof);
    static void *ftp_read_thread(void *arg);
    static void *ftp_write_thread(void *arg);

    std::string ftp_dir_path;
    bool mkdir_ftp();
    char ftp_buf[16];
    bool check_credentials();
   protected:
    virtual bool init_handle();
    virtual bool deinit_handle();
   public:
    DataPointGridFTP(const URL& url);
    virtual ~DataPointGridFTP();
    virtual bool analyze(analyze_t& arg);
    virtual bool start_reading(DataBufferPar& buf);
    virtual bool start_writing(DataBufferPar& buf,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading();
    virtual bool stop_writing();
    virtual bool check();
    virtual bool remove();
    virtual bool list_files(std::list<FileInfo>& files, bool resolve = true);
    virtual bool out_of_order();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTGRIDFTP_H__
