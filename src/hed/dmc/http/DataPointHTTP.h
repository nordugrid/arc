#ifndef __ARC_DATAPOINTHTTP_H__
#define __ARC_DATAPOINTHTTP_H__

#include <string>
#include <list>

#include <arc/data/DataPointDirect.h>

namespace Arc {

  class URL;

  class DataPointHTTP : public DataPointDirect {
   private:
    static Logger logger;
   public:
    DataPointHTTP(const URL& url);
    virtual ~DataPointHTTP();
    virtual bool list_files(std::list<FileInfo>& files, bool resolve = true);
    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTHTTP_H__
