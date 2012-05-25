#ifndef __ARC_WATCHDOG_H__
#define __ARC_WATCHDOG_H__

namespace Arc {

  /// Internal implementation of watchdog.
  /// Currently only single global watchdog is supported.
  class Watchdog;

  /// This class is meant to provide interface
  /// for watchdog executor part.
  class WatchdogListener {
  private:
    Watchdog& instance_;

  public:
    WatchdogListener(void);

    /// Waits till timeout occures and then returns true.
    /// If any error occures it retruns false and watchdog
    /// is normally not usable anymore.
    bool Listen(void);
  };

  /// This class is meant to be used in code
  /// which provides "I'm alive" ticks to watchdog.
  class WatchdogChannel {
  private:
    int id_;

  public:

    /// Defines watchdog kicking source with specified timeout.
    /// Code must call Kick() method of this instance to keep
    /// watchdog from timeouting. If object is destroyed watchdog
    /// does not monitor it anymore. Althogh timeout is specified
    /// in seconds real time resolution of watchdog is about 1 minute.
    WatchdogChannel(int timeout);

    /// Upon destruction channel is closed and watchdog forgets about it.
    ~WatchdogChannel(void);

    /// Tells watchdog this source is still alive.
    void Kick(void);
  };

} // namespace Arc

#endif // __ARC_WATCHDOG_H__

