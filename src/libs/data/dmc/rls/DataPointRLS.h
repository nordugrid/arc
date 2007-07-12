#ifndef __ARC_DATAPOINTRLS_H__
#define __ARC_DATAPOINTRLS_H__

#include "data/DataPoint.h"

namespace Arc {
  class DMCRLS;
  class URL;

  class DataPointRLS : public DataPoint {
   public:
    DataPointRLS(DMCRLS* dmc, const URL& url);
    ~DataPointRLS() {};
    virtual unsigned long long int GetSize();
   private:
    DMCRLS* dmc;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTRLS_H__
