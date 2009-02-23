#ifndef __ARC_DATAPOINTARC_H__
#define __ARC_DATAPOINTARC_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  class URL;

  class DataPointARC
    : public DataPointDirect {
  private:
    static Logger logger;
  public:
    DataPointARC(const URL& url);
    virtual ~DataPointARC();
    DataStatus Check();
    DataStatus Remove();
    DataStatus ListFiles(std::list<FileInfo>& files, bool long_list = false, bool resolve = false);
    DataStatus StartReading(DataBuffer& buffer);
    DataStatus StartWriting(DataBuffer& buffer,
                            DataCallback *space_cb = NULL);
    DataStatus StopReading();
    DataStatus StopWriting();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTARC_H__
