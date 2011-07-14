// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTARC_H__
#define __ARC_DATAPOINTARC_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>
#include <arc/data/DataHandle.h>
#include <arc/data/CheckSum.h>

namespace Arc {

  /**
   * Provides an interface to the Chelonia storage system developed by ARC.
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
    DataPointARC(const URL& url, const UserConfig& usercfg);
    virtual ~DataPointARC();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus Check();
    virtual DataStatus Remove();
    virtual DataStatus Stat(FileInfo& file, DataPoint::DataPointInfoType verb);
    virtual DataStatus List(std::list<FileInfo>& file, DataPoint::DataPointInfoType verb);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTARC_H__
