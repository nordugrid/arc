// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_LDAPQUERY_H__
#define __ARC_LDAPQUERY_H__

#include <list>
#include <string>

#include <ldap.h>

#include <arc/Logger.h>
#include <arc/URL.h>

#define SASLMECH "GSI-GSSAPI"
#define TIMEOUT 10

/**
 * LDAP callback type. Your ldap callbacks should be of same structure.
 */
typedef void (*ldap_callback)(const std::string& attr,
                              const std::string& value,
                              void *ref);

namespace Arc {

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
              bool anonymous = true,
              const std::string& usersn = "",
              int timeout = TIMEOUT);

    /**
     * Destructor. Will disconnect from the ldapserver if still connected.
     */
    ~LDAPQuery();

    /**
     * Queries the ldap server.
     */
    bool Query(const std::string& base,
               const std::string& filter = "(objectclass=*)",
               const std::list<std::string>& attributes =
                 std::list<std::string>(),
               URL::Scope scope = URL::subtree);

    /**
     * Retrieves the result of the query from the ldap-server.
     */
    bool Result(ldap_callback callback,
                void *ref);

  private:
    bool Connect();
    bool SetConnectionOptions(int version);
    bool HandleResult(ldap_callback callback, void *ref);
    void HandleSearchEntry(LDAPMessage *msg,
                           ldap_callback callback,
                           void *ref);

    std::string host;
    int port;
    bool anonymous;
    std::string usersn;
    int timeout;

    ldap *connection;
    int messageid;

    static Logger logger;

    friend int my_sasl_interact(ldap*, unsigned int, void*, void*);
  };

} // end namespace

#endif // __ARC_LDAPQUERY_H__
