// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGRIDFTP_H__
#define __ARC_DATAPOINTGRIDFTP_H__

#include <list>
#include <string>

#include <globus_common.h>
#include <globus_ftp_client.h>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>
#include <arc/URL.h>
#include <arc/globusutils/GSSCredential.h>

namespace ArcDMCGridFTP {

  using namespace Arc;

  class Lister;

  /**
   * GridFTP is essentially the FTP protocol with GSI security. This class
   * uses libraries from the Globus Toolkit. It can also be used for regular
   * FTP.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointGridFTP
    : public DataPointDirect {
  private:
    class CBArg {
    private:
      Glib::Mutex lock;
      DataPointGridFTP* arg;
      CBArg(void);
      CBArg(const CBArg&);
    public:
      CBArg(DataPointGridFTP* a);
      ~CBArg(void) {};
      DataPointGridFTP* acquire(void);
      void release(void);
      void abandon(void);
    };

    static Logger logger;
    CBArg* cbarg;
    bool ftp_active;
    globus_ftp_client_handle_t ftp_handle;
    globus_ftp_client_operationattr_t ftp_opattr;
    globus_thread_t ftp_control_thread;
    int ftp_threads;
    bool autodir;

    SimpleCondition cond;
    DataStatus callback_status;
    GSSCredential *credential;

    bool reading;
    bool writing;

    bool ftp_eof_flag;
    int check_received_length;

    bool data_error;
    SimpleCounter data_counter;

    Lister* lister;

    static void ftp_complete_callback(void *arg,
                                      globus_ftp_client_handle_t *handle,
                                      globus_object_t *error);
    static void ftp_get_complete_callback(void *arg,
                                          globus_ftp_client_handle_t *handle,
                                          globus_object_t *error);
    static void ftp_put_complete_callback(void *arg,
                                          globus_ftp_client_handle_t *handle,
                                          globus_object_t *error);
    static void ftp_read_callback(void *arg,
                                  globus_ftp_client_handle_t *handle,
                                  globus_object_t *error,
                                  globus_byte_t *buffer, globus_size_t length,
                                  globus_off_t offset, globus_bool_t eof);
    static void ftp_check_callback(void *arg,
                                   globus_ftp_client_handle_t *handle,
                                   globus_object_t *error,
                                   globus_byte_t *buffer, globus_size_t length,
                                   globus_off_t offset, globus_bool_t eof);
    static void ftp_write_callback(void *arg,
                                   globus_ftp_client_handle_t *handle,
                                   globus_object_t *error,
                                   globus_byte_t *buffer, globus_size_t length,
                                   globus_off_t offset, globus_bool_t eof);
    static void* ftp_read_thread(void *arg);
    static void* ftp_write_thread(void *arg);

    bool mkdir_ftp();
    char ftp_buf[16];
    bool check_credentials();
    void set_attributes();
    DataStatus RemoveFile();
    DataStatus RemoveDir();
    DataStatus do_more_stat(FileInfo& f, DataPointInfoType verb);
  public:
    DataPointGridFTP(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointGridFTP();
    static Plugin* Instance(PluginArgument *arg);
    virtual bool SetURL(const URL& url);
    virtual DataStatus StartReading(DataBuffer& buf);
    virtual DataStatus StartWriting(DataBuffer& buf,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Rename(const URL& newurl);
    virtual bool WriteOutOfOrder();
    virtual bool ProvidesMeta() const;
    virtual const std::string DefaultCheckSum() const;
    virtual bool RequiresCredentials() const;
  };

} // namespace ArcDMCGridFTP

#endif // __ARC_DATAPOINTGRIDFTP_H__
