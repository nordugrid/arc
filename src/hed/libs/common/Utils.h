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
  void SetEnv(const std::string& var, const std::string& value);

  /// Portable function for obtaining description of last system error
  std::string StrError(int errnum = errno);

  /// Wrapper for pointer with automatic destruction
  /** If ordinary pointer is wrapped in instance of this class
    it will be automatically destroyed when instance is destroyed.
    This is useful for maintaing pointers in scope of one
    function. Only pointers returned by new() are supported. */
  template <typename T> class AutoPointer {
   private:
    T* object;
    void operator=(const AutoPointer<T>&) { };
    void operator=(T*) { };
    AutoPointer(const AutoPointer&):object(NULL) { };
   public:
    /// NULL pointer constructor
    AutoPointer(void):object(NULL) { };
    /// Constructor which wraps pointer
    AutoPointer(T* o):object(o) { }
    /// Destructor destroys wrapped object using delete()
    ~AutoPointer(void) { if(object) delete object; };
    /// For refering wrapped object
    T& operator*(void) const { return *object; };
    /// For refering wrapped object
    T* operator->(void) const { return object; };
    /// Returns false if pointer is NULL and true otherwise.
    operator bool(void) const { return (object!=NULL); };
    /// Returns true if pointer is NULL and false otherwise.
    bool operator!(void) const { return (object==NULL); };
    /// Cast to original pointer
    operator T*(void) const { return object; };
  };

} // namespace Arc

# endif // __ARC_UTILS_H__
