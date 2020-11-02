// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_URL_H__
#define __ARC_URL_H__

#include <iostream>
#include <list>
#include <map>
#include <string>


// Default ports for different protocols
#define RC_DEFAULT_PORT 389
#define HTTP_DEFAULT_PORT 80
#define HTTPS_DEFAULT_PORT 443
#define HTTPG_DEFAULT_PORT 8443
#define LDAP_DEFAULT_PORT 389
#define FTP_DEFAULT_PORT 21
#define GSIFTP_DEFAULT_PORT 2811
#define LFC_DEFAULT_PORT 5010
#define XROOTD_DEFAULT_PORT 1094
#define S3_DEFAULT_PORT 80
#define S3_HTTPS_DEFAULT_PORT 443


namespace Arc {

  class URLLocation;

  /// Class to represent general URLs.
  /** The URL is split into protocol, hostname, port and path. This class tries
   *  to follow RFC 3986 for splitting URLs, at least for protocol + host part.
   *  It also accepts local file paths which are converted to file:path.
   *  The usual system dependent file paths are supported. Relative paths are
   *  converted to absolute paths by prepending them with current working
   *  directory path. A file path can't start from # symbol. If the string
   *  representation of URL starts from '@' then it is treated as path to a
   *  file containing a list of URLs.
   *
   *  A URL is parsed in the following way:
\verbatim
 [protocol:][//[username:passwd@][host][:port]][;urloptions[;...]][/path[?httpoption[&...]][:metadataoption[:...]]]
\endverbatim
   *  The 'protocol' and 'host' parts are treated as case-insensitive and
   *  to avoid confusion are converted to lowercase in constructor. Note that
   *  'path' is always converted to absolute path in the constructor. The meaning
   *  of 'absolute' may depend upon URL type. For generic URL and local POSIX
   *  file paths that means the path starts from / like
\verbatim
 /path/to/file
\endverbatim
   *  For Windows paths the absolute path may look like
\verbatim
 C:\path\to\file
\endverbatim
   *  It is important to note that path still can be empty. For referencing
   *  a local file using an absolute path on a POSIX filesystem one may use either
\verbatim
 file:///path/to/file or file:/path/to/file
\endverbatim
   *  The relative path will look like
\verbatim
 file:to/file
\endverbatim
   *  For local Windows files possible URLs are
\verbatim
 %file:C:\path\to\file or %file:to\file
\endverbatim
   *  URLs representing LDAP resources have a different structure of options
   *  following the 'path' part:
\verbatim
 ldap://host[:port][;urloptions[;...]][/path[?attributes[?scope[?filter]]]]
\endverbatim
   *  For LDAP URLs paths are converted from /key1=value1/.../keyN=valueN
   *  notation to keyN=valueN,...,key1=value1 and hence path does not contain a
   *  leading /. If an LDAP URL initially had its path in the second notation,
   *  the leading / is treated as a separator only and is stripped.
   *
   *  URLs of indexing services optionally may have locations specified
   *  before the 'host' part
\verbatim
 protocol://[location[;location[;...]]@][host][:port]...
\endverbatim
   *  The structure of the 'location' element is protocol specific.
   *  \ingroup common
   *  \headerfile URL.h arc/URL.h
   */
  class URL {

  public:
    /// Empty constructor. URL object is invalid.
    URL();

    /// Constructs a new URL from a string representation.
    /**
     * \param url      The string representation of URL
     * \param encoded  Set to true if URL is encoded according to RFC 3986
     * \param defaultPort Port to use if 'url' doesn't specify port
     * \param defaultPath Path to use if 'url' doesn't specify path
     * \since Changed in 4.1.0. defaultPort and defaultPath arguments added.
     **/
    URL(const std::string& url, bool encoded = false, int defaultPort = -1, const std::string& defaultPath = "");

    /// Empty destructor.
    virtual ~URL();

    /// Scope for LDAP URLs.
    enum Scope {
      base, onelevel, subtree
    };

    /// Perform decoding of stored URL parts according to RFC 3986
    /** This method is supposed to be used only if for some reason
       URL constructor was called with encoded=false for URL which
       was encoded. Use it only once. */
    void URIDecode(void);

    /// Returns the protocol of the URL.
    const std::string& Protocol() const;

    /// Changes the protocol of the URL.
    void ChangeProtocol(const std::string& newprot);

    /// Returns the username of the URL.
    const std::string& Username() const;

    /// Returns the password of the URL.
    const std::string& Passwd() const;

    /// Returns the hostname of the URL.
    const std::string& Host() const;

    /// Changes the hostname of the URL.
    void ChangeHost(const std::string& newhost);

