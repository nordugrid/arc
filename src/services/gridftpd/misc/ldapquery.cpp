#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ldap.h>
#ifdef HAVE_SASL_H
#include <signal.h>
#include <sasl.h>
#endif
#ifdef HAVE_SASL_SASL_H
#include <signal.h>
#include <sasl/sasl.h>
#endif
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include <arc/Thread.h>
#include <arc/StringConv.h>

#include "ldapquery.h"

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#define _(A) dgettext("arclib", (A))
#else
#define _(A) (A)
#endif


namespace gridftpd {

  static Arc::Logger logger(Arc::Logger::getRootLogger(),"LdapQuery");

  class sigpipe_ingore {
    public:
      sigpipe_ingore();
  };

  struct ldap_bind_arg {
    LDAP *connection;
    Arc::SimpleCondition cond;
    bool anonymous;
    std::string usersn;
    bool valid;
  };

  static void* ldap_bind_with_timeout(void* arg_);

  #if defined(HAVE_SASL_H) || defined(HAVE_SASL_SASL_H)
  class sasl_defaults {
    public:
      sasl_defaults (ldap *ld,
                     const std::string & mech,
                     const std::string & realm,
                     const std::string & authcid,
                     const std::string & authzid,
                     const std::string & passwd);
      ~sasl_defaults() {};

    private:
      std::string p_mech;
      std::string p_realm;
      std::string p_authcid;
      std::string p_authzid;
      std::string p_passwd;

    friend int my_sasl_interact(ldap *ld,
                                unsigned int flags,
                                void * defaults_,
                                void * interact_);
  };

  static sigpipe_ingore sigpipe_ingore;

  sigpipe_ingore::sigpipe_ingore() {
    signal(SIGPIPE,SIG_IGN);
  }

  sasl_defaults::sasl_defaults (ldap *ld,
                                const std::string & mech,
                                const std::string & realm,
                                const std::string & authcid,
                                const std::string & authzid,
                                const std::string & passwd) : p_mech    (mech),
                                                              p_realm   (realm),
                                                              p_authcid (authcid),
                                                              p_authzid (authzid),
                                                              p_passwd  (passwd) {

    if (p_mech.empty()) {
      char * temp;
      ldap_get_option (ld, LDAP_OPT_X_SASL_MECH, &temp);
      if (temp) {
        p_mech = temp;
        free (temp);
      }
    }
    if (p_realm.empty()) {
      char * temp;
      ldap_get_option (ld, LDAP_OPT_X_SASL_REALM, &temp);
      if (temp) {
        p_realm = temp;
        free (temp);
      }
    }
    if (p_authcid.empty()) {
      char * temp;
      ldap_get_option (ld, LDAP_OPT_X_SASL_AUTHCID, &temp);
      if (temp) {
        p_authcid = temp;
        free (temp);
      }
    }
    if (p_authzid.empty()) {
      char * temp;
      ldap_get_option (ld, LDAP_OPT_X_SASL_AUTHZID, &temp);
      if (temp) {
        p_authzid = temp;
        free (temp);
      }
    }
  }


  int my_sasl_interact(ldap* /* ld */,
                       unsigned int flags,
                       void * defaults_,
                       void * interact_) {

    sasl_interact_t * interact = (sasl_interact_t *) interact_;
    sasl_defaults *   defaults = (sasl_defaults *)   defaults_;

    if (flags == LDAP_SASL_INTERACTIVE) {
      logger.msg(Arc::VERBOSE, _("SASL Interaction"));
    }

    while (interact->id != SASL_CB_LIST_END) {

      bool noecho = false;
      bool challenge = false;
      bool use_default = false;

      switch (interact->id) {
      case SASL_CB_GETREALM:
        if (defaults && !defaults->p_realm.empty())
          interact->defresult = strdup (defaults->p_realm.c_str());
        break;
      case SASL_CB_AUTHNAME:
        if (defaults && !defaults->p_authcid.empty())
          interact->defresult = strdup (defaults->p_authcid.c_str());
        break;
      case SASL_CB_USER:
        if (defaults && !defaults->p_authzid.empty())
          interact->defresult = strdup (defaults->p_authzid.c_str());
        break;
      case SASL_CB_PASS:
        if (defaults && !defaults->p_passwd.empty())
          interact->defresult = strdup (defaults->p_passwd.c_str());
        noecho = true;
        break;
      case SASL_CB_NOECHOPROMPT:
        noecho = true;
        challenge = true;
        break;
      case SASL_CB_ECHOPROMPT:
        challenge = true;
        break;
      }

      if (flags != LDAP_SASL_INTERACTIVE &&
         (interact->defresult || interact->id == SASL_CB_USER)) {
        use_default = true;
      }
      else {
        if (flags == LDAP_SASL_QUIET) return 1;

        if (challenge && interact->challenge)
          logger.msg(Arc::VERBOSE, "%s: %s", _("Challenge"), interact->challenge);

        if (interact->defresult)
          logger.msg(Arc::VERBOSE, "%s: %s", _("Default"), interact->defresult);

        std::string prompt;
        std::string input;

        prompt = interact->prompt ?
                 std::string (interact->prompt) + ": " : "Interact: ";

        if (noecho) {
          input = getpass (prompt.c_str());
        }
        else {
          std::cout << prompt;
          std::cin >> input;
        }
        if (input.empty())
          use_default = true;
        else {
          interact->result = strdup (input.c_str());
          interact->len = input.length();
        }
      }

      if (use_default) {
        interact->result = strdup (interact->defresult ?
        interact->defresult : "");
        interact->len = strlen ((char *) interact->result);
      }

      if (defaults && interact->id == SASL_CB_PASS) {
        // clear default password after first use
        defaults->p_passwd = "";
      }

      interact++;
    }
    return 0;
  }
  #endif


