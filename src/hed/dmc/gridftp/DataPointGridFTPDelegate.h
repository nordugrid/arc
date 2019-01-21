// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGRIDFTPDELEGATE_H__
#define __ARC_DATAPOINTGRIDFTPDELEGATE_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>
#include <arc/URL.h>
#include <arc/Run.h>
#include <arc/Utils.h>
//#include <arc/globusutils/GSSCredential.h>

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
  class DataPointGridFTPDelegate
    : public DataPointDirect {
  private:
    class LogRedirect: public Run::Data {
     public:
      LogRedirect(): level_(FATAL) { };
      virtual ~LogRedirect() { Flush(); };
      virtual void Append(char const* data, unsigned int size);
      void Flush();
    private:
      // for sanity checks
      static std::string::size_type const level_size_max_;
      static std::string::size_type const buffer_size_max_;
      LogLevel level_;
      std::string buffer_;
    };

    static Logger logger;
    LogRedirect log_redirect;
    int ftp_threads;
    bool autodir;

    SimpleCondition cond;

    bool reading;
    bool writing;
    Arc::CountedPointer<Run> ftp_run;
    DataStatus data_status;

    bool ftp_eof_flag;

    static void ftp_read_thread(void *arg);
    static void ftp_write_thread(void *arg);

    DataStatus StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataBuffer& buf, DataStatus::DataStatusType errCode);
    DataStatus StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataStatus::DataStatusType errCode);
    DataStatus EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode);
    DataStatus EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode, char tag);

  public:
    DataPointGridFTPDelegate(const URL& url, const UserConfig& usercfg, const std::string& transfer_url, PluginArgument* parg);
    virtual ~DataPointGridFTPDelegate();
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

#endif // __ARC_DATAPOINTGRIDFTPDELEGATE_H__

