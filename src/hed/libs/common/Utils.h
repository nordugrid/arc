// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_UTILS_H__
#define __ARC_UTILS_H__

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <string>
#include <list>

namespace Arc {

  /** \addtogroup common
   *  @{ */

  /// Portable function for getting environment variables. Protected by shared lock.
  std::string GetEnv(const std::string& var);

  /// Portable function for getting environment variables. Protected by shared lock.
  std::string GetEnv(const std::string& var, bool &found);

  /// Portable function for getting all environment variables. Protected by shared lock.
  std::list<std::string> GetEnv();

  /// Portable function for setting environment variables. Protected by exclusive lock.
  bool SetEnv(const std::string& var, const std::string& value, bool overwrite = true);

  /// Portable function for unsetting environment variables. Protected by exclusive lock.
  void UnsetEnv(const std::string& var);

  // These are functions to be used used exclusively for solving
  // problem with specific libraries which depend too much on
  // environment variables.
  /// Obtain lock on environment.
  /** For use with external libraries using unprotected setenv/getenv in a
      multi-threaded environment. */
  void EnvLockAcquire(void);
  /// Release lock on environment.
  /** For use with external libraries using unprotected setenv/getenv in a
      multi-threaded environment. */
  void EnvLockRelease(void);
  /// Start code which is using setenv/getenv.
  /** For use with external libraries using unprotected setenv/getenv in a
      multi-threaded environment. Must always have corresponding EnvLockUnwrap.
   * \param all set to true for setenv and false for getenv. */
  void EnvLockWrap(bool all = false);
  /// End code which is using setenv/getenv.
  /** For use with external libraries using unprotected setenv/getenv in a
      multi-threaded environment.
      \param all must be same as in corresponding EnvLockWrap. */
  void EnvLockUnwrap(bool all = false);
  /// Use after fork() to reset all internal variables and release all locks.
  /** For use with external libraries using unprotected setenv/getenv in a
      multi-threaded environment.
      This function is deprecated. */
  void EnvLockUnwrapComplete(void);

  /// Class to provide automatic locking/unlocking of environment on creation/destruction.
  /** For use with external libraries using unprotected setenv/getenv in a
      multi-threaded environment.
      \headerfile Utils.h arc/Utils.h */
  class EnvLockWrapper {
   private:
    bool all_;
   public:
    /// Create a new environment lock for using setenv/getenv.
    /** \param all set to true for setenv and false for getenv. */
    EnvLockWrapper(bool all = false):all_(all) { EnvLockWrap(all_); };
    /// Release environment lock.
    ~EnvLockWrapper(void) { EnvLockUnwrap(all_); };
  };

  /// Marks off a section of code which should not be interrupted by signals.
  /** \headerfile Utils.h arc/Utils.h */
  class InterruptGuard {
  public:
    InterruptGuard();
    ~InterruptGuard();
  private:
    void (*saved_sigint_handler)(int);
  };

  /// Portable function for obtaining description of last system error
  std::string StrError(int errnum = errno);

  /// Wrapper for pointer with automatic destruction
  /** If ordinary pointer is wrapped in instance of this class
     it will be automatically destroyed when instance is destroyed.
     This is useful for maintaining pointers in scope of one
     function. Only pointers returned by new() are supported.
     \headerfile Utils.h arc/Utils.h */
  template<typename T>
  class AutoPointer {
  private:
    T *object;
    void (*deleter)(T*);
    static void DefaultDeleter(T* o) { delete o; }
    void operator=(const AutoPointer<T>&);
#if __cplusplus >= 201103L
  private:
    AutoPointer(AutoPointer<T> const&);
#else
  // Workaround for older gcc which does not implement construction
  // of new elements in std::list according to specification.
  public: 
    AutoPointer(AutoPointer<T> const& o) {
      operator=(const_cast<AutoPointer<T>&>(o));
    }
#endif
  public:
    /// NULL pointer constructor
    AutoPointer(void (*d)(T*) = &DefaultDeleter)
      : object(NULL), deleter(d) {}
    /// Constructor which wraps pointer and optionally defines deletion function
    AutoPointer(T *o, void (*d)(T*) = &DefaultDeleter)
      : object(o), deleter(d) {}
    /// Moving constructor
    AutoPointer(AutoPointer<T>& o)
      : object(o.Release()), deleter(o.deleter) {}
    /// Destructor destroys wrapped object using assigned deleter
    ~AutoPointer(void) {
      if (object) if(deleter) (*deleter)(object);
      object = NULL;
    }
    AutoPointer<T>& operator=(T* o) {
      if (object) if(deleter) (*deleter)(object);
      object = o; 
      return *this;
    }
    AutoPointer<T>& operator=(AutoPointer<T>& o) {
      if (object) if(deleter) (*deleter)(object);
      object = o.object; 
      o.object = NULL;
      return *this;
    }
    /// For referring wrapped object
    T& operator*(void) const {
      return *object;
    }
    /// For referring wrapped object
    T* operator->(void) const {
      return object;
    }
    /// Returns false if pointer is NULL and true otherwise.
    operator bool(void) const {
      return (object != NULL);
    }
    /// Returns true if pointer is NULL and false otherwise.
    bool operator!(void) const {
      return (object == NULL);
    }
    /// Cast to original pointer
    T* Ptr(void) const {
      return object;
    }
    /// Release referred object so that it can be passed to other container
    T* Release(void) {
      T* tmp = object;
      object = NULL;
      return tmp;
    }
  };

