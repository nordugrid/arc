#ifndef __ARC_DATAHANDLE_H__
#define __ARC_DATAHANDLE_H__

#include "DataPoint.h"
#include "DMC.h"

namespace Arc {

  class URL;

  /// This clas is a wrapper around the DataPoint class.
  /** It simplifies the construction and use and destruction of 
    DataPoint objects. */

  class DataHandle {
   public:
    DataHandle() : p(NULL) {};
    DataHandle(const URL& url) : p(DMC::GetDataPoint(url)) {};
    ~DataHandle() { if (p) delete p; };
    DataPoint* operator->() { return p; };
    bool operator!() { return !p; };
    operator bool() { return !!p; };
   private:
    DataPoint *p;
  };

} // namespace Arc

#endif // __ARC_DATAHANDLE_H__
