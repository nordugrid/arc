// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_LDAPQUERY_H__
#define __ARC_LDAPQUERY_H__

#include <list>
#include <string>

#ifdef USE_WIN32_LDAP_API
#include <winldap.h>
#else
#include <ldap.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>

#define SASLMECH "GSI-GSSAPI"

/**
 * LDAP callback type. Your ldap callbacks should be of same structure.
 */
typedef void (*ldap_callback)(const std::string& attr,
                              const std::string& value,
                              void *ref);

namespace ArcDMCLDAP {

  using namespace Arc;

  /**
   *  LDAPQuery class; querying of LDAP servers.
   */
  class LDAPQuery {

  public:
    /**
     * Constructs a new LDAPQuery object and sets connection options.
     * The connection is first established when calling Query.
     */
    LDAPQuery(const std::string& ldaphost,
              int ldapport,
              int timeout,
              bool anonymous = true,
              const std::string& usersn = "");

    /**
     * Destructor. Will disconnect from the ldapserver if still connected.
     */
    ~LDAPQuery();

    /**
     * Queries the ldap server.
     * @return 0: success, 1: timeout, -1: other error
     */
    int Query(const std::string& base,
              const std::string& filter = "(objectclass=*)",
              const std::list<std::string>& attributes =
                std::list<std::string>(),
              URL::Scope scope = URL::subtree);

    /**
     * Retrieves the result of the query from the ldap-server.
     * @return 0: success, 1: timeout, -1: other error
     */
    int Result(ldap_callback callback,
               void *ref);

  private:
    int Connect();
    bool SetConnectionOptions(int version);
    int HandleResult(ldap_callback callback, void *ref);
    void HandleSearchEntry(LDAPMessage *msg,
                           ldap_callback callback,
                           void *ref);

    std::string host;
    int port;
    bool anonymous;
    std::string usersn;
    int timeout;

    ldap *connection;
#ifdef USE_WIN32_LDAP_API
    ULONG messageid;
#else
    int messageid;
#endif

    static Logger logger;

    /*
     * Note that the last pointer is holding allocating memory
     * that must be freed
     */
    friend int my_sasl_interact(ldap*, unsigned int, void*, void*);
  };

} // end namespace

#endif // __ARC_LDAPQUERY_H__
