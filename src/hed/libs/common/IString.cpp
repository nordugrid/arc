#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include "IString.h"

namespace Arc {

  PrintFBase::PrintFBase() : refcount(1) {}

  PrintFBase::~PrintFBase() {}

  void PrintFBase::Retain() {
    refcount++;
  }

  bool PrintFBase::Release() {
    refcount--;
    return (refcount == 0);
  }

  const char* FindTrans(const char* p) {
#ifdef HAVE_LIBINTL_H
    return dgettext("Arc", p);
#else
    return p;
#endif
  }

  IString::IString(const IString& istr)
    : p(istr.p) {
    p->Retain();
  }

  IString::~IString() {
    if (p->Release())
      delete p;
  }

  IString& IString::operator=(const IString& istr) {
    if (p->Release())
      delete p;    
    p = istr.p;
    p->Retain();
    return *this;
  }

  std::ostream& operator<<(std::ostream& os, const IString& msg) {
    msg.p->msg(os);
    return os;
  }

} // namespace Arc
