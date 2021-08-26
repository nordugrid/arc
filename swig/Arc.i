/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

/* Python bindings one SWIG C++ file is generated for each mapped ARC
 * library, so each of the specialised SWIG files (.i) is passed to SWIG.
 */

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

%typemap(out) uint32_t
{
  $result = PyInt_FromLong((int)$1);
}
#endif

%define %wraplist(X, Y...)
%template(X ## List) std::list< Y >;
%enddef
