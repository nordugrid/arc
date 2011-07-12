// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAHANDLE_H__
#define __ARC_DATAHANDLE_H__

#include <arc/data/DataPoint.h>

namespace Arc {

  class URL;
  class UserConfig;

  /// This class is a wrapper around the DataPoint class.
  /** It simplifies the construction, use and destruction of
   * DataPoint objects and should be used instead of DataPoint
   * classes directly. The appropriate DataPoint subclass is
   * created automatically and stored internally in DataHandle.
   * A DataHandle instance can be thought of as a pointer to
   * the DataPoint instance and the DataPoint can be accessed
   * through the usual dereference operators. A DataHandle
   * cannot be copied.
   */

  class DataHandle {
  public:
    /// Construct a new DataHandle
    DataHandle(const URL& url, const UserConfig& usercfg)
      : p(loader.load(url, usercfg)) {}
    /// Destructor
    ~DataHandle() {
      if (p)
        delete p;
    }
    /// Returns a pointer to a DataPoint object
    DataPoint* operator->() {
      return p;
    }
    /// Returns a const pointer to a DataPoint object
    const DataPoint* operator->() const {
      return p;
    }
    /// Returns a reference to a DataPoint object
    DataPoint& operator*() {
      return *p;
    }
    /// Returns a const reference to a DataPoint object
    const DataPoint& operator*() const {
      return *p;
    }
    /// Returns true if the DataHandle is not valid
    bool operator!() const {
      return !p;
    }
    /// Returns true if the DataHandle is valid
    operator bool() const {
      return !!p;
    }
  private:
    /// Pointer to specific DataPoint instance
    DataPoint *p;
    static DataPointLoader loader;
    /// Private default constructor
    DataHandle(void);
    /// Private copy constructor and assignment operator because DataHandle
    /// should not be copied.
    DataHandle(const DataHandle&);
    DataHandle& operator=(const DataHandle&);
  };

} // namespace Arc

#endif // __ARC_DATAHANDLE_H__