    /// Returns the port of the URL.
    int Port() const;

    /// Changes the port of the URL.
    void ChangePort(int newport);

    /// Returns the path of the URL.
    const std::string& Path() const;

    /// Returns the path of the URL with all options attached.
    std::string FullPath() const;

    /// Returns the path and all options, URI-encoded according to RFC 3986.
    /** Forward slashes ('/') in the path are not encoded but are encoded in
     *  the options. */
    std::string FullPathURIEncoded() const;

    /// Changes the path of the URL.
    void ChangePath(const std::string& newpath);

    /// Changes the path of the URL and all options attached.
    void ChangeFullPath(const std::string& newpath, bool encoded = false);

    /// Changes whole URL possibly taking into account current one if new URL is relative.
    void ChangeURL(const std::string& newurl, bool encoded = false);

    /// Returns HTTP options if any.
    const std::map<std::string, std::string>& HTTPOptions() const;

    /// Returns the value of an HTTP option.
    /**  \param option     The option whose value is returned.
     *   \param undefined  This value is returned if the HTTP option is
     *                     not defined.
     */
    const std::string& HTTPOption(const std::string& option,
                                  const std::string& undefined = "") const;

    /// Adds a HTP option with the given value.
    /** \return false if overwrite is false and option already exists, true
     *   otherwise. */
    bool AddHTTPOption(const std::string& option, const std::string& value,
                   bool overwrite = true);

    /// Removes a HTTP option if exists.
    /** \param option     The option to remove. */
    void RemoveHTTPOption(const std::string& option);

    /// Returns the LDAP attributes if any.
    const std::list<std::string>& LDAPAttributes() const;

    /// Adds an LDAP attribute.
    void AddLDAPAttribute(const std::string& attribute);

    /// Returns the LDAP scope.
    Scope LDAPScope() const;

    /// Changes the LDAP scope.
    void ChangeLDAPScope(const Scope newscope);

    /// Returns the LDAP filter.
    const std::string& LDAPFilter() const;

    /// Changes the LDAP filter.
    void ChangeLDAPFilter(const std::string& newfilter);

    /// Returns URL options if any.
    const std::map<std::string, std::string>& Options() const;

    /// Returns the value of a URL option.
    /** \param option     The option whose value is returned.
     *  \param undefined  This value is returned if the URL option is
     *                    not defined.
     */
    const std::string& Option(const std::string& option,
                              const std::string& undefined = "") const;

    /// Returns metadata options if any.
    const std::map<std::string, std::string>& MetaDataOptions() const;

    /// Returns the value of a metadata option.
    /** \param option     The option whose value is returned.
     *  \param undefined  This value is returned if the metadata option is
     *  not defined.
     */
    const std::string& MetaDataOption(const std::string& option,
                                      const std::string& undefined = "") const;

    /// Adds a URL option with the given value.
    /** Note that some compilers may interpret AddOption("name", "value") as a
     *  call to AddOption(const std::string&, bool) so it is recommended to use
     *  explicit string types when calling this method.
     *  \return false if overwrite is false and option already exists, true
     *   otherwise.
     */
    bool AddOption(const std::string& option, const std::string& value,
                   bool overwrite = true);

    /// Adds a URL option where option has the format "name=value".
    /** \return false if overwrite is true and option already exists or if
     *  option does not have the correct format. Returns true otherwise.
     */
    bool AddOption(const std::string& option, bool overwrite = true);

    /// Adds a metadata option.
    void AddMetaDataOption(const std::string& option, const std::string& value,
                           bool overwrite = true);

    /// Adds a Location.
    void AddLocation(const URLLocation& location);

    /// Returns the locations if any.
    const std::list<URLLocation>& Locations() const;

    /// Returns the common location options if any.
    const std::map<std::string, std::string>& CommonLocOptions() const;

    /// Returns the value of a common location option.
    /** \param option     The option whose value is returned.
     *  \param undefined  This value is returned if the common location
     *                           option is not defined.
     */
    const std::string& CommonLocOption(const std::string& option,
                                       const std::string&
                                       undefined = "") const;

    /// Removes a URL option if exists.
    /**  \param option     The option to remove. */
    void RemoveOption(const std::string& option);

    /// Remove a metadata option if exits.
    /**  \param option     The option to remove. */
    void RemoveMetaDataOption(const std::string& option);

    /// Returns a string representation of the URL including meta-options.
    virtual std::string str(bool encode = false) const;

    /// Returns a string representation of the URL without any options.
    virtual std::string plainstr(bool encode = false) const;

