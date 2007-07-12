#ifndef __ARC_DATAPOINTGRIDFTP_H__
#define __ARC_DATAPOINTGRIDFTP_H__

#include "data/DataPoint.h"

namespace Arc {
  class DMCGridFTP;
  class URL;

  class DataPointGridFTP : public DataPoint {
   public:
    DataPointGridFTP(DMCGridFTP* dmc, const URL& url);
    ~DataPointGridFTP() {};
    virtual unsigned long long int GetSize();
   private:
    DMCGridFTP* dmc;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTGRIDFTP_H__
