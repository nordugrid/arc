// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTDELEGATE_H__
#define __ARC_DATAPOINTDELEGATE_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/Run.h>
#include <arc/Utils.h>
#include "DataPointDirect.h"

namespace Arc {


  class DataPointDelegate: public DataPointDirect {
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

    SimpleCondition cond;

    bool reading;
    bool writing;
    Arc::CountedPointer<Run> helper_run;
    DataStatus data_status;
    std::string exec_path;
    std::list<std::string> additional_args;

    static void read_thread(void *arg);
    static void write_thread(void *arg);

    DataStatus StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataBuffer& buf, DataStatus::DataStatusType errCode);
    DataStatus StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataStatus::DataStatusType errCode);
    DataStatus EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode);
    DataStatus EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode, char tag);

  protected:
    virtual DataStatus Transfer3rdParty(const URL& source, const URL& destination, TransferCallback callback = NULL);

  public:
    static char const * ReadCommand;
    static char const * WriteCommand;
    static char const * MkdirCommand;
    static char const * MkdirRecursiveCommand;
    static char const * CheckCommand;
    static char const * RemoveCommand;
    static char const * StatCommand;
    static char const * ListCommand;
    static char const * RenameCommand;
    static char const * TransferFromCommand;
    static char const * TransferToCommand;
    static char const * Transfer3rdCommand;

    /// Create object which starts special executable which loads specified module
    DataPointDelegate(char const* module_name, const URL& url, const UserConfig& usercfg, PluginArgument* parg);

    /// Create object which starts specified external executable
    DataPointDelegate(char const* exec_path, std::list<std::string> const & extra, const URL& url, const UserConfig& usercfg, PluginArgument* parg);

    virtual ~DataPointDelegate();

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
    virtual DataStatus Transfer(const URL& otherendpoint, bool source, TransferCallback callback = NULL);
    virtual bool WriteOutOfOrder();
    virtual bool ProvidesMeta() const;
    virtual const std::string DefaultCheckSum() const;
    virtual bool RequiresCredentials() const;
  };

} // namespace ArcDMCGridFTP

#endif // __ARC_DATAPOINTDELEGATE_H__

