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
  void *rs_limit;     /* struct slap_limits_set* */
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

typedef int (*BI_search_op)(Operation*, SlapReply*);

static BI_search_op shell_back_search;

static int shell_back_search_wrapper(Operation *op, SlapReply *rs) {
  void *save;
  int res;
  save = op->o_request.oq_search.rs_attrs;
  op->o_request.oq_search.rs_attrs = NULL;
  res = shell_back_search(op, rs);
  op->o_request.oq_search.rs_attrs = save;
  return res;
}

} /* extern "C" */

class SlapdWrapper {
public:
  SlapdWrapper();
  ~SlapdWrapper() {}
};

static void* wrapper(void*) {
  void *handle = dlopen(NULL, RTLD_LAZY); /* handle for main program */
  backend_info_t backend_info = (backend_info_t) dlsym(handle, "backend_info");
  if(!backend_info) {
    /* This is not the slapd binary - nothing to wrap */
    dlclose(handle);
    return NULL;
  }

  void *shell_handle = NULL;
  const char *arc_ldaplib_shell = getenv("ARC_LDAPLIB_SHELL");
  if(!arc_ldaplib_shell)
    arc_ldaplib_shell = "/usr/lib/ldap/back_shell.so";
  shell_back_search = (BI_search_op) dlsym(handle, "shell_back_search");
  if(!shell_back_search) {
    /* Shell backend is not compiled statically into main program
       Searching in modules... */
    shell_handle = dlopen(arc_ldaplib_shell, RTLD_LAZY);
    if(!shell_handle) {
      std::cerr << "Error: Unable to dlopen " << arc_ldaplib_shell << std::endl;
      exit(1);
    }
    shell_back_search = (BI_search_op) dlsym(shell_handle, "shell_back_search");
  }

  if(!shell_back_search) {
    std::cerr << "Error: shell_back_search found neither in main program, nor in " << arc_ldaplib_shell << std::endl;
    exit(1);
  }

  int i = 0;
  void **backendInfo = NULL;
  do {
    sleep(1); /* let the slapd server start */
    backendInfo = backend_info("shell");
    i++;
  }
  while (!backendInfo && i < 150);

  if(backendInfo)
    for(i = 0; i < 100; i++) {
      if(backendInfo[i] == (void*)shell_back_search) {
	backendInfo[i] = (void*)&shell_back_search_wrapper;
	break;
      }
    }
  else {
    std::cerr << "Error: slapd shell backend not found - exiting" << std::endl;
    exit(1);
  }

  if(shell_handle)
    dlclose(shell_handle);
  dlclose(handle);

  return NULL;
}

SlapdWrapper::SlapdWrapper() {
  pthread_t pid;
  pthread_create(&pid, NULL, &wrapper, NULL);
  pthread_detach(pid);
}

SlapdWrapper slapdwrapper;
