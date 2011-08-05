/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%module arc

%include <stl.i>

#ifdef SWIGPYTHON
%include <std_list.i>
#ifdef PYDOXYGEN
%include "../python/pydoxygen.i"
#endif
#endif

#ifdef SWIGJAVA
%include <std_common.i>
%include <std_string.i>

%{
#include <list>
#include <stdexcept>
#include <iterator>
%}

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

/* On CentOS the following is needed in order for java bindings to be compiled
 * successfully.
 */
%typemap(javaout) std::string& get {
  return new $javaclassname($jnicall, $owner);
}
#endif

%template(StringPair) std::pair<std::string, std::string>;
%template(StringList) std::list<std::string>;
%template(StringStringMap) std::map<std::string, std::string>;

#ifdef SWIGPYTHON
namespace Arc {

/**
 * Python cannot deal with string references since strings in Python
 * are immutable. Therefore ignore the string reference argument and
 * store a reference in a temporary variable. Here it is assumed that
 * the string reference does not contain any input to the function being
 * called.
 **/
%typemap(in, numinputs=0) std::string& content (std::string str) {
  $1 = &str;
}

/**
 * Return the original return value and the temporary string reference
 * combined in a Python tuple.
 **/
%typemap(argout) std::string& content {
  PyObject *tuple;
  tuple = PyTuple_New(2);
  PyTuple_SetItem(tuple,0,$result);
  PyTuple_SetItem(tuple,1,Py_BuildValue("s",$1->c_str()));
  $result = tuple;
}


// Convert output 'std::list<std::string>*' to a Python list containing Python strings.
%typemap(out) std::list<std::string>* {
  $result = PyList_New(0);
  for (std::list<std::string>::iterator it = $1->begin();
       it != $1->end(); ++it) {
#if PY_MAJOR_VERSION >= 3
    PyList_Append($result, PyUnicode_FromString(it->c_str()));
#else
    PyList_Append($result, PyString_FromString(it->c_str()));
#endif
  }
}
}
#endif

#ifdef SWIGJAVA
%template(StringListIteratorHandler) listiteratorhandler<std::string>;
#endif


%include "common.i"
%include "message.i"
%include "client.i"
%include "credential.i"
%include "data.i"
%include "delegation.i"
#ifdef SWIGPYTHON
%include "security.i"
#endif
