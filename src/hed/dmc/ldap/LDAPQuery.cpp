#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <iostream>
#include <list>
#include <string>

#include <ldap.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef HAVE_SASL_H
#include <sasl.h>
#endif
#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#endif

#ifndef LDAP_SASL_QUIET
#define LDAP_SASL_QUIET 0  /* Does not exist in Solaris LDAP */
#endif

#ifndef LDAP_OPT_SUCCESS
#define LDAP_OPT_SUCCESS LDAP_SUCCESS
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>

#include "LDAPQuery.h"

namespace Arc {

  Logger LDAPQuery::logger(Logger::rootLogger, "LDAPQuery");

  struct ldap_bind_arg {
    LDAP *connection;
    LogLevel loglevel;
    SimpleCondition cond;
    bool valid;
    bool anonymous;
    std::string usersn;
  };

  static void *ldap_bind_with_timeout(void *arg);

#if defined (HAVE_SASL_H) || defined (HAVE_SASL_SASL_H)
  class sasl_defaults {
  public:
    sasl_defaults(ldap *ld,
		  const std::string& mech,
		  const std::string& realm,
		  const std::string& authcid,
		  const std::string& authzid,
		  const std::string& passwd);
    ~sasl_defaults() {}

  private:
    std::string p_mech;
    std::string p_realm;
    std::string p_authcid;
    std::string p_authzid;
    std::string p_passwd;

    friend int my_sasl_interact(ldap *ld,
				unsigned int flags,
				void *defaults_,
				void *interact_);
  };

  sasl_defaults::sasl_defaults(ldap *ld,
			       const std::string& mech,
			       const std::string& realm,
			       const std::string& authcid,
			       const std::string& authzid,
			       const std::string& passwd)
    : p_mech(mech),
      p_realm(realm),
      p_authcid(authcid),
      p_authzid(authzid),
      p_passwd(passwd) {

    if (p_mech.empty()) {
      char *temp;
      ldap_get_option(ld, LDAP_OPT_X_SASL_MECH, &temp);
      if (temp) {
	p_mech = temp;
	free(temp);
      }
    }
    if (p_realm.empty()) {
      char *temp;
      ldap_get_option(ld, LDAP_OPT_X_SASL_REALM, &temp);
      if (temp) {
	p_realm = temp;
	free(temp);
      }
    }
    if (p_authcid.empty()) {
      char *temp;
      ldap_get_option(ld, LDAP_OPT_X_SASL_AUTHCID, &temp);
      if (temp) {
	p_authcid = temp;
	free(temp);
      }
    }
    if (p_authzid.empty()) {
      char *temp;
      ldap_get_option(ld, LDAP_OPT_X_SASL_AUTHZID, &temp);
      if (temp) {
	p_authzid = temp;
	free(temp);
      }
    }
  }


  int my_sasl_interact(ldap *,
		       unsigned int flags,
		       void *defaults_,
		       void *interact_) {

    sasl_interact_t *interact = (sasl_interact_t *)interact_;
    sasl_defaults *defaults = (sasl_defaults *)defaults_;

    if (flags == LDAP_SASL_INTERACTIVE)
      LDAPQuery::logger.msg(VERBOSE, "SASL Interaction");

    while (interact->id != SASL_CB_LIST_END) {

      bool noecho = false;
      bool challenge = false;
      bool use_default = false;

      switch (interact->id) {
      case SASL_CB_GETREALM:
	if (defaults && !defaults->p_realm.empty())
	  interact->defresult = strdup(defaults->p_realm.c_str());
	break;

      case SASL_CB_AUTHNAME:
	if (defaults && !defaults->p_authcid.empty())
	  interact->defresult = strdup(defaults->p_authcid.c_str());
	break;

      case SASL_CB_USER:
	if (defaults && !defaults->p_authzid.empty())
	  interact->defresult = strdup(defaults->p_authzid.c_str());
	break;

      case SASL_CB_PASS:
	if (defaults && !defaults->p_passwd.empty())
	  interact->defresult = strdup(defaults->p_passwd.c_str());
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
	  (interact->defresult || interact->id == SASL_CB_USER))
	use_default = true;
      else {
	if (flags == LDAP_SASL_QUIET)
	  return 1;

	if (challenge && interact->challenge)
	  LDAPQuery::logger.msg(VERBOSE, "Challenge: %s",
				interact->challenge);

	if (interact->defresult)
	  LDAPQuery::logger.msg(VERBOSE, "Default: %s",
				interact->defresult);

	std::string prompt;
	std::string input;

	prompt = interact->prompt ?
		 std::string(interact->prompt) + ": " : "Interact: ";

	if (noecho)
	  input = getpass(prompt.c_str());
	else {
	  std::cout << prompt;
	  std::cin >> input;
	}
	if (input.empty())
	  use_default = true;
	else {
	  interact->result = strdup(input.c_str());
	  interact->len = input.length();
	}
      }

      if (use_default) {
	interact->result = strdup(interact->defresult ?
				  interact->defresult : "");
	interact->len = strlen((char *)interact->result);
      }

      if (defaults && interact->id == SASL_CB_PASS)
	// clear default password after first use
	defaults->p_passwd = "";

      interact++;
    }
    return 0;
  }
#endif


