// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGFAL_H__
#define __ARC_DATAPOINTGFAL_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  /**
   * Provides access to the gLite Grid File Access Library through ARC's API.
   * Only rfio:// protocol is supported at the moment.
   *
   * Notes on env variables:
   *  - If LFC plugin is installed LFC_HOST must be set (lcgutils bug 322)
   *  - If SRM is used LCG_GFAL_INFOSYS must be set to BDII host:port
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointGFAL
    : public DataPointDirect {
  public:
    DataPointGFAL(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointGFAL();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StopReading();
    virtual DataStatus StartWriting(DataBuffer& buffer, DataCallback *space_cb = NULL);
    virtual DataStatus StopWriting();
    virtual DataStatus Check();
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& newurl);
  private:
    DataStatus do_stat(const URL& stat_url, FileInfo& file);
    void log_gfal_err();
    static void read_file_start(void *object);
    void read_file();
    static void write_file_start(void *object);
    void write_file();
    static Logger logger;
    int fd;
    bool reading;
    bool writing;
    SimpleCondition transfer_condition;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTGFAL_H__
