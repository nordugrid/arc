// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGFAL_H__
#define __ARC_DATAPOINTGFAL_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace ArcDMCGFAL {

  using namespace Arc;

  /**
   * Provides access to the gLite Grid File Access Library through ARC's API.
   * The following protocols are supported: lfc, srm, root, gsiftp, rfio, dcap
   * and gsidcap.
   *
   * Notes on env variables:
   *  - If SRM is used LCG_GFAL_INFOSYS must be set to BDII host:port unless
   *    the full URL with port and web service path is given.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointGFAL
    : public DataPointDirect {
  public:
    DataPointGFAL(const URL& url, const UserConfig& usercfg, const std::string& transfer_url, PluginArgument* parg);
    virtual ~DataPointGFAL();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StopReading();
    virtual DataStatus StartWriting(DataBuffer& buffer, DataCallback *space_cb = NULL);
    virtual DataStatus StopWriting();
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& newurl);
    virtual bool RequiresCredentialsInFile() const;
    // Even though this is not a DataPointIndex, it still needs to handle
    // locations so Resolve and AddLocation must be implemented
    virtual DataStatus Resolve(bool source = true);
    virtual DataStatus AddLocation(const URL& url, const std::string& meta);

  protected:
    // 3rd party transfer (destination pulls from source)
    virtual DataStatus Transfer3rdParty(const URL& source, const URL& destination, DataPoint::Callback3rdParty callback = NULL);

  private:
    DataStatus do_stat(const URL& stat_url, FileInfo& file, DataPointInfoType verb);
    static void read_file_start(void *object);
    void read_file();
    static void write_file_start(void *object);
    void write_file();
    static Logger logger;
    int fd;
    bool reading;
    bool writing;
    SimpleCounter transfer_condition;
    std::string lfc_host;
    std::list<URLLocation> locations;
  };

} // namespace ArcDMCGFAL

#endif // __ARC_DATAPOINTGFAL_H__
