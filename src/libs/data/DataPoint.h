#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include "common/URL.h"

namespace Arc {
  class DataPoint {
   protected:
    DataPoint(const URL& url): url(url), size(-1) {};
   public:
    virtual ~DataPoint() {};
    virtual unsigned long long int GetSize() = 0;
   protected:
    URL url;
    unsigned long long int size;
  };

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
