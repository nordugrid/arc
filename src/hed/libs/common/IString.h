#ifndef __ARC_ISTRING__
#define __ARC_ISTRING__

#include <list>
#include <string>
#include <ostream>

#include <stdio.h>  // snprintf
#include <stdlib.h> // free
#include <string.h> // strcpy, strdup

#define istring(x) (x)

namespace Arc {

  class PrintFBase {
   public:
    PrintFBase ();
    virtual ~PrintFBase();
    virtual void msg(std::ostream& os) = 0;
    void Retain();
    bool Release();
   private:
    // Copying not allowed
    PrintFBase (const PrintFBase&);
    // Assignment not allowed
    PrintFBase& operator=(const PrintFBase&);
    int refcount;
  };

  const char* FindTrans(const char* p);

  template <class T0 = int, class T1 = int, class T2 = int, class T3 = int,
	    class T4 = int, class T5 = int, class T6 = int, class T7 = int>
  class PrintF : public PrintFBase {

   public:

    PrintF(const std::string& m,
	   const T0& tt0 = 0, const T1& tt1 = 0,
	   const T2& tt2 = 0, const T3& tt3 = 0,
	   const T4& tt4 = 0, const T5& tt5 = 0,
	   const T6& tt6 = 0, const T7& tt7 = 0)
      : PrintFBase(), m(m) {
      Copy(t0, tt0);
      Copy(t1, tt1);
      Copy(t2, tt2);
      Copy(t3, tt3);
      Copy(t4, tt4);
      Copy(t5, tt5);
      Copy(t6, tt6);
      Copy(t7, tt7);
    }

    ~PrintF() {
      for (std::list<char*>::iterator it = ptrs.begin();
	   it != ptrs.end(); it++)
	free(*it);
    }

    void msg(std::ostream& os) {

      char buffer[2048];
      snprintf(buffer, 2048, Get(m),
	       Get(t0), Get(t1), Get(t2), Get(t3),
	       Get(t4), Get(t5), Get(t6), Get(t7));
      os << buffer;
    }

   private:

    // general case
    template <class T, class U>
    inline void Copy(T& t, const U& u) {
      t = u;
    }

    // char[] and const char[]
    template <class T>
    inline void Copy(T& t, const char u[]) {
      strcpy(t, u);
    }

    // const char*
    inline void Copy(const char*& t, const char* const& u) {
      t = strdup(u);
      ptrs.push_back(const_cast<char*>(t));
    }

    // char*
    inline void Copy(char*& t, char* const& u) {
      t = strdup(u);
      ptrs.push_back(t);
    }

    // general case
    template <class T>
    inline static const T& Get(const T& t) {
      return t;
    }

    // const char[] and const char*
    inline static const char* Get(const char* const& t) {
      return FindTrans(t);
    }

    // char[] and char*
    inline static const char* Get(char* const& t) {
      return FindTrans(const_cast<const char*&>(t));
    }

    // std::string
    inline static const char* Get(const std::string& t) {
      return FindTrans(t.c_str());
    }

    // std::string ()()
    inline static const char* Get(std::string (*t)()) {
      return FindTrans(t().c_str());
    }

    std::string m;
    T0 t0;
    T1 t1;
    T2 t2;
    T3 t3;
    T4 t4;
    T5 t5;
    T6 t6;
    T7 t7;
    std::list<char*> ptrs;
  };

  class IString {

   public:

    IString(const std::string& m)
      : p(new PrintF<>(m)) {}

    template <class T0>
    IString(const std::string& m, const T0& t0)
      : p(new PrintF<T0>(m, t0)) {}

    template <class T0, class T1>
    IString(const std::string& m, const T0& t0, const T1& t1)
      : p(new PrintF<T0, T1>(m, t0, t1)) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, const T0& t0, const T1& t1, const T2& t2)
      : p(new PrintF<T0, T1, T2>(m, t0, t1, t2)) {}

    template <class T0, class T1, class T2, class T3>
    IString(const std::string& m, const T0& t0, const T1& t1, const T2& t2, const T3& t3)
      : p(new PrintF<T0, T1, T2, T3>(m, t0, t1, t2, t3)) {}

    template <class T0, class T1, class T2, class T3, class T4>
    IString(const std::string& m, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4)
      : p(new PrintF<T0, T1, T2, T3, T4>(m, t0, t1, t2, t3, t4)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
      : p(new PrintF<T0, T1, T2, T3, T4, T5>(m, t0, t1, t2, t3, t4, t5)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, T6>(m, t0, t1, t2, t3, t4, t5, t6)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, T6, T7>(m, t0, t1, t2, t3, t4, t5, t6, t7)) {}

    ~IString();

    IString(const IString& istr);
    IString& operator=(const IString& istr);

   private:

    PrintFBase *p;

    friend std::ostream& operator<<(std::ostream& os, const IString& msg);
  };

  std::ostream& operator<<(std::ostream& os, const IString& msg);

} // namespace Arc

#endif // __ARC_ISTRING__