  LDAPQuery::LDAPQuery(const std::string& ldaphost,
		       int ldapport,
		       bool anonymous,
		       const std::string& usersn,
		       int timeout)
    : host(ldaphost),
      port(ldapport),
      anonymous(anonymous),
      usersn(usersn),
      timeout(timeout),
      connection(NULL),
      messageid(0) {}


  LDAPQuery::~LDAPQuery() {

    if (connection) {
      ldap_unbind_ext(connection, NULL, NULL);
      connection = NULL;
    }
  }


  bool LDAPQuery::Connect() {

    const int version = LDAP_VERSION3;

    logger.msg(DEBUG, "LDAPQuery: Initializing connection to %s:%d",
	       host, port);

    if (connection) {
      logger.msg(ERROR, "LDAP connection already open to %s", host);
      return false;
    }

#ifdef HAVE_LDAP_INITIALIZE
    ldap_initialize(&connection,
		    ("ldap://" + host + ':' + tostring(port)).c_str());
#else
    connection = ldap_init(host.c_str(), port);
#endif

    if (!connection) {
      logger.msg(ERROR, "Could not open LDAP connection to %s", host);
      return false;
    }

    if (!SetConnectionOptions(version)) {
      ldap_unbind_ext(connection, NULL, NULL);
      connection = NULL;
      return false;
    }

    ldap_bind_arg arg;

    arg.connection = connection;
    arg.loglevel = logger.getThreshold();
    arg.valid = true;
    arg.anonymous = anonymous;
    arg.usersn = usersn;

    pthread_t thr;
    if (pthread_create(&thr, NULL, &ldap_bind_with_timeout, &arg) != 0) {
      ldap_unbind_ext(connection, NULL, NULL);
      connection = NULL;
      logger.msg(ERROR, "Failed to create ldap bind thread (%s)", host);
      return false;
    }

    if (!arg.cond.wait(1000 * (timeout + 1))) {
      pthread_cancel(thr);
      pthread_detach(thr);
      // if bind fails unbind will fail too - so don't call it
      connection = NULL;
      logger.msg(ERROR, "Ldap bind timeout (%s)", host);
      return false;
    }

    pthread_join(thr, NULL);

    if (!arg.valid) {
      ldap_unbind_ext(connection, NULL, NULL);
      connection = NULL;
      logger.msg(ERROR, "Failed to bind to ldap server (%s)", host);
      return false;
    }

    return true;
  }


  bool LDAPQuery::SetConnectionOptions(int version) {

    timeval tout;
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    // solaris does not have LDAP_OPT_NETWORK_TIMEOUT
    if (ldap_set_option(connection, LDAP_OPT_NETWORK_TIMEOUT, &tout) !=
	LDAP_OPT_SUCCESS) {
      logger.msg(ERROR,
		 "Could not set LDAP network timeout (%s)", host);
      return false;
    }
#endif

    if (ldap_set_option(connection, LDAP_OPT_TIMELIMIT, &timeout) !=
	LDAP_OPT_SUCCESS) {
      logger.msg(ERROR,
		 "Could not set LDAP timelimit (%s)", host);
      return false;
    }

    if (ldap_set_option(connection, LDAP_OPT_PROTOCOL_VERSION, &version) !=
	LDAP_OPT_SUCCESS) {
      logger.msg(ERROR,
		 "Could not set LDAP protocol version (%s)", host);
      return false;
    }

    return true;
  }

