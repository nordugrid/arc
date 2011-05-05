// -*- indent-tabs-mode: nil -*-

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
#define LDAP_DEFAULT_PORT 389
#define FTP_DEFAULT_PORT 21
#define GSIFTP_DEFAULT_PORT 2811
#define LFC_DEFAULT_PORT 5010
#define XROOTD_DEFAULT_PORT 1094


namespace Arc {

  class URLLocation;


  /// Class to hold general URL's.
  /** The URL is split into protocol, hostname, port and path.
     This class tries to follow RFC 3986 for spliting URLs at least
     for protocol + host part.
     It also accepts local file paths which are converted to file:path.
     Usual system dependant file paths are supported. Relative
     paths are converted to absolute ones by prepending them
     with current working directory path.
     File path can't start from # symbol (why?).
     If string representation of URL starts from '@' then it is
     treated as path to file containing list of URLs.
     Simple URL is parsed in following way:
        [protocol:][//[username:passwd@][host][:port]][;urloptions[;...]][/path[?httpoption[&...]][:metadataoption[:...]]]
     The 'protocol' and 'host' parts are treated as case-insensitive and
     to avoid confusion are converted to lowercase in constructor.
     Note that 'path' is always converted to absolute path in constructor.
     Meaning of 'absolute' may depend upon URL type. For generic URL and
     local POSIX file paths that means path starts from / like
       /path/to/file
     For Windows paths absolute path may look like
       C:\path\to\file
     It is important to note that path still can be empty.
     For referencing local file using absolute path on POSIX filesystem
     one may use either
       file:///path/to/file
     or
       file:/path/to/file
     Relative path will look like
       file:to/file
     For local Windows files possible URLs are
       file:C:\path\to\file
       file:to\file
     URLs representing LDAP resources have different structure of options
     following 'path' part
        ldap://host[:port][;urloptions[;...]][/path[?attributes[?scope[?filter]]]]
     For LDAP URLs paths are converted from /key1=value1/.../keyN=valueN
     notation to keyN=valueN,...,key1=value1 and hence path does not contain
     leading /. If LDAP URL initially had path in second notation leading
     / is treated as separator only and is stripped.
     URLs of indexing services optionally may have locations specified
     before 'host' part
        protocol://[location[;location[;...]]@][host][:port]...
     The structure of 'location' element is protocol specific.
  */
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
    enum Scope {
      base, onelevel, subtree
    };

    /** Returns the protocol of the URL. */
    const std::string& Protocol() const;

    /** Changes the protocol of the URL. */
    void ChangeProtocol(const std::string& newprot);

    /** Indicates whether the protocol is secure or not. */
    bool IsSecureProtocol() const;

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

    /** Returns the path of the URL with all options attached. */
    std::string FullPath() const;

    /** Changes the path of the URL. */
    void ChangePath(const std::string& newpath);

    /** Changes the path of the URL and all options attached. */
    void ChangeFullPath(const std::string& newpath);

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

    /** Returns metadata options if any. */
    const std::map<std::string, std::string>& MetaDataOptions() const;

    /** Returns the value of a metadata option.
       \param option     The option whose value is returned.
       \param undefined  This value is returned if the metadata option is
        not defined. */
    const std::string& MetaDataOption(const std::string& option,
                                      const std::string& undefined = "") const;

    /** Adds a URL option with the given value. Returns false if overwrite
     *  is false and option already exists, true otherwise. Note that some
     *  compilers may interpret AddOption("name", "value") as a call to
     *  AddOption(string, bool) so it is recommended to use explicit
     *  string types when calling this method. */
    bool AddOption(const std::string& option, const std::string& value,
                   bool overwrite = true);

    /** Adds a URL option where option has the format "name=value". Returns
     *  false if overwrite is true and option already exists or if option does
     *  not have the correct format. Returns true otherwise. */
    bool AddOption(const std::string& option, bool overwrite = true);

    /** Adds a metadata option */
    void AddMetaDataOption(const std::string& option, const std::string& value,
                           bool overwrite = true);

    /** Adds a Location */
    void AddLocation(const URLLocation& location);

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

    /** Returns a string representation of the URL including meta-options. */
    virtual std::string str() const;

    /** Returns a string representation of the URL without any options */
    virtual std::string plainstr() const;

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

    /// Returns true if string matches url
    bool StringMatches(const std::string& str) const;


    /** Parse a string of options separated by separator into an attribute->value map */
    std::map<std::string, std::string> ParseOptions(const std::string& optstring,
                                                    char separator);

    /** Returns a string representation of the options given in the options map */
    static std::string OptionString(const std::map<std::string,
                                                   std::string>& options, char separator);

  protected:
    /** the url protocol. */
    std::string protocol;

    /** username of the url. */
    std::string username;

    /** password of the url. */
    std::string passwd;

    /** hostname of the url. */
    std::string host;

    /** if host is IPv6 numerical address notation. */
    bool ip6addr;

    /** portnumber of the url. */
    int port;

    /** the url path. */
    std::string path;

    /** HTTP options of the url. */
    std::map<std::string, std::string> httpoptions;

    /** Meta data options */
    std::map<std::string, std::string> metadataoptions;

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

    /** flag to describe validity of URL */
    bool valid;

    /** a private method that converts an ldap basedn to a path. */
    static std::string BaseDN2Path(const std::string&);

    /** a private method that converts an ldap path to a basedn. */
    static std::string Path2BaseDN(const std::string&);

    /** Overloaded operator << to print a URL. */
    friend std::ostream& operator<<(std::ostream& out, const URL& u);

    /** Convenience method for spliting schema specific part into path and options */
    void ParsePath(void);
  };


  /// Class to hold a resolved URL location.
  /** It is specific to file indexing service registrations. */
  class URLLocation
    : public URL {

  public:
    /** Creates a URLLocation from a string representaion. */
    URLLocation(const std::string& url = "");

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


  /// Class to iterate through elements of path
  class PathIterator {
  public:
    /** Constructor accepts path and stores it internally. If
      end is set to false iterator is pointing at first element
      in path. Otherwise selected element is one before last. */
    PathIterator(const std::string& path, bool end = false);
    ~PathIterator();

    /** Advances iterator to point at next path element */
    PathIterator& operator++();

    /** Moves iterator to element before current */
    PathIterator& operator--();

    /** Return false when iterator moved outside path elements */
    operator bool() const;

    /** Returns part of initial path from first till and including current */
    std::string operator*() const;

    /** Returns part of initial path from one after current till end */
    std::string Rest() const;

  private:
    const std::string& path;
    std::string::size_type pos;
    bool end;
    bool done;
  };


  /// Reads a list of URLs from a file
  std::list<URL> ReadURLList(const URL& urllist);

} // namespace Arc

#endif //  __ARC_URL_H__
