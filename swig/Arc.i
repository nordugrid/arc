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
%include "pydoxygen.i"
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
    } catch (UnsatisfiedLinkError e1) {
      try {
        System.load("/usr/lib64/arc/libjarc.so");
      } catch (UnsatisfiedLinkError e2) {
        try {
          System.load("/usr/lib/arc/libjarc.so");
        } catch (UnsatisfiedLinkError e3) {
          System.err.println("Unable to load native code library (jarc), which provides Java interface to the ARC C++ libraries.");
          System.exit(1);
        }
      }
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
  // Copied verbatim from '%typemape(javacode) SWIGTYPE'.
  private Object objectManagingMyMemory;
  protected void setMemoryManager(Object r) {
    objectManagingMyMemory = r;
  } // %typemap(javacode) SWIGTYPE - End

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
 * internal object, a workaround is needed for Java. The reason for this is
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
 * Since typemaps overrides other typemaps which are less specific in the
 * matching and since the $typemap macro is not working in older versions of
 * swig (e.g. 1.3.29) the code in the below typemap is copied verbatim in this
 * and other swig interface files (.i). Look for '%typemap(javacode) SWIGTYPE'
 * comment.
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


/* Swig doesn't provide a direct way to wrap an abstract class with no concrete
 * members to a Java interface. Instead the following workaround is made in
 * order to make the EntityConsumer interface work correctly in the Java
 * bindings.
 * The '%prewrapentityconsumerinterface' macro should be invoked
 * before the C++ class definition is included (%include).
 * Some of the code is taken from: <http://stackoverflow.com/questions/8168517/generating-java-interface-with-swig>
 */
%define %prewrapentityconsumerinterface(X, Y...)
%feature("director") Arc::EntityConsumer< Y >;
%rename(Native ## X ## Consumer) Arc::EntityConsumer< Y >;
%typemap(jstype) const Arc::EntityConsumer< Y >&   %{X ## Consumer%};
%typemap(jstype)       Arc::EntityConsumer< Y >&   %{X ## Consumer%};
%typemap(javainterfaces) Arc::EntityConsumer< Y >  %{X ## Consumer%};

#if SWIG_VERSION > 0x010331
/* Make sure every Java consumer interface instance is converted to an instance
 * which inherits from the concrete native consumer class which wraps the C++
 * consumer abstract class.
 */
%typemap(javain, pgcppname="n", pre= "    $javaclassname n = $module.makeNative($javainput);") const Arc::EntityConsumer< Y >&  %{ Native ## X ## Consumer.getCPtr(n) %};
%typemap(javain, pgcppname="n", pre= "    $javaclassname n = $module.makeNative($javainput);")       Arc::EntityConsumer< Y >&  %{ Native ## X ## Consumer.getCPtr(n) %};

/* For C++ classes holding an instance of the consumer object, the java wrapped
 * class also need to hold a reference to the native consumer. Otherwise it will
 * go out of scope, while being used. Note that a Map (HashMap) must be added
 * to the particular Java class, corresponding the C++ in question. This must be
 * done at before '%include'ing the corresponding C++ header. E.g.:
   %typemap(javacode) Arc::Example %{
     private java.util.HashMap<ExampleConsumer, NativeExampleConsumer> consumers = new java.util.HashMap<ExampleConsumer, NativeExampleConsumer>();
   %}
 * Overrides the 'javain' typemaps above since those below are more specific due
 * to the use of the 'addConsumer_consumer' and 'removeConsumer_consumer'
 * matching.
 */
%typemap(javain, pgcppname="n", pre= "    $javaclassname n = $module.makeNative($javainput); consumers.put($javainput, n);") Arc::EntityConsumer< Y >&  addConsumer_consumer %{Native ## X ## Consumer.getCPtr(n)%};
%typemap(javain, pgcppname="n", pre= "    if (!consumers.containsKey($javainput)) return; $javaclassname n = ($javaclassname)consumers.get($javainput);", post = "      consumers.remove($javainput);")       Arc::EntityConsumer< Y >&  removeConsumer_consumer %{Native ## X ## Consumer.getCPtr(n)%};
%typemap(javain, pgcppname="n", pre= "    if (!consumers.containsKey($javainput)) return; $javaclassname n = ($javaclassname)consumers.get($javainput);", post = "      consumers.remove($javainput);") const Arc::EntityConsumer< Y >&  removeConsumer_consumer %{Native ## X ## Consumer.getCPtr(n)%};

#else
/* Workaround for older swig versions. The pgcppname, pre and post attributes
 * to the javain typemap doesn't work in swig 1.3.31 and lower. Additional
 * workarounds is found in the other swig interface files (e.g. compute.i).
 */
%typemap(javain)  const Arc::EntityConsumer< Y >&  %{Native ## X ## Consumer.getCPtr(n)%};
%typemap(javain)        Arc::EntityConsumer< Y >&  %{Native ## X ## Consumer.getCPtr(n)%};
#endif
 
%pragma(java) modulecode=%{
  private static class Native ## X ## ConsumerProxy extends Native ## X ## Consumer {
    private X ## Consumer delegate;
    public Native ## X ## ConsumerProxy(X ## Consumer i) {
      delegate = i;
    }
    
    public Native ## X ## ConsumerProxy(X ## Consumer i, long cPtr) {
      super(cPtr, false);
      delegate = i;
    }

    public void addEntity(X e) {
      delegate.addEntity(e);
    }
  }

  // No access modifier. Classes in this package need to access the method.
  static Native ## X ## Consumer makeNative(X ## Consumer i) {
    if (i instanceof Native ## X ## Consumer) {
      // If it already *is* a Native ## X ## Consumer don't bother wrapping it again
      return (Native ## X ## Consumer)i;
    }
    /* Not all Swig wrapped classes which inherits (C++) from EntityConsumer is
     * a native consumer. Check if the instance have a 'getCPtr' method and if
     * so instantiate native consumer proxy with the value returned by
     * 'getCPtr'.
     */
    try {
      Class[] types = new Class[1];
      types[0] = i.getClass();
      Object[] params = new Object[1];
      params[0] = i;
      return new Native ## X ## ConsumerProxy(i, ((Long)i.getClass().getMethod("getCPtr", types).invoke(null, params)).longValue());
    } catch (Exception e) {
      return new Native ## X ## ConsumerProxy(i);
    }
  }
%}
%enddef

#endif


#ifdef SWIGPYTHON
// Make typemap for time_t - otherwise it will be a useless proxy object.
// Solution taken from: http://stackoverflow.com/questions/2816046/simple-typemap-example-in-swig-java
%typemap(in) time_t
{
  if      (PyLong_Check($input))  $1 = (time_t) PyLong_AsLong($input);
  else if (PyInt_Check($input))   $1 = (time_t) PyInt_AsLong($input);
  else if (PyFloat_Check($input)) $1 = (time_t) PyFloat_AsDouble($input);
  else {
    PyErr_SetString(PyExc_TypeError,"Expected a large type");
    return NULL;
  }
}
%typemap(typecheck) time_t = long;

%typemap(out) time_t
{
  $result = PyLong_FromLong((long)$1);
}


%typemap(in) uint32_t
{
  if      (PyInt_Check($input))   $1 = (uint32_t) PyInt_AsLong($input);
  else if (PyFloat_Check($input)) $1 = (uint32_t) PyFloat_AsDouble($input);
  else {
    PyErr_SetString(PyExc_TypeError,"Unable to convert type to 32bit number (int/float)");
    return NULL;
  }
}
%typemap(typecheck) uint32_t = int;

// It seems there is no need for an out typemap for uint32_t...
// Methods returning an uint32_t type will in python return a long.
#endif
#ifdef SWIGJAVA
// Make typemap for time_t - otherwise it will be a useless proxy object.
// Solution taken from: http://stackoverflow.com/questions/2816046/simple-typemap-example-in-swig-java
%typemap(in) time_t {
  $1 = (time_t)$input;
}
%typemap(javain) time_t "$javainput"

%typemap(out) time_t %{
  $result = (jlong)$1;
%}
%typemap(javaout) time_t {
  return $jnicall;
}

%typemap(jni)    time_t "jlong"
%typemap(jtype)  time_t "long"
%typemap(jstype) time_t "long"


%typemap(in) uint32_t {
  $1 = (uint32_t)$input;
}
%typemap(javain) uint32_t "$javainput"

%typemap(out) uint32_t %{
  $result = (jint)$1;
%}
%typemap(javaout) uint32_t {
  return $jnicall;
}

%typemap(jni)    uint32_t "jint"
%typemap(jtype)  uint32_t "int"
%typemap(jstype) uint32_t "int"
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

/* TODO: Python: Avoid creating a new SWIG types for each module, for types that
 * are general for different modules. E.g. StringPair - put it in common, and
 * use the one from common in the other modules. In python:
 * arc.common.StringPair == arc.compute.StringPair => False
 */

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
