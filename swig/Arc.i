%module arc

%include <stl.i>

#ifdef SWIGPYTHON
%include <std_list.i>
%template(StringList) std::list<std::string>;
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

%template(StringList) std::list<std::string>;
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
