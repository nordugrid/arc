/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%module arc

%include <stl.i>

#ifdef SWIGPYTHON
%include <std_list.i>
%include <std_set.i>
#ifdef PYDOXYGEN
%include "../python/pydoxygen.i"
#endif
#endif

#define DEPRECATED(X) X

#ifdef SWIGJAVA
%include <std_common.i>
%include <std_string.i>

%{
#include <list>
#include <stdexcept>
#include <iterator>
%}

/* Swig does not offer any bindings of the std::list template class, so
 * below a implementation is done which offers basic looping and element
 * access support, i.e. through the std::list and listiteratorhandler
 * classes.
 */

namespace std {
    template<class T> class list {
      public:
        typedef size_t size_type;
        typedef T value_type;
        typedef const value_type& const_reference;
        list();
        list(size_type n);
        size_type size() const;
        %rename(isEmpty) empty;
        bool empty() const;
        void clear();
        %rename(add) push_back;
        void push_back(const value_type& x);
        std::list<value_type>::iterator begin();
        std::list<value_type>::iterator end();
    };
}


%define specialize_std_list(T)
#warning "specialize_std_list - specialization for type T no longer needed"
%enddef

template <class T>
class listiteratorhandler
{
  private:
    typename std::list<T>::iterator it;
  public:
    listiteratorhandler(typename std::list<T>::iterator it);
    T pointer();
    void next();
    bool equal(typename std::list<T>::iterator ita);
};
%{
template <class T>
class listiteratorhandler
{
  private:
    typename std::list<T>::iterator it;
  public:
    listiteratorhandler(typename std::list<T>::iterator it) : it(it) {}

    T pointer() { return it.operator*(); };
    void next() { it.operator++(); };
    bool equal(typename std::list<T>::iterator ita) { return it.operator==(ita); };
};

%}


/* For non-static methods in the ARC library which returns a reference to an
 * internal object, a work around is needed for Java. The reason for this is
 * the following:
 * // In C++ let A::get() return a reference to an internal (private) object.
 * const R& A::get() const; // C++
 * // In Java the A::get() method is wrapped to call the C++ A::get().
 * A a = new A(); // Java
 * R r = a->get(); // Java
 * // The memory of object r is managed by object a. When a is garbage
 * // collected, which means in C++ terms that the a object is deleted, the r
 * // object becomes invalid since a no longer exist.
 * // In C++ a will exist through out the scope which a is defined in, but in
 * // Java the object a might garbage collected when it is not in use any more,
 * // which means that it might be garbage collected before a goes out of scope.
 * ...
 * // Do some something not involving the a object.
 * ...
 * // Now still in scope a might have been garbage collected and thus the
 * // following statement might cause a segmentation fault.
 * System.out.println("r = " + r.toString());
 *
 * Therefore when returning a C++ reference, the Java object holding the
 * C++ reference must have a Java reference to the Java object which returned
 * the C++ reference. In that way the garbage collector will not garbage collect
 * the Java object actually managing the memory of the referenced C++ object.
 *
 * See <http://swig.org/Doc1.3/Java.html#java_memory_management_member_variables>
 * for more info.
 */
/* Add member to any Java proxy class which is able to hold a reference to
 * an object managing its memory.
 * Add method which sets managing object.
 */
%typemap(javacode) SWIGTYPE %{
  private Object objectManagingMyMemory;
  protected void setMemoryManager(Object r) {
    objectManagingMyMemory = r;
  }
%}
/* Make sure that when a C++ reference is returned that the corresponding Java
 * proxy class will call the setMemoryManager method to indicate that the memory
 * is maintained by this object.
 */
%typemap(javaout) SWIGTYPE &  {
  long cPtr = $jnicall;
  $javaclassname ret = null;
  if (cPtr != 0) {
    ret = new $javaclassname(cPtr, $owner);
    ret.setMemoryManager(this);
  }
  return ret;
}

// Do not apply memory management to the get method of the wrapped std::map<std::string, double> class.
%typemap(javaout) double& get {
  return new $javaclassname($jnicall, $owner);
}

/* On CentOS the following is needed in order for java bindings to be compiled
 * successfully.
 */
%typemap(javaout) std::string& get {
  return new $javaclassname($jnicall, $owner);
}
#endif

%template(StringPair) std::pair<std::string, std::string>;
%template(StringList) std::list<std::string>;
%template(StringVector) std::vector<std::string>;
%template(StringStringMap) std::map<std::string, std::string>;
%template(StringDoubleMap) std::map<std::string, double>;


#ifdef SWIGPYTHON
namespace Arc {

/**
 * Python cannot deal with string references since strings in Python
 * are immutable. Therefore ignore the string reference argument and
 * store a reference in a temporary variable. Here it is assumed that
 * the string reference does not contain any input to the function being
 * called.
 **/
%typemap(in, numinputs=0) std::string& TUPLEOUTPUTSTRING (std::string str) {
  $1 = &str;
}

/**
 * Return the original return value and the temporary string reference
 * combined in a Python tuple.
 **/
%typemap(argout) std::string& TUPLEOUTPUTSTRING {
  $result = PyTuple_Pack(2, $result, SWIG_From_std_string(*$1));
}
}

%pythoncode %{
import warnings

def deprecated(method):
    """This decorator is used to mark python methods as deprecated, _not_
    functions. It will result in a warning being emmitted when the method
    is used."""
    def newMethod(*args, **kwargs):
        warnings.warn("Call to deprecated method 'arc.%s.%s'." % (args[0].__class__.__name__, method.__name__), category = DeprecationWarning, stacklevel = 2)
        return method(*args, **kwargs)
    newMethod.__name__ = method.__name__
    newMethod.__doc__ = method.__doc__
    newMethod.__dict__.update(method.__dict__)
    return newMethod
%}

%rename(__nonzero__) operator bool;
%rename(__str__) operator std::string;

%pythoncode %{
class StaticPropertyWrapper(object):
    def __init__(self, wrapped_class):
        object.__setattr__(self, "wrapped_class", wrapped_class)

    def __getattr__(self, name):
        orig_attr = getattr(self.wrapped_class, name)
        if isinstance(orig_attr, property):
            return orig_attr.fget()
        else:
            return orig_attr

    def __setattr__(self, name, value):
        orig_attr = getattr(self.wrapped_class, name)
        if isinstance(orig_attr, property):
            orig_attr.fset(value)
        else:
            setattr(self.wrapped_class, name, value)


%}
#endif

#ifdef SWIGJAVA
%template(StringListIteratorHandler) listiteratorhandler<std::string>;

%rename(toBool) operator bool;
%rename(toString) operator std::string;

/* The std::cout object will always exist, so do not set any references. See
 * comments in Arc.i.
 */
%typemap(javaout) std::ostream& getStdout { return new $javaclassname($jnicall, $owner); }
/* Provide method of getting std::cout as a std::ostream object, usable
 * with the LogDestination class.
 */
%inline %{
std::ostream& getStdout() { return std::cout; }
%}
#endif


%include "common.i"
%include "loader.i"
%include "message.i"
%include "client.i"
%include "credential.i"
%include "data.i"
%include "delegation.i"
#ifdef SWIGPYTHON
%include "security.i"
#endif
