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

  class DataPointARC
    : public DataPointDirect {
  private:
    static Logger logger;
    DataHandle *transfer;
    bool reading;
    bool writing;
    URL bartender_url;
    MD5Sum *md5sum;
    int chksum_index;
  public:
    DataPointARC(const URL& url, const UserConfig& usercfg);
    virtual ~DataPointARC();
    static Plugin* Instance(PluginArgument *arg);
    DataStatus Check();
    DataStatus Remove();
    DataStatus ListFiles(std::list<FileInfo>& files, bool long_list = false,
                         bool resolve = false, bool metadata = false);
    DataStatus StartReading(DataBuffer& buffer);
    DataStatus StartWriting(DataBuffer& buffer,
                            DataCallback *space_cb = NULL);
    DataStatus StopReading();
    DataStatus StopWriting();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTARC_H__