  static void *ldap_bind_with_timeout(void *arg_) {

    ldap_bind_arg *arg = (ldap_bind_arg *)arg_;

    int ldresult = 0;
    if (arg->anonymous) {
      BerValue cred = {
	0, const_cast<char *>("")
      };
      ldresult = ldap_sasl_bind_s(arg->connection, NULL, LDAP_SASL_SIMPLE,
				  &cred, NULL, NULL, NULL);
    }
    else {
#if defined (HAVE_SASL_H) || defined (HAVE_SASL_SASL_H)
      int ldapflag = LDAP_SASL_QUIET;
#ifdef LDAP_SASL_AUTOMATIC
      // solaris does not have LDAP_SASL_AUTOMATIC
      if (arg->loglevel >= DEBUG)
	ldapflag = LDAP_SASL_AUTOMATIC;
#endif
      sasl_defaults defaults = sasl_defaults(arg->connection,
					     SASLMECH,
					     "",
					     "",
					     arg->usersn,
					     "");
      ldresult = ldap_sasl_interactive_bind_s(arg->connection,
					      NULL,
					      SASLMECH,
					      NULL,
					      NULL,
					      ldapflag,
					      my_sasl_interact,
					      &defaults);
#else
      BerValue cred = {
	0, const_cast<char *>("")
      };
      ldresult = ldap_sasl_bind_s(arg->connection, NULL, LDAP_SASL_SIMPLE,
				  &cred, NULL, NULL, NULL);
#endif
    }

    if (ldresult != LDAP_SUCCESS)
      arg->valid = false;
    else
      arg->valid = true;

    arg->cond.signal();
    return NULL;
  }


  bool LDAPQuery::Query(const std::string& base,
			const std::string& filter,
			const std::list<std::string>& attributes,
			URL::Scope scope) {

    if (!Connect())
      return false;

    logger.msg(DEBUG, "LDAPQuery: Querying %s", host);

    logger.msg(VERBOSE, "  base dn: %s", base);
    if (!filter.empty())
      logger.msg(VERBOSE, "  filter: %s", filter);
    if (!attributes.empty()) {
      logger.msg(VERBOSE, "  attributes:");
      for (std::list<std::string>::const_iterator vs = attributes.begin();
	   vs != attributes.end(); vs++)
	logger.msg(VERBOSE, "    %s", *vs);
    }

    timeval tout;
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    char *filt = (char *)filter.c_str();

    char **attrs;
    if (attributes.empty())
      attrs = NULL;
    else {
      attrs = new char *[attributes.size() + 1];
      int i = 0;
      for (std::list<std::string>::const_iterator vs = attributes.begin();
	   vs != attributes.end(); vs++, i++)
	attrs[i] = (char *)vs->c_str();
      attrs[i] = NULL;
    }

    int ldresult = ldap_search_ext(connection,
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
      logger.msg(ERROR, "%s (%s)", ldap_err2string(ldresult), host);
      ldap_unbind_ext(connection, NULL, NULL);
      connection = NULL;
      return false;
    }

    return true;
  }


  bool LDAPQuery::Result(ldap_callback callback, void *ref) {

    bool result = HandleResult(callback, ref);

    ldap_unbind_ext(connection, NULL, NULL);
    connection = NULL;
    messageid = 0;

    return result;
  }


  bool LDAPQuery::HandleResult(ldap_callback callback, void *ref) {

    logger.msg(DEBUG, "LDAPQuery: Getting results from %s", host);

    if (!messageid) {
      logger.msg(ERROR, "Error: no LDAP query started to %s", host);
      return false;
    }

    timeval tout;
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    bool done = false;
    int ldresult = 0;
    LDAPMessage *res = NULL;

    while (!done && (ldresult = ldap_result(connection,
					    messageid,
					    LDAP_MSG_ONE,
					    &tout,
					    &res)) > 0) {
      for (LDAPMessage *msg = ldap_first_message(connection, res); msg;
	   msg = ldap_next_message(connection, msg)) {

	switch (ldap_msgtype(msg)) {
	case LDAP_RES_SEARCH_ENTRY:
	  HandleSearchEntry(msg, callback, ref);
	  break;

	case LDAP_RES_SEARCH_RESULT:
	  done = true;
	  break;
	}                 // switch
      }           // for
      ldap_msgfree(res);
    }

    if (ldresult == 0) {
      logger.msg(ERROR, "LDAP query timed out: %s", host);
      return false;
    }

    if (ldresult == -1) {
      logger.msg(ERROR, "%s (%s)", ldap_err2string(ldresult), host);
      return false;
    }

    return true;
  }


  void LDAPQuery::HandleSearchEntry(LDAPMessage *msg,
				    ldap_callback callback,
				    void *ref) {
    char *dn = ldap_get_dn(connection, msg);
    callback("dn", dn, ref);
    if (dn)
      ldap_memfree(dn);

    BerElement *ber = NULL;
    for (char *attr = ldap_first_attribute(connection, msg, &ber);
	 attr; attr = ldap_next_attribute(connection, msg, ber)) {
      BerValue **bval;
      if ((bval = ldap_get_values_len(connection, msg, attr))) {
	for (int i = 0; bval[i]; i++)
	  callback(attr, (bval[i]->bv_val ? bval[i]->bv_val : ""), ref);
	ber_bvecfree(bval);
      }
      ldap_memfree(attr);
    }
    if (ber)
      ber_free(ber, 0);
  }

} // namespace Arc
