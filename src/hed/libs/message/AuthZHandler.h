#ifndef __ARC_AUTHZHANDLER_H__
#define __ARC_AUTHZHANDLER_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "Message.h"

namespace Arc {

typedef enum {
  AuthNO = 0,
  AuthYES = 1,
  AuthUNDEFINED = 2,
  AuthUNKNOWN = 3,
  AuthERROR = 4
} AuthResult;

class AuthStatus {
 private:
  AuthResult result_;
 public:
  AuthStatus(AuthResult r = AuthUNDEFINED):result_(r) { };
  ~AuthStatus(void) { };
  operator AuthResult(void) { return result_; };
  AuthStatus& operator =(AuthResult r) { result_=r; return *this; };
};

class AuthZHandler {
 public:
  AuthZHandler(Arc::Config* cfg);
  virtual ~AuthZHandler(void);
  virtual AuthStatus process(Message& msg);
};

class PDP {
 public:
  PDP(Arc::Config* cfg);
  virtual ~PDP(void);
  virtual AuthStatus authorize(MessageAuth& msg);
};

} // namespace Arc

#endif /* __ARC_AUTHZHANDLER_H__ */

