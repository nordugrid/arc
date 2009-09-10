// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAHANDLE_H__
#define __ARC_DATAHANDLE_H__

#include <arc/data/DataPoint.h>

namespace Arc {

  class URL;
  class UserConfig;

  /// This class is a wrapper around the DataPoint class.
  /** It simplifies the construction, use and destruction of
     DataPoint objects. */

  class DataHandle {
  public:
    DataHandle(const URL& url, const UserConfig& usercfg)
      : p(loader.load(url, usercfg)) {}
    ~DataHandle() {
      if (p)
        delete p;
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
    static DataPointLoader loader;
  };

} // namespace Arc

#endif // __ARC_DATAHANDLE_H__
