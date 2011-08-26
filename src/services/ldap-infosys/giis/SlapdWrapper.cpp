/* Quite an ugly hack to return all attributes from slapd-shell even if
   a attribute name is given.
   Needed in order to be compatible with clients expecting the non-standard
   behaviour of the Globus GIIS backend. */

#include <dlfcn.h>
#include <lber.h>
#include <ldap_features.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

extern "C" {

typedef void** (*backend_info_t)(const char*);

/* These are simplified versions of some structs and unions in the openldap
   internal header ldap.h
   Most pointers to structs have been replaced with void pointers, since
   only the size of the pointers is important. */

typedef struct req_search_s {
  int rs_scope; 
  int rs_deref; 
  int rs_slimit; 
  int rs_tlimit; 
  void *rs_limit;     /* struct slap_limits_set* */
  int rs_attrsonly;
  void *rs_attrs;     /* AttributeName*          */
  void *rs_filter;    /* Filter*                 */
  struct berval rs_filterstr;
} req_search_s;

#if LDAP_VENDOR_VERSION >= 20300  /* openldap 2.3 and later */

typedef union OpRequest {
  req_search_s oq_search;
  /* There are more fields in this union, but they are not relevant
     for this hack. */
} OpRequest;

typedef struct Operation {
  void *o_hdr;        /* Opheader*               */
  ber_tag_t o_tag;
  time_t o_time;
  int o_tincr;
  void *o_bd;         /* BackendDB*              */
  struct berval o_req_dn;
  struct berval o_req_ndn;
  OpRequest o_request;
  /* There are more fields in this struct, but they are not relevant
     for this hack. */
} Operation;

typedef struct SlapReply SlapReply;

#else  /* openldap 2.2 */

typedef struct slap_op {
  unsigned long o_opid;
  unsigned long o_connid;
  void *o_conn;       /* struct slap_conn*       */
  void *o_bd;         /* BackendDB*              */
  ber_int_t o_msgid;
  ber_int_t o_protocol;
  ber_tag_t o_tag;
  time_t o_time;
  struct berval o_req_dn;
  struct berval o_req_ndn;
  union o_req_u {
    req_search_s oq_search;
    /* There are more fields in this union, but they are not relevant
       for this hack. */
  } o_request;
  /* There are more fields in this struct, but they are not relevant
     for this hack. */
} Operation;

typedef struct slap_rep SlapReply;

#endif

typedef int (*BI_bi_func)(void**);
typedef int (*BI_op_func)(Operation*, SlapReply*);

static int shell_back_search_wrapper(Operation *op, SlapReply *rs) {
  static BI_op_func shell_back_search = NULL;

  if (!shell_back_search) {
    shell_back_search =
      (BI_op_func) dlsym(RTLD_DEFAULT, "shell_back_search");
  }

  if (!shell_back_search) {
    std::cerr << "Can not find shell_back_search" << std::endl;
    exit(1);
  }

  void* save = op->o_request.oq_search.rs_attrs;
  op->o_request.oq_search.rs_attrs = NULL;
  int res = shell_back_search(op, rs);
  op->o_request.oq_search.rs_attrs = save;
  return res;
}

static int shell_back_initialize_wrapper(void **bi) {
  static BI_bi_func shell_back_initialize = NULL;

  if (!shell_back_initialize) {
    shell_back_initialize =
      (BI_bi_func) dlsym(RTLD_DEFAULT, "shell_back_initialize");
  }

  if (!shell_back_initialize) {
    std::cerr << "Can not find shell_back_initialize" << std::endl;
    exit(1);
  }

  int res = shell_back_initialize(bi);

  static BI_op_func shell_back_search = NULL;

  if (!shell_back_search) {
    shell_back_search =
      (BI_op_func) dlsym(RTLD_DEFAULT, "shell_back_search");
  }

  if (!shell_back_search) {
    std::cerr << "Can not find shell_back_search" << std::endl;
    exit(1);
  }

  for (int i = 0; i < 100; i++) {
    if (bi[i] == (void*)shell_back_search) {
      bi[i] = (void*)&shell_back_search_wrapper;
      break;
    }
  }

  return res;
}

int init_module(int, char**) {
  backend_info_t backend_info =
    (backend_info_t) dlsym(RTLD_DEFAULT, "backend_info");

  if (!backend_info) {
    std::cerr << "Can not find backend_info" << std::endl;
    exit(1);
  }

  void **bi = backend_info("shell");

  if (bi) {
    BI_op_func shell_back_search =
      (BI_op_func) dlsym(RTLD_DEFAULT, "shell_back_search");

    if (!shell_back_search) {
      std::cerr << "Can not find shell_back_search" << std::endl;
      exit(1);
    }

    if (shell_back_search) {
      for (int i = 0; i < 100; i++) {
        if (bi[i] == (void*)shell_back_search) {
          bi[i] = (void*)&shell_back_search_wrapper;
          break;
        }
      }
    }
  }

  return 0;
}

} /* extern "C" */

class SlapdWrapper {
public:
  SlapdWrapper();
  ~SlapdWrapper() {}
};

SlapdWrapper::SlapdWrapper() {
  BI_op_func shell_back_initialize =
    (BI_op_func) dlsym(RTLD_DEFAULT, "shell_back_initialize");

  if (shell_back_initialize) {
    // Look for shell_back_initialize in backend info for static backends
    void** slap_binfo = (void**) dlsym(RTLD_DEFAULT, "slap_binfo");
    if (slap_binfo) {
      for (int i = 0; i < 2000; i++) {
        if (slap_binfo[i] == (void*)shell_back_initialize) {
          slap_binfo[i] = (void*)&shell_back_initialize_wrapper;
          break;
        }
      }
    }
  }
  else {
    std::cerr << "The shell_back_initialize symbol does not exist in default scope." << std::endl;
    std::cerr << "Try adding the slapd wrapper as a module instead." << std::endl;
  }
}

SlapdWrapper slapdwrapper;
