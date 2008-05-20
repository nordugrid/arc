#ifndef __DATACALLBACK_H__
#define __DATACALLBACK_H__

namespace Arc {

  /// This class is used by DataHandle to report missing space on local
  /// filesystem. One of 'cb' functions here will be called if operation
  /// initiated by DataHandle::start_reading runs out of disk space.
  class DataCallback {
  public:
    DataCallback() {}
    virtual ~DataCallback() {}
    virtual bool cb(int) {
      return 0;
    }
    virtual bool cb(unsigned int) {
      return 0;
    }
    virtual bool cb(long long int) {
      return 0;
    }
    virtual bool cb(unsigned long long int) {
      return 0;
    }
  };

} // namespace Arc

#endif // __DATACALLBACK_H__
