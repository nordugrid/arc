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
#endif

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