  LdapQuery::LdapQuery(const std::string& ldaphost,
                       int ldapport,
                       bool anonymous,
                       const std::string& usersn,
                       int timeout) : host(ldaphost),
                                      port(ldapport),
                                      anonymous(anonymous),
                                      usersn(usersn),
                                      timeout(timeout),
                                      connection(NULL),
                                      messageid(0) {}


  LdapQuery::~LdapQuery() {

    if (connection) {
      ldap_unbind_ext (connection, NULL, NULL);
      connection = NULL;
    }
  }


  void LdapQuery::Connect() {

    const int version = LDAP_VERSION3;

    logger.msg(Arc::VERBOSE, "%s: %s:%i", _("LdapQuery: Initializing connection to"), host, port);

    if (connection)
      throw LdapQueryError(
        _("Ldap connection already open to") + (" " + host));

    ldap_initialize(&connection,
                    ("ldap://" + host + ':' + Arc::tostring(port)).c_str());

    if (!connection)
      throw LdapQueryError(
        _("Could not open ldap connection to") + (" " + host));

    try {
      SetConnectionOptions(version);
    }
    catch (LdapQueryError& e) {
      // Clean up and re-throw exception
      ldap_unbind_ext (connection, NULL, NULL);
      connection = NULL;
      throw;
    }

    ldap_bind_arg arg;

    arg.connection = connection;
    arg.anonymous = anonymous;
    arg.usersn = usersn;
    arg.valid = false;

    pthread_t thr;
    if (pthread_create(&thr, NULL, &ldap_bind_with_timeout, &arg) != 0) {
      ldap_unbind_ext (connection, NULL, NULL);
      connection = NULL;
      throw LdapQueryError(
        _("Failed to create ldap bind thread") + (" (" + host + ")"));
    }

    if (!arg.cond.wait(1000 * (timeout + 1))) {
      pthread_cancel (thr);
      pthread_detach (thr);
      // if bind fails unbind will fail too - so don't call it
      connection = NULL;
      throw LdapQueryError(
        _("Ldap bind timeout") + (" (" + host + ")"));
    }

    pthread_join (thr, NULL);

    if (!arg.valid) {
      ldap_unbind_ext (connection, NULL, NULL);
      connection = NULL;
      throw LdapQueryError(
        _("Failed to bind to ldap server") + (" (" + host + ")"));
    };
  }


  void LdapQuery::SetConnectionOptions(int version) {

    timeval tout;
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    if (ldap_set_option (connection, LDAP_OPT_NETWORK_TIMEOUT, &tout) !=
        LDAP_OPT_SUCCESS)
      throw LdapQueryError(
        _("Could not set ldap network timeout") + (" (" + host + ")"));

    if (ldap_set_option (connection, LDAP_OPT_TIMELIMIT, &timeout) !=
        LDAP_OPT_SUCCESS)
      throw LdapQueryError(
        _("Could not set ldap timelimit") + (" (" + host + ")"));

    if (ldap_set_option (connection, LDAP_OPT_PROTOCOL_VERSION, &version) !=
        LDAP_OPT_SUCCESS)
      throw LdapQueryError(
        _("Could not set ldap protocol version") + (" (" + host + ")"));
  }


