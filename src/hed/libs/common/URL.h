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
  /** A URL is constructed from a string representation and split 
    into protocol, hostname, port and path.  */
  class URL {

   public:
    /** Empty constructor. Necessary when the class is part of
     *  another class and the like.
     */
    URL();

    /** Constructs a new URL from a string representation. The string
     *	is split into protocol, hostname, port and path.
     */
    URL(const std::string& url);

    /** URL Destructor */
    virtual ~URL();

    /** Returns the protocol of the URL. */
    const std::string& Protocol() const;

    /** Returns the username of the URL. */
    const std::string& Username() const;

    /** Returns the password of the URL. */
    const std::string& Passwd() const;

    /** Returns the hostname of the URL. */
    const std::string& Host() const;

    /** Returns the port of the URL. */
    int Port() const;

    /** Returns the path of the URL. */
    const std::string& Path() const;

    /** In case of ldap-protocol, return the basedn of the URL. */
    std::string BaseDN() const;

    /** Returns HTTP options if any. */
    const std::map<std::string, std::string>& HTTPOptions() const;

    /** Returns the value of an HTTP option.
	\param option     The option whose value is returned.
	\param undefined  This value is returned if the HTTP option is
				not defined. */
    const std::string& HTTPOption(const std::string& option,
				  const std::string& undefined = "") const;

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

    /** http-options of the url. */
    std::map<std::string, std::string> httpoptions;

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
  /** It is specific for an RC or RLS registration. */
  class URLLocation : public URL {

   public:
    /** Creates a URL Location from a URL. */
    URLLocation(const std::string& url);

    /** Creates a URL Location from a name and an option string. */
    URLLocation(const std::string& name, const std::string& optstring);

    /** URL Location destructor. */
    virtual ~URLLocation();

    /** Returns the URL Location name (used for RC registrations). */
    const std::string& Name() const;

    /** Returns a string representation of the URL Location. */
    virtual std::string str() const;

    /** Returns a string representation including options and locations */
    virtual std::string fullstr() const;

   protected:
    /** the URL Location name (used for RC registrations). */
    std::string name;
  };

} // namespace Arc

#endif //  __ARC_URL_H__
