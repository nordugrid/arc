/*
 * Interface for querying ldap servers. Should support GSSGSI-API for
 * SASL if your environment is set up correctly, however so far it
 * isn't necessary in the ARC.
 */

#ifndef ARCLIB_LDAPQUERY
#define ARCLIB_LDAPQUERY

#include <ldap.h>

#include <list>
#include <string>
#include <vector>

#include <arc/URL.h>

#define SASLMECH "GSI-GSSAPI"

namespace gridftpd {

  /** LdapQuery exception. Gets thrown whan an error occurs in a query. */
  class LdapQueryError : public std::exception {
    public:
      /** Standard exception class constructor. */
      LdapQueryError(std::string message): message(message) {}
      ~LdapQueryError() throw() {}
      virtual const char* what() const throw() { return message.c_str(); }
    private:
      std::string message;
  };


  /**
   * LDAP callback type. Your ldap callbacks should be of same structure.
   */
  typedef void (*ldap_callback)(const std::string& attr,
                                const std::string& value,
                                void *ref);


  /**
   *  LdapQuery class; querying of LDAP servers.
   */
  class LdapQuery {

    public:
      /**
       * Constructs a new LdapQuery object and sets connection options.
       * The connection is first established when calling Query.
       */
      LdapQuery(const std::string& ldaphost,
                int ldapport,
                bool anonymous = true,
                const std::string& usersn = "",
                int timeout = 20);

      /**
       * Destructor. Will disconnect from the ldapserver if stll connected.
       */
      ~LdapQuery();

      /**
       * Scope for a LDAP queries. Use when querying.
       */
      enum Scope { base, onelevel, subtree };

      /**
       * Queries the ldap server.
       */
      void Query(const std::string& base,
                 const std::string& filter = "(objectclass=*)",
                 const std::vector <std::string>& attributes =
                 std::vector<std::string>(),
                 Scope scope = subtree);

      /**
       * Retrieves the result of the query from the ldap-server.
       */
      void Result(ldap_callback callback,
                  void *ref);

      /**
       * Returns the hostname of the ldap-server.
       */
      std::string Host();

    private:
      void Connect();
      void SetConnectionOptions(int version);
      void HandleResult(ldap_callback callback, void *ref);
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
  };

  /** General method to perform parallel ldap-queries to a set of clusters */
  class ParallelLdapQueries {
    public:
      ParallelLdapQueries(std::list<Arc::URL> clusters,
                          std::string filter,
                          std::vector<std::string> attrs,
                          ldap_callback callback,
                          void* object,
                          LdapQuery::Scope scope = LdapQuery::subtree,
                          const std::string& usersn = "",
                          bool anonymous = true,
                          int timeout = 20);

      ~ParallelLdapQueries();

      void Query();

      static void* DoLdapQuery(void* arg);

    private:
      std::list<Arc::URL> clusters;
      std::string filter;
      std::vector<std::string> attrs;
      ldap_callback callback;
      void* object;
      LdapQuery::Scope scope;
      std::string usersn;
      bool anonymous;
      int timeout;

      std::list<Arc::URL>::iterator urlit;
      pthread_mutex_t lock;
  };

} // namespace gridftpd

#endif // ARCLIB_LDAPQUERY

