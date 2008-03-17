#ifndef __ARC_ISTRING__
#define __ARC_ISTRING__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <ostream>

#include <string.h>

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

  void FindTrans(const char*& p);

  template <class T0 = int, class T1 = int, class T2 = int, class T3 = int,
	    class T4 = int, class T5 = int, class T6 = int, class T7 = int>
  class PrintF : public PrintFBase {

   public:

    PrintF(const std::string& m,
	   const T0& tt0 = 0, const T1& tt1 = 0,
	   const T2& tt2 = 0, const T3& tt3 = 0,
	   const T4& tt4 = 0, const T5& tt5 = 0,
	   const T6& tt6 = 0, const T7& tt7 = 0)
      : PrintFBase(), m(m),
	t0(tt0), t1(tt1), t2(tt2), t3(tt3),
	t4(tt4), t5(tt5), t6(tt6), t7(tt7) {
      if (typeid(T0) == typeid(char*) || typeid(T0) == typeid(const char*))
	*(char**)&t0 = strdup(*(const char**)&tt0);
      if (typeid(T1) == typeid(char*) || typeid(T1) == typeid(const char*))
	*(char**)&t1 = strdup(*(const char**)&tt1);
      if (typeid(T2) == typeid(char*) || typeid(T2) == typeid(const char*))
	*(char**)&t2 = strdup(*(const char**)&tt2);
      if (typeid(T3) == typeid(char*) || typeid(T3) == typeid(const char*))
	*(char**)&t3 = strdup(*(const char**)&tt3);
      if (typeid(T4) == typeid(char*) || typeid(T4) == typeid(const char*))
	*(char**)&t4 = strdup(*(const char**)&tt4);
      if (typeid(T5) == typeid(char*) || typeid(T5) == typeid(const char*))
	*(char**)&t5 = strdup(*(const char**)&tt5);
      if (typeid(T6) == typeid(char*) || typeid(T6) == typeid(const char*))
	*(char**)&t6 = strdup(*(const char**)&tt6);
      if (typeid(T7) == typeid(char*) || typeid(T7) == typeid(const char*))
	*(char**)&t7 = strdup(*(const char**)&tt7);
    }

    ~PrintF() {
      if (typeid(T0) == typeid(char*) || typeid(T0) == typeid(const char*))
	free(*(char**)&t0);
      if (typeid(T1) == typeid(char*) || typeid(T1) == typeid(const char*))
	free(*(char**)&t1);
      if (typeid(T2) == typeid(char*) || typeid(T2) == typeid(const char*))
	free(*(char**)&t2);
      if (typeid(T3) == typeid(char*) || typeid(T3) == typeid(const char*))
	free(*(char**)&t3);
      if (typeid(T4) == typeid(char*) || typeid(T4) == typeid(const char*))
	free(*(char**)&t4);
      if (typeid(T5) == typeid(char*) || typeid(T5) == typeid(const char*))
	free(*(char**)&t5);
      if (typeid(T6) == typeid(char*) || typeid(T6) == typeid(const char*))
	free(*(char**)&t6);
      if (typeid(T7) == typeid(char*) || typeid(T7) == typeid(const char*))
	free(*(char**)&t7);
    }

    void msg(std::ostream& os) {
      T0 s0 = t0;
      T1 s1 = t1;
      T2 s2 = t2;
      T3 s3 = t3;
      T4 s4 = t4;
      T5 s5 = t5;
      T6 s6 = t6;
      T7 s7 = t7;

      const char *message = m.c_str();

      if (typeid(T0) == typeid(char*) || typeid(T0) == typeid(const char*))
	FindTrans(*(const char**)&s0);
      if (typeid(T1) == typeid(char*) || typeid(T1) == typeid(const char*))
	FindTrans(*(const char**)&s1);
      if (typeid(T2) == typeid(char*) || typeid(T2) == typeid(const char*))
	FindTrans(*(const char**)&s2);
      if (typeid(T3) == typeid(char*) || typeid(T3) == typeid(const char*))
	FindTrans(*(const char**)&s3);
      if (typeid(T4) == typeid(char*) || typeid(T4) == typeid(const char*))
	FindTrans(*(const char**)&s4);
      if (typeid(T5) == typeid(char*) || typeid(T5) == typeid(const char*))
	FindTrans(*(const char**)&s5);
      if (typeid(T6) == typeid(char*) || typeid(T6) == typeid(const char*))
	FindTrans(*(const char**)&s6);
      if (typeid(T7) == typeid(char*) || typeid(T7) == typeid(const char*))
	FindTrans(*(const char**)&s7);

      FindTrans(message);

      char buffer[2048];

      snprintf(buffer, 2048, message, s0, s1, s2, s3, s4, s5, s6, s7);

      os << buffer;
    }

   private:

    std::string m;
    T0 t0;
    T1 t1;
    T2 t2;
    T3 t3;
    T4 t4;
    T5 t5;
    T6 t6;
    T7 t7;
  };

  class IString {

   public:

    IString(const std::string& m)
      : p(new PrintF<>(m)) {}

    template <class T0>
    IString(const std::string& m, T0 t0)
      : p(new PrintF<T0>(m, t0)) {}

    IString(const std::string& m, const std::string& t0)
      : p(new PrintF<const char*>(m, t0.c_str())) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1)
      : p(new PrintF<T0, T1>(m, t0, t1)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1)
      : p(new PrintF<T0, const char*>(m, t0, t1.c_str())) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1)
      : p(new PrintF<const char*, T1>(m, t0.c_str(), t1)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1)
      : p(new PrintF<const char*, const char*>(m, t0.c_str(), t1.c_str())) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2)
      : p(new PrintF<T0, T1, T2>(m, t0, t1, t2)) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2)
      : p(new PrintF<T0, T1, const char*>(m, t0, t1, t2.c_str())) {}

    template <class T0, class T2>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2)
      : p(new PrintF<T0, const char*, T2>(m, t0, t1.c_str(), t2)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2)
      : p(new PrintF<T0, const char*, const char*>(m, t0, t1.c_str(), t2.c_str())) {}

    template <class T1, class T2>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2)
      : p(new PrintF<const char*, T1, T2>(m, t0.c_str(), t1, t2)) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2)
      : p(new PrintF<const char*, T1, const char*>(m, t0.c_str(), t1, t2.c_str())) {}

    template <class T2>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2)
      : p(new PrintF<const char*, const char*, T2>(m, t0.c_str(), t1.c_str(), t2)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2)
      : p(new PrintF<const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str())) {}

    template <class T0, class T1, class T2, class T3>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3)
      : p(new PrintF<T0, T1, T2, T3>(m, t0, t1, t2, t3)) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3)
      : p(new PrintF<T0, T1, T2, const char*>(m, t0, t1, t2, t3.c_str())) {}

    template <class T0, class T1, class T3>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3)
      : p(new PrintF<T0, T1, const char*, T3>(m, t0, t1, t2.c_str(), t3)) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3)
      : p(new PrintF<T0, T1, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str())) {}

    template <class T0, class T2, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3)
      : p(new PrintF<T0, const char*, T2, T3>(m, t0, t1.c_str(), t2, t3)) {}

    template <class T0, class T2>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3)
      : p(new PrintF<T0, const char*, T2, const char*>(m, t0, t1.c_str(), t2, t3.c_str())) {}

    template <class T0, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3)
      : p(new PrintF<T0, const char*, const char*, T3>(m, t0, t1.c_str(), t2.c_str(), t3)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3)
      : p(new PrintF<T0, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str())) {}

    template <class T1, class T2, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3)
      : p(new PrintF<const char*, T1, T2, T3>(m, t0.c_str(), t1, t2, t3)) {}

    template <class T1, class T2>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3)
      : p(new PrintF<const char*, T1, T2, const char*>(m, t0.c_str(), t1, t2, t3.c_str())) {}

    template <class T1, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3)
      : p(new PrintF<const char*, T1, const char*, T3>(m, t0.c_str(), t1, t2.c_str(), t3)) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3)
      : p(new PrintF<const char*, T1, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str())) {}

    template <class T2, class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3)
      : p(new PrintF<const char*, const char*, T2, T3>(m, t0.c_str(), t1.c_str(), t2, t3)) {}

    template <class T2>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3)
      : p(new PrintF<const char*, const char*, T2, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str())) {}

    template <class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3)
      : p(new PrintF<const char*, const char*, const char*, T3>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3)
      : p(new PrintF<const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4)
      : p(new PrintF<T0, T1, T2, T3, T4>(m, t0, t1, t2, t3, t4)) {}

    template <class T0, class T1, class T2, class T3>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4)
      : p(new PrintF<T0, T1, T2, T3, const char*>(m, t0, t1, t2, t3, t4.c_str())) {}

    template <class T0, class T1, class T2, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4)
      : p(new PrintF<T0, T1, T2, const char*, T4>(m, t0, t1, t2, t3.c_str(), t4)) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<T0, T1, T2, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str())) {}

    template <class T0, class T1, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4)
      : p(new PrintF<T0, T1, const char*, T3, T4>(m, t0, t1, t2.c_str(), t3, t4)) {}

    template <class T0, class T1, class T3>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4)
      : p(new PrintF<T0, T1, const char*, T3, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str())) {}

    template <class T0, class T1, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4)
      : p(new PrintF<T0, T1, const char*, const char*, T4>(m, t0, t1, t2.c_str(), t3.c_str(), t4)) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<T0, T1, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str())) {}

    template <class T0, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4)
      : p(new PrintF<T0, const char*, T2, T3, T4>(m, t0, t1.c_str(), t2, t3, t4)) {}

    template <class T0, class T2, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4)
      : p(new PrintF<T0, const char*, T2, T3, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str())) {}

    template <class T0, class T2, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4)
      : p(new PrintF<T0, const char*, T2, const char*, T4>(m, t0, t1.c_str(), t2, t3.c_str(), t4)) {}

    template <class T0, class T2>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<T0, const char*, T2, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str())) {}

    template <class T0, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4)
      : p(new PrintF<T0, const char*, const char*, T3, T4>(m, t0, t1.c_str(), t2.c_str(), t3, t4)) {}

    template <class T0, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4)
      : p(new PrintF<T0, const char*, const char*, T3, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str())) {}

    template <class T0, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4)
      : p(new PrintF<T0, const char*, const char*, const char*, T4>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str())) {}

    template <class T1, class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4)
      : p(new PrintF<const char*, T1, T2, T3, T4>(m, t0.c_str(), t1, t2, t3, t4)) {}

    template <class T1, class T2, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4)
      : p(new PrintF<const char*, T1, T2, T3, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str())) {}

    template <class T1, class T2, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4)
      : p(new PrintF<const char*, T1, T2, const char*, T4>(m, t0.c_str(), t1, t2, t3.c_str(), t4)) {}

    template <class T1, class T2>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<const char*, T1, T2, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str())) {}

    template <class T1, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4)
      : p(new PrintF<const char*, T1, const char*, T3, T4>(m, t0.c_str(), t1, t2.c_str(), t3, t4)) {}

    template <class T1, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4)
      : p(new PrintF<const char*, T1, const char*, T3, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str())) {}

    template <class T1, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4)
      : p(new PrintF<const char*, T1, const char*, const char*, T4>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4)) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str())) {}

    template <class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4)
      : p(new PrintF<const char*, const char*, T2, T3, T4>(m, t0.c_str(), t1.c_str(), t2, t3, t4)) {}

    template <class T2, class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4)
      : p(new PrintF<const char*, const char*, T2, T3, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str())) {}

    template <class T2, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4)
      : p(new PrintF<const char*, const char*, T2, const char*, T4>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4)) {}

    template <class T2>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str())) {}

    template <class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4)
      : p(new PrintF<const char*, const char*, const char*, T3, T4>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4)) {}

    template <class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str())) {}

    template <class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<T0, T1, T2, T3, T4, T5>(m, t0, t1, t2, t3, t4, t5)) {}

    template <class T0, class T1, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*>(m, t0, t1, t2, t3, t4, t5.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5>(m, t0, t1, t2, t3, t4.c_str(), t5)) {}

    template <class T0, class T1, class T2, class T3>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5>(m, t0, t1, t2, t3.c_str(), t4, t5)) {}

    template <class T0, class T1, class T2, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str())) {}

    template <class T0, class T1, class T2, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5)) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5>(m, t0, t1, t2.c_str(), t3, t4, t5)) {}

    template <class T0, class T1, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str())) {}

    template <class T0, class T1, class T3, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5)) {}

    template <class T0, class T1, class T3>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str())) {}

    template <class T0, class T1, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5)) {}

    template <class T0, class T1, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str())) {}

    template <class T0, class T1, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5)) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5>(m, t0, t1.c_str(), t2, t3, t4, t5)) {}

    template <class T0, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str())) {}

    template <class T0, class T2, class T3, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5)) {}

    template <class T0, class T2, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str())) {}

    template <class T0, class T2, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5)) {}

    template <class T0, class T2, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str())) {}

    template <class T0, class T2, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5)) {}

    template <class T0, class T2>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T0, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5)) {}

    template <class T0, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str())) {}

    template <class T0, class T3, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5)) {}

    template <class T0, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str())) {}

    template <class T0, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5)) {}

    template <class T0, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str())) {}

    template <class T0, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5>(m, t0.c_str(), t1, t2, t3, t4, t5)) {}

    template <class T1, class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str())) {}

    template <class T1, class T2, class T3, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5)) {}

    template <class T1, class T2, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str())) {}

    template <class T1, class T2, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5)) {}

    template <class T1, class T2, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str())) {}

    template <class T1, class T2, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5)) {}

    template <class T1, class T2>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T1, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5)) {}

    template <class T1, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str())) {}

    template <class T1, class T3, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5)) {}

    template <class T1, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str())) {}

    template <class T1, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5)) {}

    template <class T1, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str())) {}

    template <class T1, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5)) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T2, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5)) {}

    template <class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str())) {}

    template <class T2, class T3, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5)) {}

    template <class T2, class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str())) {}

    template <class T2, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5)) {}

    template <class T2, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str())) {}

    template <class T2, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5)) {}

    template <class T2>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5)) {}

    template <class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str())) {}

    template <class T3, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5)) {}

    template <class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str())) {}

    template <class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5)) {}

    template <class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str())) {}

    template <class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, T6>(m, t0, t1, t2, t3, t4, t5, t6)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, const char*>(m, t0, t1, t2, t3, t4, t5, t6.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*, T6>(m, t0, t1, t2, t3, t4, t5.c_str(), t6)) {}

    template <class T0, class T1, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*, const char*>(m, t0, t1, t2, t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5, T6>(m, t0, t1, t2, t3, t4.c_str(), t5, t6)) {}

    template <class T0, class T1, class T2, class T3, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*, T6>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T1, class T2, class T3>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5, T6>(m, t0, t1, t2, t3.c_str(), t4, t5, t6)) {}

    template <class T0, class T1, class T2, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*, T6>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T0, class T1, class T2, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T2, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5, T6>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T0, class T1, class T2, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T1, class T2, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*, T6>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5, T6>(m, t0, t1, t2.c_str(), t3, t4, t5, t6)) {}

    template <class T0, class T1, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5, t6.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*, T6>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str(), t6)) {}

    template <class T0, class T1, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5, T6>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5, t6)) {}

    template <class T0, class T1, class T3, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T1, class T3, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*, T6>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T1, class T3>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5, T6>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5, t6)) {}

    template <class T0, class T1, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T0, class T1, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*, T6>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T0, class T1, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5, T6>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T0, class T1, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T1, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*, T6>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5, T6>(m, t0, t1.c_str(), t2, t3, t4, t5, t6)) {}

    template <class T0, class T2, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5, t6.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*, T6>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str(), t6)) {}

    template <class T0, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T2, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5, T6>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5, t6)) {}

    template <class T0, class T2, class T3, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T2, class T3, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*, T6>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T2, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T2, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5, T6>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5, t6)) {}

    template <class T0, class T2, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T0, class T2, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*, T6>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T0, class T2, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T2, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5, T6>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T0, class T2, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T2, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*, T6>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T2>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5, T6>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5, t6)) {}

    template <class T0, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5, t6.c_str())) {}

    template <class T0, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*, T6>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6)) {}

    template <class T0, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5, T6>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6)) {}

    template <class T0, class T3, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T3, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*, T6>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T0, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5, T6>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6)) {}

    template <class T0, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T0, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*, T6>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T0, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T0, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5, T6>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T0, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T0, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*, T6>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5, T6>(m, t0.c_str(), t1, t2, t3, t4, t5, t6)) {}

    template <class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5, t6.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*, T6>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str(), t6)) {}

    template <class T1, class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T1, class T2, class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5, T6>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5, t6)) {}

    template <class T1, class T2, class T3, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T1, class T2, class T3, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*, T6>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T1, class T2, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T1, class T2, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5, T6>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5, t6)) {}

    template <class T1, class T2, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T1, class T2, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*, T6>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T1, class T2, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T1, class T2, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5, T6>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T1, class T2, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T1, class T2, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*, T6>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T1, class T2>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T1, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5, T6>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5, t6)) {}

    template <class T1, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5, t6.c_str())) {}

    template <class T1, class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*, T6>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str(), t6)) {}

    template <class T1, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T1, class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5, T6>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5, t6)) {}

    template <class T1, class T3, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T1, class T3, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*, T6>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T1, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T1, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5, T6>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5, t6)) {}

    template <class T1, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T1, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*, T6>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T1, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T1, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5, T6>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T1, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T1, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*, T6>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5, T6>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5, t6)) {}

    template <class T2, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5, t6.c_str())) {}

    template <class T2, class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*, T6>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str(), t6)) {}

    template <class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T2, class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5, T6>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5, t6)) {}

    template <class T2, class T3, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T2, class T3, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*, T6>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T2, class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T2, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5, T6>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5, t6)) {}

    template <class T2, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T2, class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*, T6>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T2, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T2, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5, T6>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T2, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T2, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*, T6>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    template <class T2>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5, t6)) {}

    template <class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5, t6.c_str())) {}

    template <class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6)) {}

    template <class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6.c_str())) {}

    template <class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6)) {}

    template <class T3, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6.c_str())) {}

    template <class T3, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6)) {}

    template <class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6)) {}

    template <class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6.c_str())) {}

    template <class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6)) {}

    template <class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str())) {}

    template <class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6)) {}

    template <class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str())) {}

    template <class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*, T6>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, T6, T7>(m, t0, t1, t2, t3, t4, t5, t6, t7)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, T6, const char*>(m, t0, t1, t2, t3, t4, t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, const char*, T7>(m, t0, t1, t2, t3, t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, T4, T5, const char*, const char*>(m, t0, t1, t2, t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*, T6, T7>(m, t0, t1, t2, t3, t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T2, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*, T6, const char*>(m, t0, t1, t2, t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T4, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*, const char*, T7>(m, t0, t1, t2, t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, T4, const char*, const char*, const char*>(m, t0, t1, t2, t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5, T6, T7>(m, t0, t1, t2, t3, t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T1, class T2, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5, T6, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5, const char*, T7>(m, t0, t1, t2, t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T3, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, T5, const char*, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*, T6, T7>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T2, class T3, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*, T6, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T3, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*, const char*, T7>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T3>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, T3, const char*, const char*, const char*, const char*>(m, t0, t1, t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5, T6, T7>(m, t0, t1, t2, t3.c_str(), t4, t5, t6, t7)) {}

    template <class T0, class T1, class T2, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5, T6, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5, const char*, T7>(m, t0, t1, t2, t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, T5, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*, T6, T7>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T2, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*, T6, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T4, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*, const char*, T7>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T4>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, T4, const char*, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5, T6, T7>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T1, class T2, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5, T6, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5, const char*, T7>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T2, class T5>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, T5, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T2, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*, T6, T7>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T2, class T6>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*, T6, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T2, class T7>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*, const char*, T7>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T2>
    IString(const std::string& m, T0 t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, T2, const char*, const char*, const char*, const char*, const char*>(m, t0, t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5, T6, T7>(m, t0, t1, t2.c_str(), t3, t4, t5, t6, t7)) {}

    template <class T0, class T1, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5, T6, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5, const char*, T7>(m, t0, t1, t2.c_str(), t3, t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, T5, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*, T6, T7>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*, T6, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T3, class T4, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*, const char*, T7>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T3, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, T4, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T3, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5, T6, T7>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T1, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5, T6, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T3, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5, const char*, T7>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T3, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, T5, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T3, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*, T6, T7>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T3, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*, T6, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T3, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*, const char*, T7>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T3>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, T3, const char*, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5, T6, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5, t6, t7)) {}

    template <class T0, class T1, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5, T6, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5, const char*, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T4, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, T5, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*, T6, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T4, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*, T6, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T4, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*, const char*, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1, class T4>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, T4, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5, T6, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T1, class T5, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5, T6, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T1, class T5, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5, const char*, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T1, class T5>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, T5, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T1, class T6, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*, T6, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T1, class T6>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*, T6, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T1, class T7>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*, const char*, T7>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T1>
    IString(const std::string& m, T0 t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, T1, const char*, const char*, const char*, const char*, const char*, const char*>(m, t0, t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5, T6, T7>(m, t0, t1.c_str(), t2, t3, t4, t5, t6, t7)) {}

    template <class T0, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5, T6, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5, t6, t7.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5, const char*, T7>(m, t0, t1.c_str(), t2, t3, t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T2, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, T5, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*, T6, T7>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T2, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*, T6, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T2, class T3, class T4, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*, const char*, T7>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T2, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, T4, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T3, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5, T6, T7>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T2, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5, T6, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T2, class T3, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5, const char*, T7>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T2, class T3, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, T5, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T3, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*, T6, T7>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T2, class T3, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*, T6, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T2, class T3, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*, const char*, T7>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T2, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, T3, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5, T6, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5, t6, t7)) {}

    template <class T0, class T2, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5, T6, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T0, class T2, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5, const char*, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T2, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, T5, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*, T6, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T2, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*, T6, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T2, class T4, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*, const char*, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T2, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, T4, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5, T6, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T2, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5, T6, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T2, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5, const char*, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T2, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, T5, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T2, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*, T6, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T2, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*, T6, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T2, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*, const char*, T7>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T2>
    IString(const std::string& m, T0 t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, T2, const char*, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5, t6, t7)) {}

    template <class T0, class T3, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5, t6, t7.c_str())) {}

    template <class T0, class T3, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T3, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, T5, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T3, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T3, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T3, class T4, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T3, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, T4, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T3, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T3, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T3, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T3, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, T5, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T3, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T3, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T3, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T3>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, T3, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T4, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6, t7)) {}

    template <class T0, class T4, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T0, class T4, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T0, class T4, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, T5, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T4, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T0, class T4, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T4, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T0, class T4>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, T4, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T0, class T5, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T0, class T5, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T0, class T5, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T0, class T5>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, T5, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T0, class T6, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*, T6, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T0, class T6>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*, T6, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T0, class T7>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*, const char*, T7>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T0>
    IString(const std::string& m, T0 t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<T0, const char*, const char*, const char*, const char*, const char*, const char*, const char*>(m, t0, t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5, T6, T7>(m, t0.c_str(), t1, t2, t3, t4, t5, t6, t7)) {}

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5, T6, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5, t6, t7.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5, const char*, T7>(m, t0.c_str(), t1, t2, t3, t4, t5, t6.c_str(), t7)) {}

    template <class T1, class T2, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, T5, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*, T6, T7>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str(), t6, t7)) {}

    template <class T1, class T2, class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*, T6, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T2, class T3, class T4, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*, const char*, T7>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, T4, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T3, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5, T6, T7>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5, t6, t7)) {}

    template <class T1, class T2, class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5, T6, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T1, class T2, class T3, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5, const char*, T7>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T1, class T2, class T3, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, T5, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T3, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*, T6, T7>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T1, class T2, class T3, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*, T6, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T2, class T3, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*, const char*, T7>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T2, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, T3, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5, T6, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5, t6, t7)) {}

    template <class T1, class T2, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5, T6, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T1, class T2, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5, const char*, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T1, class T2, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, T5, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*, T6, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T1, class T2, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*, T6, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T2, class T4, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*, const char*, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T2, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, T4, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5, T6, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T1, class T2, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5, T6, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T1, class T2, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5, const char*, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T1, class T2, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, T5, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T2, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*, T6, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T1, class T2, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*, T6, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T2, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*, const char*, T7>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T2>
    IString(const std::string& m, const std::string& t0, T1 t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, T2, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5, t6, t7)) {}

    template <class T1, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5, t6, t7.c_str())) {}

    template <class T1, class T3, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5, t6.c_str(), t7)) {}

    template <class T1, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, T5, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T3, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str(), t6, t7)) {}

    template <class T1, class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T3, class T4, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T3, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, T4, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T3, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5, t6, t7)) {}

    template <class T1, class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T1, class T3, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T1, class T3, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, T5, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T3, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T1, class T3, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T3, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T3>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, T3, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5, t6, t7)) {}

    template <class T1, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T1, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T1, class T4, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, T5, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T1, class T4, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T4, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T1, class T4>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, T4, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T1, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T1, class T5, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T1, class T5, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T1, class T5>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, T5, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T1, class T6, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*, T6, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T1, class T6>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*, T6, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T1, class T7>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*, const char*, T7>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T1>
    IString(const std::string& m, const std::string& t0, T1 t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, T1, const char*, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1, t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T2, class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5, t6, t7)) {}

    template <class T2, class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5, t6, t7.c_str())) {}

    template <class T2, class T3, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5, t6.c_str(), t7)) {}

    template <class T2, class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T2, class T3, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str(), t6, t7)) {}

    template <class T2, class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T2, class T3, class T4, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T2, class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, T4, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T2, class T3, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5, t6, t7)) {}

    template <class T2, class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T2, class T3, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T2, class T3, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T2, class T3, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T2, class T3, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T2, class T3, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T2, class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, T3, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T2, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5, t6, t7)) {}

    template <class T2, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T2, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T2, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T2, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T2, class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T2, class T4, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T2, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, T4, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T2, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T2, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T2, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T2, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T2, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T2, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T2, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T2>
    IString(const std::string& m, const std::string& t0, const std::string& t1, T2 t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, T2, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2, t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T3, class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5, t6, t7)) {}

    template <class T3, class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5, t6, t7.c_str())) {}

    template <class T3, class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5, t6.c_str(), t7)) {}

    template <class T3, class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T3, class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6, t7)) {}

    template <class T3, class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T3, class T4, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T3, class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, T4, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T3, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6, t7)) {}

    template <class T3, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T3, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T3, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T3, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T3, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T3, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    template <class T3>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, T3 t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, T3, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3, t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T4, class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6, t7)) {}

    template <class T4, class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6, t7.c_str())) {}

    template <class T4, class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7)) {}

    template <class T4, class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5, t6.c_str(), t7.c_str())) {}

    template <class T4, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7)) {}

    template <class T4, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6, t7.c_str())) {}

    template <class T4, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7)) {}

    template <class T4>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, T4 t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, T4, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4, t5.c_str(), t6.c_str(), t7.c_str())) {}

    template <class T5, class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7)) {}

    template <class T5, class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6, t7.c_str())) {}

    template <class T5, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7)) {}

    template <class T5>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, T5 t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, T5, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5, t6.c_str(), t7.c_str())) {}

    template <class T6, class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*, T6, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7)) {}

    template <class T6>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, T6 t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*, T6, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6, t7.c_str())) {}

    template <class T7>
    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, T7 t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*, const char*, T7>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7)) {}

    IString(const std::string& m, const std::string& t0, const std::string& t1, const std::string& t2, const std::string& t3, const std::string& t4, const std::string& t5, const std::string& t6, const std::string& t7)
      : p(new PrintF<const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*>(m, t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str(), t4.c_str(), t5.c_str(), t6.c_str(), t7.c_str())) {}

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
