// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_RUN_H__
#define __ARC_RUN_H__

#include <glibmm.h>
#include <arc/Thread.h>
#include <arc/DateTime.h>

namespace Arc {

  class RunPump;

  /// This class runs an external executable.
  /** It is possible to read from or write to its standard handles or to
   * redirect them to std::string elements.
   * \ingroup common
   * \headerfile Run.h arc/Run.h */
  class Run {
    friend class RunPump;
  public:
    // Interface to container for data received/sent to process.
    class Data {
     public:
      virtual ~Data() {};
      virtual void Append(char const* data, unsigned int size) = 0;
      virtual void Remove(unsigned int size) = 0;
      virtual char const* Get() const = 0;
      virtual unsigned int Size() const = 0;
    };

  private:
    Run(const Run&);
    Run& operator=(Run&);
  protected:
    // Container for data pumped through std* channels.
    class StringData: public Data {
     public:
      StringData();
      virtual ~StringData();
      void Assign(std::string& str, int content_max_size = 0);
      virtual void Append(char const* data, unsigned int size);
      virtual void Remove(unsigned int size);
      virtual char const* Get() const;
      virtual unsigned int Size() const;
     private:
      std::string* content_;
      int content_max_size_;
    };

    // working directory
    std::string working_directory;
    // Std* handles
    int stdout_;
    int stderr_;
    int stdin_;
    // Associated string containers
    Data *stdout_str_;
    Data *stderr_str_;
    Data *stdin_str_;
    StringData stdout_str_wrap_;
    StringData stderr_str_wrap_;
    StringData stdin_str_wrap_;
    // Request to keep std* handles of process same as in parent
    bool stdout_keep_;
    bool stderr_keep_;
    bool stdin_keep_;
    // PID of child
    pid_t pid_;
    // Arguments to execute
    std::list<std::string> argv_;
    std::list<std::string> envp_;
    std::list<std::string> envx_;
    void (*initializer_func_)(void*);
    void *initializer_arg_;
    bool initializer_is_complex_;
    void (*kicker_func_)(void*);
    void *kicker_arg_;
    // IO handlers are called when data can be sent/received
    typedef bool (Run::*HandleHandler)();
    bool stdout_handler();
    bool stderr_handler();
    bool stdin_handler();
    // Child exit handler
    typedef bool (Run::*PidHandler)(int);
    void child_handler(int result);
    bool started_; // child process was stated
    bool running_; // child process is running
    bool abandoned_; // we are interested in this process anymore
    int result_; // process execution result
    Glib::Mutex lock_;
    Glib::Cond cond_;
    int user_id_;
    int group_id_;
    Time run_time_;
    Time exit_time_;
  public:
    /// Constructor prepares object to run cmdline.
    Run(const std::string& cmdline);
    /// Constructor prepares object to run executable and arguments specified in argv.
    Run(const std::list<std::string>& argv);
    /// Destructor kills running executable and releases associated resources.
    ~Run(void);
    /// Returns true if object is valid.
    operator bool(void) {
      return argv_.size() != 0;
    }
    /// Returns true if object is invalid.
    bool operator!(void) {
      return argv_.size() == 0;
    }
    /// Starts running executable. This method may be called only once.
    /** \return true if executable started without problems */
    bool Start(void);
    /// Wait till execution finished or till timeout seconds expires.
    /** \return true if execution is complete. */
    bool Wait(int timeout);
    /// Wait till execution finished.
    /** \return true if execution is complete, false if execution was not
        started. */
    bool Wait(void);
    /// Returns exit code of execution.
    /** If child process was killed then exit code is -1.
        If code is compiled with support for detecting lost child
        process this code is -1 also if track of child was lost. */
    int Result(void) {
      return result_;
    }
    /// Return true if execution is going on.
    bool Running(void);
    /// Returns time when executable was started.
    Time RunTime(void) {
      return run_time_;
    };
    /// Return time when executable finished executing.
    Time ExitTime(void) {
      return exit_time_;
    };
    /// Read from stdout pipe of running executable.
    /** This method may be used while stdout is directed to string, but the
        result is unpredictable.
        \param timeout upper limit for which method will block in milliseconds.
        Negative means infinite.
        \param buf buffer to write the stdout to
        \param size size of buf
        \return number of read bytes. */
    int ReadStdout(int timeout, char *buf, int size);
    /// Read from stderr pipe of running executable.
    /** This method may be used while stderr is directed to string, but the
        result is unpredictable.
        \param timeout upper limit for which method will block in milliseconds.
        Negative means infinite.
        \param buf buffer to write the stderr to
        \param size size of buf
        \return number of read bytes. */
    int ReadStderr(int timeout, char *buf, int size);
    /// Write to stdin pipe of running executable.
    /** This method may be used while stdin is directed to string, but the
        result is unpredictable.
        \param timeout upper limit for which method will block in milliseconds.
        Negative means infinite.
        \param buf buffer to read the stdin from
        \param size size of buf
        \return number of written bytes. */
    int WriteStdin(int timeout, const char *buf, int size);
    /// Associate stdout pipe of executable with string.
    /** This method must be called before Start(). str object
        must be valid as long as this object exists. */
    void AssignStdout(std::string& str, int max_size = 102400);
    void AssignStdout(Data& str);
    /// Associate stderr pipe of executable with string.
    /** This method must be called before Start(). str object
        must be valid as long as this object exists. */
    void AssignStderr(std::string& str, int max_size = 102400);
    void AssignStderr(Data& str);
    /// Associate stdin pipe of executable with string.
    /** This method must be called before Start(). str object
        must be valid as long as this object exists. */
    void AssignStdin(std::string& str);
    void AssignStdin(Data& str);
    /// Keep stdout same as parent's if keep = true. No pipe will be created.
    void KeepStdout(bool keep = true);
    /// Keep stderr same as parent's if keep = true. No pipe will be created.
    void KeepStderr(bool keep = true);
    /// Keep stdin same as parent's if keep = true. No pipe will be created.
    void KeepStdin(bool keep = true);
    /// Closes pipe associated with stdout handle.
    void CloseStdout(void);
    /// Closes pipe associated with stderr handle.
    void CloseStderr(void);
    /// Closes pipe associated with stdin handle.
    void CloseStdin(void);
    /// Assign a function to be called just after process is forked but before execution starts.
    void AssignInitializer(void (*initializer_func)(void*), void *initializer_arg, bool is_complex);
    /// Assign a function to be called just after execution ends. It is executed asynchronously.
    void AssignKicker(void (*kicker_func)(void*), void *kicker_arg);
    /// Assign working directory of the process to run.
    void AssignWorkingDirectory(std::string& wd) {
      working_directory = wd;
    }
    /// Assign uid for the process to run under.
    void AssignUserId(int uid) {
      user_id_ = uid;
    }
    /// Assign gid for the process to run under.
    void AssignGroupId(int gid) {
      group_id_ = gid;
    }
    /// Add environment variable to be passed to process before it is run
    void AddEnvironment(const std::string& key, const std::string& value) {
      AddEnvironment(key+"="+value);
    }
    /// Add environment variable to be passed to process before it is run
    void AddEnvironment(const std::string& var) {
      envp_.push_back(var);
    }
    /// Remove environment variable to be passed to process before it is run
    void RemoveEnvironment(const std::string& key) {
      envx_.push_back(key);
    }
    /// Kill running executable.
    /** First soft kill signal (SIGTERM) is sent to executable. If after
        timeout seconds executable is still running it's killed forcefuly. */
    void Kill(int timeout);
    /// Detach this object from running process.
    /** After calling this method class instance is not associated with external
        process anymore. As result destructor will not kill process. */
    void Abandon(void);
    /// Call this method after fork() in child process.
    /** It will reinitialize internal structures for new environment. Do
        not call it in any other case than defined. */
    static void AfterFork(void);
  };

}

#endif // __ARC_RUN_H__
