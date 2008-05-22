#ifndef __ARC_URL_H__
#define __ARC_URL_H__

#include <iostream>
#include <list>
#include <map>
#include <string>


/** Default ports for different protocols */
#define RC_DEFAULT_PORT 389
#define RLS_DEFAULT_PORT 39281
#define HTTP_DEFAULT_PORT 80
#define HTTPS_DEFAULT_PORT 443
#define HTTPG_DEFAULT_PORT 8443
#define SRM_DEFAULT_PORT 8443
#define LDAP_DEFAULT_PORT 389
#define FTP_DEFAULT_PORT 21
#define GSIFTP_DEFAULT_PORT 2811
#define LFC_DEFAULT_PORT 5010


namespace Arc {


  class URLLocation;


  /// Class to hold general URL's.
  /** The URL is split into protocol, hostname, port and path. */
  class URL {

   public:
    /** Empty constructor. Necessary when the class is part of
        another class and the like. */
    URL();

    /** Constructs a new URL from a string representation. */
    URL(const std::string& url);

    /** URL Destructor */
    virtual ~URL();

    /** Scope for LDAP URLs */
    enum Scope { base, onelevel, subtree };

    /** Returns the protocol of the URL. */
    const std::string& Protocol() const;

    /** Changes the protocol of the URL. */
    void ChangeProtocol(const std::string& newprot);

    /** Returns the username of the URL. */
    const std::string& Username() const;

    /** Returns the password of the URL. */
    const std::string& Passwd() const;

    /** Returns the hostname of the URL. */
    const std::string& Host() const;

    /** Changes the hostname of the URL. */
    void ChangeHost(const std::string& newhost);

    /** Returns the port of the URL. */
    int Port() const;

    /** Changes the port of the URL. */
    void ChangePort(int newport);

    /** Returns the path of the URL. */
    const std::string& Path() const;

    /** Changes the path of the URL. */
    void ChangePath(const std::string& newpath);

    /** Returns HTTP options if any. */
    const std::map<std::string, std::string>& HTTPOptions() const;

    /** Returns the value of an HTTP option.
	\param option     The option whose value is returned.
	\param undefined  This value is returned if the HTTP option is
				not defined. */
    const std::string& HTTPOption(const std::string& option,
                                  const std::string& undefined = "") const;

    /** Returns the LDAP attributes if any. */
    const std::list<std::string>& LDAPAttributes() const;

    /** Adds an LDAP attribute. */
    void AddLDAPAttribute(const std::string& attribute);

    /** Returns the LDAP scope. */
    Scope LDAPScope() const;

    /** Changes the LDAP scope. */
    void ChangeLDAPScope(const Scope newscope);

    /** Returns the LDAP filter. */
    const std::string& LDAPFilter() const;

    /** Changes the LDAP filter. */
    void ChangeLDAPFilter(const std::string& newfilter);

    /** Returns URL options if any. */
    const std::map<std::string, std::string>& Options() const;

    /** Returns the value of a URL option.
	\param option     The option whose value is returned.
	\param undefined  This value is returned if the URL option is
				not defined. */
    const std::string& Option(const std::string& option,
                              const std::string& undefined = "") const;

    /** Adds a URL option. */
    void AddOption(const std::string& option, const std::string& value,
                   bool overwrite = true);

    /** Returns the locations if any. */
    const std::list<URLLocation>& Locations() const;

    /** Returns the common location options if any. */
    const std::map<std::string, std::string>& CommonLocOptions() const;

    /** Returns the value of a common location option.
	\param option     The option whose value is returned.
	\param undefined  This value is returned if the common location
				option is not defined. */
    const std::string& CommonLocOption(const std::string& option,
                                       const std::string&
                                         undefined = "") const;

    /** Returns a string representation of the URL. */
    virtual std::string str() const;

    /** Returns a string representation including options and locations */
    virtual std::string fullstr() const;

    /** Returns a string representation with protocol, host and port only */
    virtual std::string ConnectionURL() const;

    /** Compares one URL to another */
    bool operator<(const URL& url) const;

    /** Is one URL equal to another? */
    bool operator==(const URL& url) const;

    /** Check if instance holds valid URL */
    operator bool() const;
    bool operator!() const;

   protected:
    /** the url protocol. */
    std::string protocol;

    /** username of the url. */
    std::string username;

    /** password of the url. */
    std::string passwd;

    /** hostname of the url. */
    std::string host;

    /** portnumber of the url. */
    int port;

    /** the url path. */
    std::string path;

    /** HTTP options of the url. */
    std::map<std::string, std::string> httpoptions;

    /** LDAP attributes of the url. */
    std::list<std::string> ldapattributes;

    /** LDAP scope of the url. */
    Scope ldapscope;

    /** LDAP filter of the url. */
    std::string ldapfilter;

    /** options of the url. */
    std::map<std::string, std::string> urloptions;

    /** locations for index server URLs. */
    std::list<URLLocation> locations;

    /** common location options for index server URLs. */
    std::map<std::string, std::string> commonlocoptions;

    /** a private method that converts an ldap basedn to a path. */
    static std::string BaseDN2Path(const std::string&);

    /** a private method that converts an ldap path to a basedn. */
    static std::string Path2BaseDN(const std::string&);

    /** Overloaded operator << to print a URL. */
    friend std::ostream& operator<<(std::ostream& out, const URL& u);
  };


  /// Class to hold a resolved URL location.
  /** It is specific to file indexing service registrations. */
  class URLLocation : public URL {

   public:
    /** Creates a URLLocation from a string representaion. */
    URLLocation(const std::string& url);

    /** Creates a URLLocation from a string representaion and a name. */
    URLLocation(const std::string& url, const std::string& name);

    /** Creates a URLLocation from a URL. */
    URLLocation(const URL& url);

    /** Creates a URLLocation from a URL and a name. */
    URLLocation(const URL& url, const std::string& name);

    /** Creates a URLLocation from options and a name. */
    URLLocation(const std::map<std::string, std::string>& options,
                const std::string& name);

    /** URLLocation destructor. */
    virtual ~URLLocation();

    /** Returns the URLLocation name. */
    const std::string& Name() const;

    /** Returns a string representation of the URLLocation. */
    virtual std::string str() const;

    /** Returns a string representation including options and locations */
    virtual std::string fullstr() const;

   protected:
    /** the URLLocation name as registered in the indexing service. */
    std::string name;
  };


  /// Reads a list of URLs from a file
  std::list<URL> ReadURLList(const URL& urllist);

} // namespace Arc

#endif //  __ARC_URL_H__
