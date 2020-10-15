// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGRIDFTPHELPER_H__
#define __ARC_DATAPOINTGRIDFTPHELPER_H__

#include <list>
#include <string>

#include <globus_common.h>
#include <globus_ftp_client.h>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/data/DataPoint.h>
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
  class DataPointGridFTPHelper {
  private:
    class CBArg {
    private:
      Glib::Mutex lock;
      DataPointGridFTPHelper* arg;
      CBArg(void);
      CBArg(const CBArg&);
    public:
      CBArg(DataPointGridFTPHelper* a);
      ~CBArg(void) {};
      DataPointGridFTPHelper* acquire(void);
      void release(void);
      void abandon(void);
    };

    bool ftp_active;
    URL url;
    UserConfig usercfg;
    std::istream& instream;
    std::ostream& outstream;

    bool is_secure;

    CBArg* cbarg;
    globus_ftp_client_handle_t ftp_handle;
    globus_ftp_client_operationattr_t ftp_opattr;
    globus_thread_t ftp_control_thread;

    // Configuration parameters
    bool force_secure;
    bool force_passive;
    int ftp_threads; // Number of transmisison threads/buffers to register
    unsigned long long int ftp_bufsize; // Size of each buffer passed to Globus code
    unsigned long long int range_start;
    unsigned long long int range_end;
    bool allow_out_of_order;
    bool stream_mode;
    bool autodir;
    DataExternalComm::DataChunkClientList delayed_chunks;
    unsigned long long int max_offset;

    // Current state representation
    SimpleCondition cond;
    DataStatus callback_status;
    GSSCredential *credential;
    bool ftp_eof_flag; // set to true when data callback reports eof
    int check_received_length;
    bool data_error; // set to true when data callback reports error
    SimpleCounter data_counter; // counts number of buffers passed to Globus code
    SimpleCondition data_counter_change;

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
    bool mkdir_ftp();
    char ftp_buf[16];
    bool check_credentials();
    void set_attributes();
    DataStatus RemoveFile();
    DataStatus RemoveDir();
    DataStatus do_more_stat(FileInfo& f, DataPoint::DataPointInfoType verb);
    DataPointGridFTPHelper(DataPointGridFTPHelper const&);
    DataPointGridFTPHelper& operator=(DataPointGridFTPHelper const&);
  public:
    DataPointGridFTPHelper(const URL& url, const UserConfig& usercfg, std::istream& instream, std::ostream& outstream);
    virtual ~DataPointGridFTPHelper();
    operator bool(void) const { return ftp_active; };
    bool operator!(void) const { return !ftp_active; };
    void SetBufferSize(unsigned long long int size) { ftp_bufsize = size; };
    void SetStreams(int streams) { ftp_threads = streams; };
    void SetRange(unsigned long long int start, unsigned long long int end) { range_start = start; range_end = end; };
    void SetSecure(bool secure) { force_secure = secure; };
    void SetPassive(bool passive) { force_passive = passive; };
    void ReadOutOfOrder(bool out_of_order) { allow_out_of_order = out_of_order; }
    DataStatus Read();
    DataStatus Write();
    DataStatus Check();
    DataStatus Remove();
    DataStatus CreateDirectory(bool with_parents=false);
    DataStatus Stat(DataPoint::DataPointInfoType verb = DataPoint::INFO_TYPE_ALL);
    DataStatus List(DataPoint::DataPointInfoType verb = DataPoint::INFO_TYPE_ALL);
    DataStatus Rename(const URL& newurl);

    static Logger logger;
  };

} // namespace ArcDMCGridFTP

#endif // __ARC_DATAPOINTGRIDFTPHELPER_H__
