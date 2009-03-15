// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAHANDLE_H__
#define __ARC_DATAHANDLE_H__

#include <arc/data/DataPoint.h>
#include <arc/data/DMC.h>

namespace Arc {

  class URL;

  /// This class is a wrapper around the DataPoint class.
  /** It simplifies the construction, use and destruction of
     DataPoint objects. */

  class DataHandle {
  public:
    DataHandle()
      : p(NULL) {}
    DataHandle(const URL& url)
      : p(DMC::GetDataPoint(url)) {}
    ~DataHandle() {
      if (p)
        delete p;
    }
    DataHandle& operator=(const URL& url) {
      if (p)
        delete p;
      p = DMC::GetDataPoint(url);
      return *this;
    }
    void Clear() {
      if (p)
        delete p;
      p = NULL;
    }
    DataPoint* operator->() {
      return p;
    }
    const DataPoint* operator->() const {
      return p;
    }
    DataPoint& operator*() {
      return *p;
    }
    const DataPoint& operator*() const {
      return *p;
    }
    bool operator!() const {
      return !p;
    }
    operator bool() const {
      return !!p;
    }
  private:
    DataPoint *p;
  };

} // namespace Arc

#endif // __ARC_DATAHANDLE_H__
