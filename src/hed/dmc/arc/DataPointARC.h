// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTARC_H__
#define __ARC_DATAPOINTARC_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>
#include <arc/data/DataHandle.h>
#include <arc/CheckSum.h>

namespace Arc {

  /**
   * Provides an interface to the Chelonia storage system developed by ARC.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointARC
    : public DataPointDirect {
  private:
    static Logger logger;
    DataHandle *transfer;
    bool reading;
    bool writing;
    URL bartender_url;
    URL turl;
    MD5Sum *md5sum;
    int chksum_index;
    bool checkBartenderURL(const URL& bartender_url);
  public:
    DataPointARC(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointARC();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false) { return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP); };
    virtual DataStatus Stat(FileInfo& file, DataPoint::DataPointInfoType verb);
    virtual DataStatus List(std::list<FileInfo>& file, DataPoint::DataPointInfoType verb);
    virtual DataStatus Rename(const URL& newurl) { return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP); };
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual bool RequiresCredentials() const { return bartender_url.Protocol() != "http"; };
  };

} // namespace Arc

#endif // __ARC_DATAPOINTARC_H__
