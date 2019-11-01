// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include "IString.h"

namespace Arc {

  PrintFBase::PrintFBase()
    : refcount(1) {}

  PrintFBase::~PrintFBase() {}

  void PrintFBase::Retain() {
    refcount++;
  }

  bool PrintFBase::Release() {
    refcount--;
    return (refcount == 0);
  }

  const char* FindTrans(const char *p) {
#ifdef ENABLE_NLS
    return dgettext(PACKAGE,
                    p ? *p ? p : istring("(empty)") : istring("(null)"));
#else
    return p ? *p ? p : "(empty)" : "(null)";
#endif
  }

  const char* FindNTrans(const char *s, const char *p, unsigned long n) {
#ifdef ENABLE_NLS
    return dngettext(PACKAGE,
                     s ? *s ? s : istring("(empty)") : istring("(null)"),
                     p ? *p ? p : istring("(empty)") : istring("(null)"), n);
#else
    return n == 1 ?
      s ? *s ? s : "(empty)" : "(null)" :
      p ? *p ? p : "(empty)" : "(null)";
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
    if(this == &istr) return *this;
    if (p->Release())
      delete p;
    p = istr.p;
    p->Retain();
    return *this;
  }

  std::string IString::str(void) const {
    std::string s;
    p->msg(s);
    return s;
  }

  std::ostream& operator<<(std::ostream& os, const IString& msg) {
    msg.p->msg(os);
    return os;
  }

} // namespace Arc