  static void* ldap_bind_with_timeout(void* arg_) {

    ldap_bind_arg* arg = (ldap_bind_arg* ) arg_;

    int ldresult = 0;
    if (arg->anonymous) {
      BerValue cred = { 0, (char*)"" };
      ldresult = ldap_sasl_bind_s (arg->connection, NULL, LDAP_SASL_SIMPLE,
                                   &cred, NULL, NULL, NULL);
    }
    else {
  #if defined(HAVE_SASL_H) || defined(HAVE_SASL_SASL_H)
      int ldapflag = LDAP_SASL_QUIET;
      if (logger.getThreshold() <= Arc::VERBOSE)
        ldapflag = LDAP_SASL_AUTOMATIC;
      sasl_defaults defaults = sasl_defaults (arg->connection,
                                              SASLMECH,
                                              "",
                                              "",
                                              arg->usersn,
                                              "");
      ldresult = ldap_sasl_interactive_bind_s (arg->connection,
                                               NULL,
                                               SASLMECH,
                                               NULL,
                                               NULL,
                                               ldapflag,
                                               my_sasl_interact,
                                               &defaults);
  #else
      BerValue cred = { 0, (char*)"" };
      ldresult = ldap_sasl_bind_s (arg->connection, NULL, LDAP_SASL_SIMPLE,
                                   &cred, NULL, NULL, NULL);
  #endif
    }

    if (ldresult != LDAP_SUCCESS) {
      arg->valid = false;
      arg->cond.signal();
    }
    else {
      arg->valid = true;
      arg->cond.signal();
    }

    return NULL;
  }


  void LdapQuery::Query(const std::string& base,
                        const std::string& filter,
                        const std::vector <std::string>& attributes,
                        Scope scope) {

    Connect();

    logger.msg(Arc::VERBOSE, "%s %s", _("LdapQuery: Querying"), host);

    logger.msg(Arc::VERBOSE, "%s: %s", _("base dn"), base);
    if (!filter.empty())
      logger.msg(Arc::VERBOSE, "  %s: %s", _("filter"), filter);
    if (!attributes.empty()) {
      logger.msg(Arc::VERBOSE, "  %s:", _("attributes"));
      for (std::vector<std::string>::const_iterator vs = attributes.begin();
           vs != attributes.end(); vs++)
        logger.msg(Arc::VERBOSE, "    %s", *vs);
    }

    timeval tout;
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    char *filt = (char *) filter.c_str();

    char ** attrs;
    if (attributes.empty())
      attrs = NULL;
    else {
      attrs = new char * [attributes.size() + 1];
      int i = 0;
      for (std::vector<std::string>::const_iterator vs = attributes.begin();
           vs != attributes.end(); vs++, i++)
        attrs [i] = (char *) vs->c_str();
      attrs [i] = NULL;
    }

    int ldresult = ldap_search_ext (connection,
                                    base.c_str(),
                                    scope,
                                    filt,
                                    attrs,
                                    0,
                                    NULL,
                                    NULL,
                                    &tout,
                                    0,
                                    &messageid);

    if (attrs)
      delete[] attrs;

    if (ldresult != LDAP_SUCCESS) {
      std::string error_msg(ldap_err2string (ldresult));
      error_msg += " (" + host + ")";
      ldap_unbind_ext (connection, NULL, NULL);
      connection = NULL;
      throw LdapQueryError(error_msg);
    }
  }


  void LdapQuery::Result(ldap_callback callback, void* ref) {

    try {
      HandleResult(callback, ref);
    } catch (LdapQueryError& e) {
      // Clean up and re-throw exception
      ldap_unbind_ext (connection, NULL, NULL);
      connection = NULL;
      messageid = 0;
      throw;
    }
    // Since C++ doesnt have finally(), here we are again
    ldap_unbind_ext (connection, NULL, NULL);
    connection = NULL;
    messageid = 0;

    return;
  }


  void LdapQuery::HandleResult(ldap_callback callback, void* ref) {

    logger.msg(Arc::VERBOSE, "%s %s", _("LdapQuery: Getting results from"), host);

    if (!messageid)
      throw LdapQueryError(
        _("Error: no ldap query started to") + (" " + host));

    timeval tout;
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    bool done = false;
    int ldresult = 0;
    LDAPMessage * res = NULL;

    while (!done && (ldresult = ldap_result (connection,
                                             messageid,
                                             LDAP_MSG_ONE,
                                             &tout,
                                             &res)) > 0) {
      for (LDAPMessage * msg = ldap_first_message (connection, res); msg;
           msg = ldap_next_message (connection, msg)) {

        switch (ldap_msgtype(msg)) {
        case LDAP_RES_SEARCH_ENTRY:
          HandleSearchEntry(msg, callback, ref);
        break;
        case LDAP_RES_SEARCH_RESULT:
          done = true;
        break;
        } // switch
      } // for
      ldap_msgfree (res);
    }

    if (ldresult == 0)
      throw LdapQueryError(_("Ldap query timed out") + (": " + host));

    if (ldresult == -1) {
      std::string error_msg(ldap_err2string (ldresult));
      error_msg += " (" + host + ")";
      throw LdapQueryError(error_msg);
    }

    return;
  }