  /// Wrapper for pointer with automatic destruction and multiple references
  /** If ordinary pointer is wrapped in instance of this class
     it will be automatically destroyed when all instances referring to it
     are destroyed.
     This is useful for maintaining pointers referred from multiple structures
     with automatic destruction of original object when last reference
     is destroyed. It is similar to Java approach with a difference that
     destruction time is strictly defined.
     Only pointers returned by new() are supported. This class is not thread-safe.
     \headerfile Utils.h arc/Utils.h  */
  template<typename T>
  class CountedPointer {
  private:
    template<typename P>
    class Base {
    private:
      Base(Base<P>&);
    public:
      int cnt;
      P *ptr;
      bool released;
      Base(P *p)
        : cnt(0),
          ptr(p),
          released(false) {
        add();
      }
      ~Base(void) {
        if (ptr && !released)
          delete ptr;
      }
      Base<P>* add(void) {
        ++cnt;
        return this;
      }
      bool rem(void) {
        if (--cnt == 0) {
          if(!released) delete this;
          return true;
        }
        return false;
      }
    };
    Base<T> *object;
  public:
    CountedPointer(T *p = NULL)
      : object(new Base<T>(p)) {}
    CountedPointer(const CountedPointer<T>& p)
      : object(p.object->add()) {}
    ~CountedPointer(void) {
      object->rem();
    }
    CountedPointer<T>& operator=(T *p) {
      if (p != object->ptr) {
        object->rem();
        object = new Base<T>(p);
      }
      return *this;
    }
    CountedPointer<T>& operator=(const CountedPointer<T>& p) {
      if (p.object->ptr != object->ptr) {
        object->rem();
        object = p.object->add();
      }
      return *this;
    }
    /// For referring wrapped object
    T& operator*(void) const {
      return *(object->ptr);
    }
    /// For referring wrapped object
    T* operator->(void) const {
      return (object->ptr);
    }
    /// Returns false if pointer is NULL and true otherwise.
    operator bool(void) const {
      return ((object->ptr) != NULL);
    }
    /// Returns true if pointer is NULL and false otherwise.
    bool operator!(void) const {
      return ((object->ptr) == NULL);
    }
    /// Returns true if pointers are equal
    bool operator==(const CountedPointer& p) const {
      return ((object->ptr) == (p.object->ptr));
    }
    /// Returns true if pointers are not equal
    bool operator!=(const CountedPointer& p) const {
      return ((object->ptr) != (p.object->ptr));
    }
    /// Comparison operator
    bool operator<(const CountedPointer& p) const {
      return ((object->ptr) < (p.object->ptr));
    }
    /// Cast to original pointer
    T* Ptr(void) const {
      return (object->ptr);
    }
    /// Release referred object so that it can be passed to other container
    T* Release(void) {
      T* tmp = object->ptr;
      object->released = true;
      return tmp;
    }
  };

  /// Load library and keep persistent.
  bool PersistentLibraryInit(const std::string& name);

  /** @{ */

} // namespace Arc

#endif // __ARC_UTILS_H__
