// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_UTILS_H__
#define __ARC_UTILS_H__

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <string>

namespace Arc {

  /// Portable function for getting environment variables
  std::string GetEnv(const std::string& var);

  /// Portable function for setting environment variables
  bool SetEnv(const std::string& var, const std::string& value);

  /// Portable function for unsetting environment variables
  void UnsetEnv(const std::string& var);

  /// Portable function for obtaining description of last system error
  std::string StrError(int errnum = errno);

  /// Wrapper for pointer with automatic destruction
  /** If ordinary pointer is wrapped in instance of this class
     it will be automatically destroyed when instance is destroyed.
     This is useful for maintaing pointers in scope of one
     function. Only pointers returned by new() are supported. */
  template<typename T>
  class AutoPointer {
  private:
    T *object;
    void operator=(const AutoPointer<T>&) {}
    void operator=(T*) {}
    AutoPointer(const AutoPointer&)
      : object(NULL) {}
  public:
    /// NULL pointer constructor
    AutoPointer(void)
      : object(NULL) {}
    /// Constructor which wraps pointer
    AutoPointer(T *o)
      : object(o) {}
    /// Destructor destroys wrapped object using delete()
    ~AutoPointer(void) {
      if (object)
        delete object;
    }
    /// For refering wrapped object
    T& operator*(void) const {
      return *object;
    }
    /// For refering wrapped object
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
    operator T*(void) const {
      return object;
    }
  };

  /// Wrapper for pointer with automatic destruction and mutiple references
  /** If ordinary pointer is wrapped in instance of this class
     it will be automatically destroyed when all instances refering to it
     are destroyed.
     This is useful for maintaing pointers refered from multiple structures
     wihth automatic destruction of original object when last reference
     is destroyed. It is similar to Java approach with a difference that
     desctruction time is strictly defined.
     Only pointers returned by new() are supported. This class is not thread-safe */
  template<typename T>
  class CountedPointer {
  private:
    template<typename P>
    class Base {
    private:
      Base(Base<P>&) {}
    public:
      int cnt;
      P *ptr;
      Base(P *p)
        : cnt(0),
          ptr(p) {
        add();
      }
      ~Base(void) {
        if (ptr)
          delete ptr;
      }
      Base<P> add(void) {
        ++cnt;
        return this;
      }
      bool rem(void) {
        if (--cnt == 0) {
          delete this;
          return true;
        }
        return false;
      }
    };
    Base<T> *object;
  public:
    CountedPointer(T *p)
      : object(new Base<T>(p)) {}
    CountedPointer(CountedPointer<T>& p)
      : object(p.object->add()) {}
    ~CountedPointer(void) {
      object->rem();
    }
    CountedPointer& operator=(T *p) {
      if (p != object->ptr) {
        object->rem();
        object = new Base<T>(p);
      }
      return *this;
    }
    CountedPointer& operator=(CountedPointer& p) {
      if (p.object->ptr != object->ptr) {
        object->rem();
        object = p.object->add();
      }
      return *this;
    }
    /// For refering wrapped object
    T& operator*(void) const {
      return *(object->ptr);
    }
    /// For refering wrapped object
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
    /// Cast to original pointer
    operator T*(void) const {
      return (object->ptr);
    }
  };


} // namespace Arc

# endif // __ARC_UTILS_H__
