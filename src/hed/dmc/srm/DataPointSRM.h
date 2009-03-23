// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTSRM_H__
#define __ARC_DATAPOINTSRM_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>
#include <arc/data/DataHandle.h>

#include "srmclient/SRMClient.h"

namespace Arc {

  class DataPointSRM
    : public DataPointDirect {
  public:
    DataPointSRM(const URL& url);
    virtual ~DataPointSRM();
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check();
    virtual DataStatus Remove();
    virtual DataStatus ListFiles(std::list<FileInfo>& files,
                                 bool long_list = false,
                                 bool resolve = false,
                                 bool metadata = false);
  private:
    SRMClientRequest *srm_request;
    static Logger logger;
    DataHandle *r_handle;  /* handle used for redirected operations */
    bool reading;
    bool writing;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTSRM_H__
