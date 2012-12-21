/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

/* For the Java bindings the approach of one single SWIG generated C++ file is
 * used, while for Python one SWIG C++ file is generated for each mapped ARC
 * library. Therefore for Python each of the specialised SWIG files (.i) is
 * passed to SWIG, while for Java only this file is passed.
 */

#ifdef SWIGJAVA
%module(directors="1") arc 
#endif

%include <stl.i>
%include <std_vector.i>

#define DEPRECATED(X) X

#ifdef SWIGPYTHON
%include <std_list.i>
%include <std_set.i>
#ifdef PYDOXYGEN
%include "../python/pydoxygen.i"
#endif

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
%include <std_common.i>
%include <std_string.i>

%{
#include <list>
#include <set>
#include <stdexcept>
#include <iterator>
%}

%pragma(java) jniclasscode=%{
  static {
    try {
        System.loadLibrary("jarc");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Unable to load native code library (jarc), which provides Java interface to the ARC C++ libraries. \n" + e);
      System.exit(1);
    }
  }
%}

/* Swig does not offer any bindings of the std::list template class, so
 * below a implementation is done which offers basic looping and element
 * access support, i.e. through the std::list and listiteratorhandler
 * classes.
 */

template <typename T>
class listiterator {
  private:
    typename std::list<T>::iterator it;
    const std::list<T>& origList;
  public:
    listiterator(typename std::list<T>::iterator it, const std::list<T>& origList);
    T next();
    bool hasNext();
};

template <typename T>
class setiterator {
  private:
    typename std::set<T>::iterator it;
  public:
    setiterator(typename std::set<T>::iterator it);
    T pointer();
    void next();
    bool equal(const setiterator<T>& ita);
};

namespace std {
  template<class T> class list {
  public:
    typedef size_t size_type;
    typedef T value_type;
    typedef const value_type& const_reference;
    list();
    list(size_type n);
    %extend {
      int size() const { return (int)self->size(); }
    }
    %rename(isEmpty) empty;
    bool empty() const;
    void clear();
    %rename(add) push_back;
    void push_back(const value_type& x);
    %extend {
      listiterator<T> iterator() { return listiterator<T>(self->begin(), *self); }
      listiterator<T> begin() { return listiterator<T>(self->begin(), *self); }
      listiterator<T> end() { return listiterator<T>(self->end(), *self); }
    }
  };

  template<class T> class set {
  public:
    typedef size_t size_type;
    typedef T key_type;
    typedef T value_type;
    typedef const value_type& const_reference;
    set();
    %extend {
      int size() const { return (int)self->size(); }
    }
    %rename(isEmpty) empty;
    bool empty() const;
    void clear();
    void insert(const value_type& x);
    %extend {
      int count(const key_type& k) const { return (int)self->count(k); }
      setiterator<T> begin() { return setiterator<T>(self->begin()); }
      setiterator<T> end() { return setiterator<T>(self->end()); }
    }
  };
}

%{
#include <stdexcept>
template <typename T>
class listiterator {
private:
  typename std::list<T>::iterator it;
  const std::list<T>& origList;
public:
  listiterator(typename std::list<T>::iterator it, const std::list<T>& origList) : it(it), origList(origList) {}

  T next() throw (std::out_of_range) { if (!hasNext()) { throw std::out_of_range(""); }; return (it++).operator*(); };
  bool hasNext() { return it != origList.end(); };
};

template <typename T>
class setiterator {
private:
  typename std::set<T>::iterator it;
public:
  setiterator(typename std::set<T>::iterator it) : it(it) {}

  T pointer() { return it.operator*(); };
  void next() { it.operator++(); };
  bool equal(const setiterator<T>& ita) { return it.operator==(ita.it); };
};
%}
#ifdef JAVA_IS_15_OR_ABOVE
%typemap(javaimports) listiterator "import java.util.Iterator; import java.util.NoSuchElementException;"
#else
%typemap(javaimports) listiterator "import java.util.NoSuchElementException;"
#endif
%typemap(javacode) listiterator %{
  public void remove() throws UnsupportedOperationException { throw new UnsupportedOperationException(); }
%}
%javaexception("java.util.NoSuchElementException") listiterator::next {
  try {
    $action
  } catch (std::out_of_range &e) {
    jclass clazz = jenv->FindClass("java/util/NoSuchElementException");
    jenv->ThrowNew(clazz, "Range error");
    return $null;
  }
}


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

/* With Swig 1.3.29 the memory management introduced above is applied to the get
 * method in the wrapped std::map template classes where the value parameter is
 * a "basic" type (int, double, string, etc.), however the management is not
 * applied to the 'SWIGTYPE_p_<basice-type>' wrapped class. So don't memory
 * management to these std::map classes.
 */
%typemap(javaout) double& get      { return new $javaclassname($jnicall, $owner); }
%typemap(javaout) int& get         { return new $javaclassname($jnicall, $owner); }
%typemap(javaout) std::string& get { return new $javaclassname($jnicall, $owner); }


%rename(toBool) operator bool;
%rename(toString) operator std::string;
%rename(equals) operator==;

%ignore *::operator!=;
%ignore *::operator<;
%ignore *::operator>;
%ignore *::operator<=;
%ignore *::operator>=;

/* The std::cout object will always exist, so do not set any references. See
 * comments in Arc.i.
 */
%typemap(javaout) std::ostream& getStdout { return new $javaclassname($jnicall, $owner); }
/* Provide method of getting std::cout as a std::ostream object, usable
 * with the LogDestination class.
 */
%inline %{
#include <iostream>
std::ostream& getStdout() { return std::cout; }
%}
#endif

#ifndef SWIGJAVA
%define %wraplist(X, Y...)
%template(X ## List) std::list< Y >;
%enddef
#else
#ifdef JAVA_IS_15_OR_ABOVE
%define %wraplist(X, Y...)
%typemap(javainterfaces) listiterator< Y > %{Iterator< X >%}
%typemap(javainterfaces) std::list< Y > %{Iterable< X >%}
%template(X ## List) std::list< Y >;
%template(X ## ListIterator) listiterator< Y >;
%enddef
#else
%define %wraplist(X, Y...)
%template(X ## List) std::list< Y >;
%template(X ## ListIterator) listiterator< Y >;
%enddef
#endif
#endif

%template(StringPair) std::pair<std::string, std::string>;
%wraplist(String, std::string)
%template(StringSet) std::set<std::string>;
#ifdef SWIGJAVA
%template(StringSetIterator) setiterator<std::string>;
#endif
%template(StringVector) std::vector<std::string>;
%template(StringStringMap) std::map<std::string, std::string>;
%template(StringDoubleMap) std::map<std::string, double>;


#ifdef SWIGJAVA
%include "common.i"
%include "loader.i"
%include "message.i"
%include "communication.i"
%include "compute.i"
%include "credential.i"
%include "data.i"
%include "delegation.i"
#endif