    /// Returns a string representation including options and locations.
    virtual std::string fullstr(bool encode = false) const;

    /// Returns a string representation with protocol, host and port only.
    virtual std::string ConnectionURL() const;

    /// Compares one URL to another.
    bool operator<(const URL& url) const;

    /// Is one URL equal to another?
    bool operator==(const URL& url) const;

    /// Check if instance holds valid URL.
    operator bool() const;
    /// Check if instance does not hold valid URL.
    bool operator!() const;

    /// Returns true if string matches url.
    bool StringMatches(const std::string& str) const;


    /// Parse a string of options separated by separator into an attribute->value map.
    std::map<std::string, std::string> ParseOptions(const std::string& optstring,
                                                    char separator, bool encoded = false);

    /// Returns a string representation of the options given in the options map.
    /** \param options Key-value map of options
     *  \param separator The character that separates options
     *  \param encode if set to true then options are encoded according to RFC 3986 */
    static std::string OptionString(const std::map<std::string,
                                    std::string>& options, char separator, bool encode = false);

    /// Perform encoding according to RFC 3986.
    /** This simply calls Arc::uri_encode(). */
    static std::string URIEncode(const std::string& str);

    /// Perform decoding according to RFC 3986.
    /** This simply calls Arc::uri_unencode(). */
    static std::string URIDecode(const std::string& str);

  protected:
    /// the url protocol.
    std::string protocol;

    /// username of the url.
    std::string username;

    /// password of the url.
    std::string passwd;

    /// hostname of the url.
    std::string host;

    /// if host is IPv6 numerical address notation.
    bool ip6addr;

    /// portnumber of the url.
    int port;

    /// the url path.
    std::string path;

    /// HTTP options of the url.
    std::map<std::string, std::string> httpoptions;

    /// Meta data options
    std::map<std::string, std::string> metadataoptions;

    /// LDAP attributes of the url.
    std::list<std::string> ldapattributes;

    /// LDAP scope of the url.
    Scope ldapscope;

    /// LDAP filter of the url.
    std::string ldapfilter;

    /// options of the url.
    std::map<std::string, std::string> urloptions;

    /// locations for index server URLs.
    std::list<URLLocation> locations;

    /// common location options for index server URLs.
    std::map<std::string, std::string> commonlocoptions;

    /// flag to describe validity of URL
    bool valid;

    /// a private method that converts an ldap basedn to a path.
    static std::string BaseDN2Path(const std::string&);

    /// a private method that converts an ldap path to a basedn.
    static std::string Path2BaseDN(const std::string&);

    /// Overloaded operator << to print a URL.
    friend std::ostream& operator<<(std::ostream& out, const URL& u);

    /// Convenience method for splitting schema specific part into path and options.
    void ParsePath(bool encoded = false);
  };


  /// Class to hold a resolved URL location.
  /** It is specific to file indexing service registrations.
   *  \ingroup common
   *  \headerfile URL.h arc/URL.h */
  class URLLocation
    : public URL {

  public:
    /// Creates a URLLocation from a string representation.
    URLLocation(const std::string& url = "");

    /// Creates a URLLocation from a string representation and a name.
    URLLocation(const std::string& url, const std::string& name);

    /// Creates a URLLocation from a URL.
    URLLocation(const URL& url);

    /// Creates a URLLocation from a URL and a name.
    URLLocation(const URL& url, const std::string& name);

    /// Creates a URLLocation from options and a name.
    URLLocation(const std::map<std::string, std::string>& options,
                const std::string& name);

    /// URLLocation destructor.
    virtual ~URLLocation();

    /// Returns the URLLocation name.
    const std::string& Name() const;

    /// Returns a string representation of the URLLocation.
    virtual std::string str(bool encode = false) const;

    /// Returns a string representation including options and locations
    virtual std::string fullstr(bool encode = false) const;

  protected:
    /// the URLLocation name as registered in the indexing service.
    std::string name;
  };


  /// Class to iterate through elements of a path.
  /** \ingroup common
   *  \headerfile URL.h arc/URL.h */
  class PathIterator {
  public:
    /// Constructor accepts path and stores it internally.
    /** If end is set to false iterator points at first element
     * in path. Otherwise selected element is one before last. */
    PathIterator(const std::string& path, bool end = false);
    ~PathIterator();

    /// Advances iterator to point at next path element
    PathIterator& operator++();

    /// Moves iterator to element before current
    PathIterator& operator--();

    /// Return false when iterator moved outside path elements
    operator bool() const;

    /// Returns part of initial path from first till and including current
    std::string operator*() const;

    /// Returns part of initial path from one after current till end
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
