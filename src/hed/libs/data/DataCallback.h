// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATACALLBACK_H__
#define __ARC_DATACALLBACK_H__

namespace Arc {

  /// Callbacks to be used when there is not enough space on the local filesystem.
  /**
   * If DataPoint::StartWriting() tries to pre-allocate disk space but finds
   * that there is not enough to write the whole file, one of the 'cb'
   * functions here will be called with the required space passed as a
   * parameter. Users should define their own subclass of this class depending
   * on how they wish to free up space. Each callback method should return true
   * if the space was freed, false otherwise. This subclass should then be used
   * as a parameter in StartWriting().
   * \ingroup data
   * \headerfile DataCallback.h arc/data/DataCallback.h
   */
  class DataCallback {
  public:
    /// Construct a new DataCallback
    DataCallback() {}
    /// Empty destructor
    virtual ~DataCallback() {}
    /// Callback with int passed as parameter
    virtual bool cb(int) {
      return false;
    }
    /// Callback with unsigned int passed as parameter
    virtual bool cb(unsigned int) {
      return false;
    }
    /// Callback with long long int passed as parameter
    virtual bool cb(long long int) {
      return false;
    }
    /// Callback with unsigned long long int passed as parameter
    virtual bool cb(unsigned long long int) {
      return false;
    }
  };

} // namespace Arc

#endif // __ARC_DATACALLBACK_H__