  void LdapQuery::HandleSearchEntry(LDAPMessage* msg,
                                    ldap_callback callback,
                                    void* ref) {
    char *dn = ldap_get_dn(connection, msg);
    callback("dn", dn, ref);
    if (dn) ldap_memfree(dn);

    BerElement *ber = NULL;
    for (char *attr = ldap_first_attribute (connection, msg, &ber);
         attr; attr = ldap_next_attribute (connection, msg, ber)) {
      BerValue **bval;
      if ((bval = ldap_get_values_len (connection, msg, attr))) {
        for (int i = 0; bval[i]; i++) {
          callback (attr,	(bval[i]->bv_val ? bval[i]->bv_val : ""), ref);
        }
        ber_bvecfree(bval);
      }
      ldap_memfree(attr);
    }
    if (ber)
      ber_free(ber, 0);

  }


  std::string LdapQuery::Host() {
    return host;
  }


  ParallelLdapQueries::ParallelLdapQueries(std::list<Arc::URL> clusters,
                                           std::string filter,
                                           std::vector<std::string> attrs,
                                           ldap_callback callback,
                                           void* object,
                                           LdapQuery::Scope scope,
                                           const std::string& usersn,
                                           bool anonymous,
                                           int timeout) :
      clusters(clusters),
      filter(filter),
      attrs(attrs),
      callback(callback),
      object(object),
      scope(scope),
      usersn(usersn),
      anonymous(anonymous),
      timeout(timeout) {
    urlit = this->clusters.begin();
    pthread_mutex_init(&lock, NULL);
  }


  ParallelLdapQueries::~ParallelLdapQueries() {
    pthread_mutex_destroy(&lock);
  }


  void ParallelLdapQueries::Query() {
    const int numqueries = clusters.size();
    pthread_t* threads = new pthread_t[numqueries];
    int res;

    for (unsigned int i = 0; i<clusters.size(); i++) {
      res = pthread_create(&threads[i],
                           NULL,
                           ParallelLdapQueries::DoLdapQuery,
                           (void*)this);
      if (res!=0) {
        delete[] threads;
        throw LdapQueryError(
          _("Thread creation in ParallelLdapQueries failed"));
      }
    }

    void* result;
    for (unsigned int i = 0; i<clusters.size(); i++) {
      res = pthread_join(threads[i], &result);
      if (res!=0) {
        delete[] threads;
        throw LdapQueryError(
          _("Thread joining in ParallelLdapQueries failed"));
      }
    }
    delete[] threads;
  }


  void* ParallelLdapQueries::DoLdapQuery(void* arg) {
    ParallelLdapQueries* plq = (ParallelLdapQueries*)arg;

    pthread_mutex_lock(&plq->lock);
    Arc::URL qurl = *(plq->urlit);
    plq->urlit++;
    pthread_mutex_unlock(&plq->lock);

    LdapQuery ldapq(qurl.Host(),
                    qurl.Port(),
                    plq->anonymous,
                    plq->usersn,
                    plq->timeout);

    try {
      pthread_mutex_lock(&plq->lock);
      ldapq.Query(qurl.Path(), plq->filter, plq->attrs, plq->scope); /* is Path() correct here to replace BaseDN() ?? */
      pthread_mutex_unlock(&plq->lock);
    } catch (LdapQueryError& e) {
      pthread_mutex_unlock(&plq->lock);
      logger.msg(Arc::VERBOSE, "%s: %s", _("Warning"), e.what());
      pthread_exit(NULL);
    }

    pthread_mutex_lock(&plq->lock);

    try {
      ldapq.Result(plq->callback, plq->object);
    } catch (LdapQueryError& e) {
      logger.msg(Arc::VERBOSE, "%s: %s", _("Warning"), e.what());
    }

    pthread_mutex_unlock(&plq->lock);
    pthread_exit(NULL);
  }

} // namespace gridftpd
