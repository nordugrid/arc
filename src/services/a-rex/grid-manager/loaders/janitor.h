#include <string>

#include <arc/Thread.h>

/// Class to communicate with Janitor - Dynmaic Runtime Environment handler
class Janitor {
 public:
  typedef enum {
    DEPLOYED,
    REMOVED,
    NOTENABLED,
    FAILED
  } Result;
 private:
  bool enabled_;
  std::string path_;
  std::string id_;
  std::string cdir_;
  bool running_;
  Result result_;
  Arc::SimpleCondition completed_;
  Arc::SimpleCondition cancel_;
  static void deploy_thread(void* arg);
  static void remove_thread(void* arg);
  void cancel(void);
  bool readConfig();
 public:
  /// Creates instance representing job entry in Janitor database
  /** Takes id for job identifier and cdir for the control 
     directory of A-Rex. constructor does not register job in 
     the Janitor. It only associates job with this instance. */
  Janitor(const std::string& id,const std::string& cdir);
  ~Janitor(void);
  /// Returns true if janitor is enabled in the config file.
  bool enabled() { return enabled_; };
  /// Returns true if instance is valid
  operator bool(void) { return !path_.empty(); };
  /// Returns true if instance is invalid
  bool operator!(void) { return path_.empty(); };
  /// Registers associated job with Janitor and deploys dynamic RTEs.
  /** This operation is asynchronous. Returned true means Janitor
     will be contacted and deployemnt will start soon. For obtaining
     result of operation see methods wait() and result().
     During this operation janitor utility is called with command
     register and optionally deploy. */
  bool deploy(void);
  /// Removes job from those handled by Janitor and releases associated RTEs.
  /** This operation is asynchronous. Returned true means Janitor
     will be contacted and removal will start soon. For obtaining
     result of operation see methods wait() and result().
     During this operation janitor utility is called with command
     remove. */
  bool remove(void);
  /// Wait till operation initiated by deploy() or remove() finished.
  /** This operation returns true if operation finished or false if
     timeout seconds passed. It may be called repeatedly and even after
     it previously returned true. If no operation is running it returns
     true immeaditely. */
  bool wait(int timeout);
  /// Returns true if operation initiated by deploy() or remove() succeeded.
  /** It should be called after wait() returned true. */
  Result result(void);
};

