#ifndef __ARC_DATAPOINTXROOTD_H__
#define __ARC_DATAPOINTXROOTD_H__

#include <list>
#include <XrdPosix/XrdPosixXrootd.hh>
#include <XrdCl/XrdClCopyProcess.hh>

#include <arc/data/DataPointDirect.h>

namespace ArcDMCXrootd {

  using namespace Arc;

  /**
   * Progress handler class that is used to pass data to callback in Transfer()
   */
  class XrootdProgressHandler : public XrdCl::CopyProgressHandler {
   public:
    XrootdProgressHandler(DataPoint::Callback3rdParty callback);
    void JobProgress(uint16_t jobNum, uint64_t bytesProcessed, uint64_t bytesTotal);
    bool ShouldCancel(uint64_t jobNum);
   private:
    DataPoint::Callback3rdParty cb;
    bool cancel;
  };

  /**
   * xrootd is a protocol for data access across large scale storage clusters.
   * More information can be found at http://xrootd.slac.stanford.edu/
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointXrootd
    : public DataPointDirect {

   public:
    DataPointXrootd(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointXrootd();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& newurl);
    virtual DataStatus Transfer(const URL& otherendpoint, bool source, Callback3rdParty callback = NULL);
    virtual bool SupportsTransfer() const;
    virtual bool RequiresCredentialsInFile() const;

   private:
    /// thread functions for async read/write
    static void read_file_start(void* arg);
    static void write_file_start(void* arg);
    void read_file();
    void write_file();
    DataStatus copy_file(std::string source, std::string dest, Callback3rdParty callback = NULL);

    /// must be called everytime a new XrdClient is created
    void set_log_level();
    /// Internal stat()
    DataStatus do_stat(const URL& url, FileInfo& file, DataPointInfoType verb);

    int fd;
    SimpleCondition transfer_cond;
    bool reading;
    bool writing;
    static Logger logger;
    // There must be one instance of this object per executable
    static XrdPosixXrootd xrdposix;
  };

} // namespace ArcDMCXrootd

#endif /* __ARC_DATAPOINTXROOTD_H__ */
