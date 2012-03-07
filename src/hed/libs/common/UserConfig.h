// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <list>
#include <vector>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/User.h>

namespace Arc {

  class Logger;
  class XMLNode;

  enum ServiceType {
    COMPUTING,
    INDEX
  };

  class ConfigEndpoint {
  public:
    enum Type { REGISTRY, COMPUTINGINFO, ANY };

    ConfigEndpoint(const std::string& URLString = "", const std::string& InterfaceName = "", ConfigEndpoint::Type type = ConfigEndpoint::ANY)
      : type(type), URLString(URLString), InterfaceName(InterfaceName) {}
    Type type;
    std::string URLString;
    std::string InterfaceName;
    std::string PreferredJobInterfaceName;

    operator bool() const {
      return (!URLString.empty());
    }

    bool operator!() const {
      return (!URLString.empty());
    }
  };

  typedef std::list<std::string> ServiceList[2];
  // Length should be number of service types

  std::string tostring(const ServiceType st);

  /// Defines how user credentials are looked for.
  /**
    * For complete information see description of
    *  UserConfig::InitializeCredentials(initializeCredentials)
    * method.
    **/
  class initializeCredentialsType {
   public:
    typedef enum {
      SkipCredentials,
      NotTryCredentials,
      TryCredentials,
      RequireCredentials,
      SkipCANotTryCredentials,
      SkipCATryCredentials,
      SkipCARequireCredentials
    } initializeType;
    initializeCredentialsType(void):val(SkipCATryCredentials) { };
    initializeCredentialsType(initializeType v):val(v) { };
    bool operator==(initializeType v) { return (val == v); };
    bool operator!=(initializeType v) { return (val != v); };
    operator initializeType(void) { return val; };
   private:
    initializeType val;
  };

  /// %User configuration class
  /**
   * This class provides a container for a selection of various
   * attributes/parameters which can be configured to needs of the user,
   * and can be read by implementing instances or programs. The class
   * can be used in two ways. One can create a object from a
   * configuration file, or simply set the desired attributes by using
   * the setter method, associated with every setable attribute. The
   * list of attributes which can be configured in this class are:
   * - certificatepath / CertificatePath(const std::string&)
   * - keypath / KeyPath(const std::string&)
   * - proxypath / ProxyPath(const std::string&)
   * - cacertificatesdirectory / CACertificatesDirectory(const std::string&)
   * - cacertificatepath / CACertificatePath(const std::string&)
   * - timeout / Timeout(int)
   * - joblist / JobListFile(const std::string&)
   * - defaultservices / AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
   * - rejectservices / AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
   * - verbosity / Verbosity(const std::string&)
   * - brokername / Broker(const std::string&) or Broker(const std::string&, const std::string&)
   * - brokerarguments / Broker(const std::string&) or Broker(const std::string&, const std::string&)
   * - bartender / Bartender(const std::list<URL>&)
   * - vomsserverpath / VOMSESPath(const std::string&)
   * - username / UserName(const std::string&)
   * - password / Password(const std::string&)
   * - keypassword / KeyPassword(const std::string&)
   * - keysize / KeySize(int)
   * - certificatelifetime / CertificateLifeTime(const Period&)
   * - slcs / SLCS(const URL&)
   * - storedirectory / StoreDirectory(const std::string&)
   * - jobdownloaddirectory / JobDownloadDirectory(const std::string&)
   * - idpname / IdPName(const std::string&)
   *
   * where the first term is the name of the attribute used in the
   * configuration file, and the second term is the associated setter
   * method (for more information about a given attribute see the
   * description of the setter method).
   *
   * The configuration file should have a INI-style format and the
   * IniConfig class will thus be used to parse the file. The above
   * mentioned attributes should be placed in the common section.
   * Another section is also valid in the configuration file, which is
   * the alias section. Here it is possible to define aliases
   * representing one or multiple services. These aliases can be used in
   * the AddServices(const std::list<std::string>&, ServiceType) and
   * AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
   * methods.
   *
   * The UserConfig class also provides a method InitializeCredentials()
   * for locating user credentials by searching in different standard
   * locations. The CredentialsFound() method can be used to test if
   * locating the credentials succeeded.
   **/
  class UserConfig {
  public:
    /// Create a UserConfig object
    /**
     * The UserConfig object created by this constructor initializes
     * only default values, and if specified by the
     * \a initializeCredentials boolean credentials will be tried
     * initialized using the InitializeCredentials() method. The object
     * is only non-valid if initialization of credentials fails which
     * can be checked with the #operator bool() method.
     *
     * @param initializeCredentials is a optional boolean indicating if
     *        the InitializeCredentials() method should be invoked, the
     *        default is \c true.
     * @see InitializeCredentials()
     * @see #operator bool()
     **/
    UserConfig(initializeCredentialsType initializeCredentials = initializeCredentialsType());
    /// Create a UserConfig object
    /**
     * The UserConfig object created by this constructor will, if
     * specified by the \a loadSysConfig boolean, first try to load the
     * system configuration file by invoking the LoadConfigurationFile()
     * method, and if this fails a ::WARNING is reported. Then the
     * configuration file passed will be tried loaded using the before
     * mentioned method, and if this fails an ::ERROR is reported, and
     * the created object will be non-valid. Note that if the passed
     * file path is empty the example configuration will be tried copied
     * to the default configuration file path specified by
     * DEFAULTCONFIG. If the example file cannot be copied one or more
     * ::WARNING messages will be reported and no configration will be
     * loaded. If loading the configurations file succeeded and if
     * \a initializeCredentials is \c true then credentials will be
     * initialized using the InitializeCredentials() method, and if no
     * valid credentials are found the created object will be non-valid.
     *
     * @param conffile is the path to a INI-configuration file.
     * @param initializeCredentials is a boolean indicating if
     *        credentials should be initialized, the default is \c true.
     * @param loadSysConfig is a boolean indicating if the system
     *        configuration file should be loaded aswell, the default is
     *        \c true.
     * @see LoadConfigurationFile(const std::string&, bool)
     * @see InitializeCredentials()
     * @see #operator bool()
     * @see SYSCONFIG
     * @see EXAMPLECONFIG
     **/
    UserConfig(const std::string& conffile,
               initializeCredentialsType initializeCredentials = initializeCredentialsType(),
               bool loadSysConfig = true);
    /// Create a UserConfig object
    /**
     * The UserConfig object created by this constructor does only
     * differ from the UserConfig(const std::string&, bool, bool)
     * constructor in that it is possible to pass the path of the job
     * list file directly to this constructor. If the job list file
     * \a joblistfile is empty, the behaviour of this constructor is
     * exactly the same as the before mentioned, otherwise the job list
     * file will be initilized by invoking the setter method
     * JobListFile(const std::string&). If it fails the created object
     *  will be non-valid, otherwise the specified configuration file
     * \a conffile will be loaded with the \a ignoreJobListFile argument
     * set to \c true.
     *
     * @param conffile is the path to a INI-configuration file
     * @param jfile is the path to a (non-)existing job list file.
     * @param initializeCredentials is a boolean indicating if
     *        credentials should be initialized, the default is \c true.
     * @param loadSysConfig is a boolean indicating if the system
     *        configuration file should be loaded aswell, the default is
     *        \c true.
     * @see JobListFile(const std::string&)
     * @see LoadConfigurationFile(const std::string&, bool)
     * @see InitializeCredentials()
     * @see #operator bool()
     **/
    UserConfig(const std::string& conffile,
               const std::string& jfile,
               initializeCredentialsType initializeCredentials = initializeCredentialsType(),
               bool loadSysConfig = true);
    /// Language binding constructor
    /**
     * The passed long int should be a pointer address to a UserConfig
     * object, and this address is then casted into this UserConfig
     * object.
     *
     * @param ptraddr is an memory address to a UserConfig object.
     **/
    UserConfig(const long int& ptraddr);
    ~UserConfig() {}

    /// Initialize user credentials.
    /**
     * The location of the user credentials will be tried located when
     * calling this method and stored internally when found. The method
     * searches in different locations.
     * Depending on value of initializeCredentials this method behaves
     * differently. Following is an explanation for RequireCredentials.
     * For less strict values see information below.
     * First the user proxy or the user key/certificate pair is tried
     * located in the following order:
     * - Proxy path specified by the environment variable
     *   X509_USER_PROXY. If value is set and corresponding file does
     *   not exist it considered to be an error and no other locations
     *   are tried.  If found no more proxy paths are tried.
     * - Current proxy path as passed to the contructor, explicitly set
     *   using the setter method ProxyPath(const std::string&) or read
     *   from configuration by constructor or LoadConfiguartionFile()
     *   method. If value is set and corresponding file does not exist
     *   it considered to be an error and no other locations are tried.
     *   If found no more proxy paths are tried.
     * - Proxy path made of x509up_u token concatenated with the user
     *   numerical ID located in the OS temporary directory. It is NOT
     *   an error if corresponding file does not exist and processing
     *   continues.
     * - Key/certificate paths specified by the environment variables
     *   X509_USER_KEY and X509_USER_CERT. If values  are set and
     *   corresponding files do not exist it considered to be an error
     *   and no other locations are tried. Error message is supressed
     *   if proxy was previously found.
     * - Current key/certificate paths passed to the contructor or
     *   explicitly set using the setter methods KeyPath(const std::string&)
     *   and CertificatePath(const std::string&) or read from configuration
     *   by constructor or LoadConfiguartionFile() method. If values
     *   are set and corresponding files do not exist it is an error
     *   and no other locations are tried. Error message is supressed
     *   if proxy was previously found.
     * - Key/certificate paths ~/.arc/usercert.pem and ~/.arc/userkey.pem
     *   respectively are tried. It is not an error if not found.
     * - Key/certificate paths ~/.globus/usercert.pem and ~/.globus/userkey.pem
     *   respectively are tried. It is not an error if not found.
     * - Key/certificate paths created by concatenation of ARC installation
     *   location and /etc/arc/usercert.pem and /etc/arc/userkey.pem
     *   respectively are tried. It is not an error if not found.
     * - Key/certificate located in current working directory are tried.
     * - If neither proxy nor key/certificate files are found this is
     *   considered to be an error.
     *
     * Along with the proxy and key/certificate pair, the path of the
     * directory containing CA certificates is also located. The presence
     * of directory will be checked in the following order and first
     * found is accepted:
     * - Path specified by the X509_CERT_DIR environment variable.
     *   It is an error if value is set and directory does not exist.
     * - Current path explicitly specified by using the setter method
     *   CACertificatesDirectory() or read from configuration by
     *   constructor or LoadConfiguartionFile() method. It is an error
     *   if value is set and directory does not exist.
     * - Path ~/.globus/certificates. It is not an error if it does
     *   not exist.
     * - Path created by concatenating the ARC installation location
     *   and /etc/certificates. It is not an error if it does not exist.
     * - Path created by concatenating the ARC installation location
     *   and /share/certificates. It is not an error if it does not exist.
     * - Path /etc/grid-security/certificates.
     *
     * It is an error if none of the directories above exist.
     *
     * In case of initializeCredentials == TryCredentials method behaves
     * same way like in case RequireCredentials except it does not report
     * errors through its Logger object and does not return false.
     *
     * If NotTryCredentials is used method does not check for presence of
     * credentials. It behaves like if corresponding files are always
     * present.
     *
     * And in case of SkipCredentials method does nothing.
     *
     * All options with SkipCA* prefix behaves similar to those without
     * prefix except the path of the directory containing CA certificates
     * is completely ignored.
     *
     * @see CredentialsFound()
     * @see ProxyPath(const std::string&)
     * @see KeyPath(const std::string&)
     * @see CertificatePath(const std::string&)
     * @see CACertificatesDirectory(const std::string&)
     **/
    bool InitializeCredentials(initializeCredentialsType initializeCredentials);
    /// Validate credential location
    /**
     * Valid credentials consists of a combination of a path
     * to existing CA-certificate directory and either a path to
     * existing proxy or a path to existing user key/certificate pair.
     * If valid credentials are found this method returns \c true,
     * otherwise \c false is returned.
     *
     * @return \c true if valid credentials are found, otherwise \c
     *         false.
     * @see InitializeCredentials()
     **/
    bool CredentialsFound() const {
      return !((proxyPath.empty() && (certificatePath.empty() || keyPath.empty())) || caCertificatesDirectory.empty());
    }

    /// Load specified configuration file
    /**
     * The configuration file passed is parsed by this method by using
     * the IniConfig class. If the parsing is unsuccessful a ::WARNING
     * is reported.
     *
     * The format of the configuration file should follow that of INI,
     * and every attribute present in the file is only allowed once, if
     * otherwise a ::WARNING will be reported. The file can contain at
     * most two sections, one named common and the other name alias. If
     * other sections exist a ::WARNING will be reported. Only the
     * following attributes is allowed in the common section of the
     * configuration file:
     * - certificatepath (CertificatePath(const std::string&))
     * - keypath (KeyPath(const std::string&))
     * - proxypath (ProxyPath(const std::string&))
     * - cacertificatesdirectory (CACertificatesDirectory(const std::string&))
     * - cacertificatepath (CACertificatePath(const std::string&))
     * - timeout (Timeout(int))
     * - joblist (JobListFile(const std::string&))
     * - defaultservices (AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType))
     * - rejectservices (AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType))
     * - verbosity (Verbosity(const std::string&))
     * - brokername (Broker(const std::string&) or Broker(const std::string&, const std::string&))
     * - brokerarguments (Broker(const std::string&) or Broker(const std::string&, const std::string&))
     * - bartender (Bartender(const std::list<URL>&))
     * - vomsserverpath (VOMSESPath(const std::string&))
     * - username (UserName(const std::string&))
     * - password (Password(const std::string&))
     * - keypassword (KeyPassword(const std::string&))
     * - keysize (KeySize(int))
     * - certificatelifetime (CertificateLifeTime(const Period&))
     * - slcs (SLCS(const URL&))
     * - storedirectory (StoreDirectory(const std::string&))
     * - jobdownloaddirectory (JobDownloadDirectory(const std::string&))
     * - idpname (IdPName(const std::string&))
     *
     * where the method in parentheses is the associated setter method.
     * If other attributes exist in the common section a ::WARNING will
     * be reported for each of these attributes.
     * In the alias section aliases can be defined, and should represent
     * a selection of services. The alias can then refered to by input
     * to the AddServices(const std::list<std::string>&, ServiceType)
     * and AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * methods. An alias can not contain any of the characters '.', ':',
     * ' ' or '\\t' and should be defined as follows:
     * \f[ <alias\_name>=<service\_type>:<flavour>:<service\_url>|<alias\_ref> [...] \f]
     * where \<alias_name\> is the name of the defined alias,
     * \<service_type\> is the service type in lower case, \<flavour\>
     * is the type of middleware plugin to use, \<service_url\> is the
     * URL which should be used to contact the service and \<alias_ref\>
     * is another defined alias. The parsed aliases will be stored
     * internally and resolved when needed. If a alias already exist,
     * and another alias with the same name is parsed then this other
     * alias will overwrite the existing alias.
     *
     * @param conffile is the path to the configuration file.
     * @param ignoreJobListFile is a optional boolean which indicates
     *        whether the joblistfile attribute in the configuration
     *        file should be ignored. Default is to ignored it
     *        (\c true).
     * @return If loading the configuration file succeeds \c true is
     *         returned, otherwise \c false is returned.
     * @see SaveToFile()
     **/
    bool LoadConfigurationFile(const std::string& conffile, bool ignoreJobListFile = true);

    /// Save to INI file.
    /**
     * This method will save the object data as a INI file. The
     * saved file can be loaded with the LoadConfigurationFile method.
     *
     * @param filename the name of the file which the data will be
     *        saved to.
     * @return \c false if unable to get handle on file, otherwise
     *         \c true is returned.
     * @see LoadConfigurationFile()
     **/
    bool SaveToFile(const std::string& filename) const;

    /// Apply credentials to BaseConfig
    /**
     * This methods sets the BaseConfig credentials to the credentials
     * contained in this object. It also passes user defined configuration
     * overlay if any.
     *
     * @see InitializeCredentials()
     * @see CredentialsFound()
     * @see BaseConfig
     * @param ccfg a BaseConfig object which will configured with
     *        the credentials of this object.
     **/
    void ApplyToConfig(BaseConfig& ccfg) const;

    /// Check for validity
    /**
     * The validity of an object created from this class can be checked
     * using this casting operator. An object is valid if the
     * constructor did not encounter any errors.
     * @see operator!()
     **/
    operator bool() const { return ok; }
    /// Check for non-validity
    /**
     * See #operator bool() for a description.
     * @see #operator bool()
     **/
    bool operator!() const { return !ok; }

    /// Set path to job list file
    /**
     * The method takes a path to a file which will be used as the job
     * list file for storing and reading job information. If the
     * specified path \a path does not exist a empty job list file will
     * be tried created. If creating the job list file in any way fails
     * \a false will be returned and a ::ERROR message will be reported.
     * Otherwise \a true is returned. If the directory containing the
     * file does not exist, it will be tried created. The method will
     * also return \a false if the file is not a regular file.
     *
     * The attribute associated with this setter method is 'joblist'.
     *
     * @param path the path to the job list file.
     * @return If the job list file is a regular file or if it can be
     *         created \a true is returned, otherwise \a false is
     *         returned.
     * @see JobListFile() const
     **/
    bool JobListFile(const std::string& path);
    /// Get a reference to the path of the job list file.
    /**
     * The job list file is used to store and fetch information about
     * submitted computing jobs to computing services. This method will
     * return the path to the specified job list file.
     *
     * @return The path to the job list file is returned.
     * @see JobListFile(const std::string&)
     **/
    const std::string& JobListFile() const { return joblistfile; }

    bool ResolveAliases(std::list<std::string>& services, ServiceType st);

    /// Set timeout
    /**
     * When communicating with a service the timeout specifies how long,
     * in seconds, the communicating instance should wait for a
     * response. If the response have not been recieved before this
     * period in time, the connection is typically dropped, and an error
     * will be reported.
     *
     * This method will set the timeout to the specified integer. If
     * the passed integer is less than or equal to 0 then \c false is
     * returned and the timeout will not be set, otherwise \c true is
     * returned and the timeout will be set to the new value.
     *
     * The attribute associated with this setter method is 'timeout'.
     *
     * @param newTimeout the new timeout value in seconds.
     * @return \c false in case \a newTimeout <= 0, otherwise \c true.
     * @see Timeout() const
     * @see DEFAULT_TIMEOUT
     **/
    bool Timeout(int newTimeout);
    /// Get timeout
    /**
     * Returns the timeout in seconds.
     *
     * @return timeout in seconds.
     * @see Timeout(int)
     * @see DEFAULT_TIMEOUT
     **/
    int  Timeout() const { return timeout; }

    /// Set verbosity.
    /**
     * The verbosity will be set when invoking this method. If the
     * string passed cannot be parsed into a corresponding LogLevel,
     * using the function a
     * ::WARNING is reported and \c false is returned, otherwise \c true
     * is returned.
     *
     * The attribute associated with this setter method is 'verbosity'.
     *
     * @return \c true in case the verbosity could be set to a allowed
     *         LogLevel, otherwise \c false.
     * @see Verbosity() const
     **/
    bool Verbosity(const std::string& newVerbosity);
    /// Get the user selected level of verbosity.
    /**
     * The string representation of the verbosity level specified by the
     * user is returned when calling this method. If the user have not
     * specified the verbosity level the empty string will be
     * referenced.
     *
     * @return the verbosity level, or empty if it has not been set.
     * @see Verbosity(const std::string&)
     **/
    const std::string& Verbosity() const { return verbosity; }

    /// Set broker to use in target matching
    /**
     * The string passed to this method should be in the format:
     * \f[<name>[:<argument>]\f]
     * where the \<name\> is the name of the broker and cannot contain
     * any ':', and the optional \<argument\> should contain arguments which
     * should be passed to the broker.
     *
     * Two attributes are associated with this setter method
     * 'brokername' and 'brokerarguments'.
     *
     * @param name the broker name and argument specified in the format
     *        given above.
     * @return This method allways returns \c true.
     * @see Broker
     * @see Broker(const std::string&, const std::string&)
     * @see Broker() const
     * @see DEFAULT_BROKER
     **/
    bool Broker(const std::string& name);
    /// Set broker to use in target matching
    /**
     * As opposed to the Broker(const std::string&) method this method
     * sets broker name and arguments directly from the passed two
     * arguments.
     *
     * Two attributes are associated with this setter method
     * 'brokername' and 'brokerarguments'.
     *
     * @param name is the name of the broker.
     * @param argument is the arguments of the broker.
     * @return This method always returns \c true.
     * @see Broker
     * @see Broker(const std::string&)
     * @see Broker() const
     * @see DEFAULT_BROKER
     **/
    bool Broker(const std::string& name, const std::string& argument) { broker = std::make_pair<std::string, std::string>(name, argument); return true;}
    /// Get the broker and corresponding arguments.
    /**
     * The returned pair contains the broker name as the first component
     * and the argument as the second.
     *
     * @see Broker(const std::string&)
     * @see Broker(const std::string&, const std::string&)
     * @see DEFAULT_BROKER
     **/
    const std::pair<std::string, std::string>& Broker() const { return broker; }

    /// Set bartenders, used to contact Chelonia
    /**
     * Takes as input a vector of Bartender URLs.
     *
     * The attribute associated with this setter method is 'bartender'.
     *
     * @param urls is a list of URL object to be set as bartenders.
     * @return This method always returns \c true.
     * @see AddBartender(const URL&)
     * @see Bartender() const
     **/
    bool Bartender(const std::vector<URL>& urls) { bartenders = urls; return true; }
    /// Set bartenders, used to contact Chelonia
    /**
     * Takes as input a Bartender URL and adds this to the list of
     * bartenders.
     *
     * @param url is a URL to be added to the list of bartenders.
     * @see Bartender(const std::list<URL>&)
     * @see Bartender() const
     **/
    void AddBartender(const URL& url) { bartenders.push_back(url); }
    /// Get bartenders
    /**
     * Returns a list of Bartender URLs
     *
     * @return The list of bartender URL objects is returned.
     * @see Bartender(const std::list<URL>&)
     * @see AddBartender(const URL&)
     **/
    const std::vector<URL>& Bartender() const { return bartenders; }

    /// Set path to file containing VOMS configuration
    /**
     * Set path to file which contians list of VOMS services and
     * associated configuration parameters needed to contact those
     * services. It is used by arcproxy.
     *
     * The attribute associated with this setter method is
     * 'vomsserverpath'.
     *
     * @param path the path to VOMS configuration file
     * @return This method always return true.
     * @see VOMSESPath() const
     **/
    bool VOMSESPath(const std::string& path) { vomsesPath = path; return true; }
    /// Get path to file containing VOMS configuration
    /**
     * Get path to file which contians list of VOMS services and
     * associated configuration parameters.
     *
     * @return The path to VOMS configuration file is returned.
     * @see VOMSESPath(const std::string&)
     **/
    const std::string& VOMSESPath();

    /// Set user-name for SLCS
    /**
     * Set username which is used for requesting credentials from
     * Short Lived Credentials Service.
     *
     * The attribute associated with this setter method is 'username'.
     *
     * @param name is the name of the user.
     * @return This method always return true.
     * @see UserName() const
     **/
    bool UserName(const std::string& name) { username = name; return true; }
    /// Get user-name
    /**
     * Get username which is used for requesting credentials from
     * Short Lived Credentials Service.
     *
     * @return The username is returned.
     * @see UserName(const std::string&)
     **/
    const std::string& UserName() const { return username; }

    /// Set password
    /**
     * Set password which is used for requesting credentials from
     * Short Lived Credentials Service.
     *
     * The attribute associated with this setter method is 'password'.
     *
     * @param newPassword is the new password to set.
     * @return This method always returns true.
     * @see Password() const
     **/
    bool Password(const std::string& newPassword) { password = newPassword; return true; }
    /// Get password
    /**
     * Get password which is used for requesting credentials from
     * Short Lived Credentials Service.
     *
     * @return The password is returned.
     * @see Password(const std::string&)
     **/
    const std::string& Password() const { return password; }

    /// Set path to user proxy
    /**
     * This method will set the path of the user proxy. Note that the
     * InitializeCredentials() method will also try to set this path, by
     * searching in different locations.
     *
     * The attribute associated with this setter method is 'proxypath'
     *
     * @param newProxyPath is the path to a user proxy.
     * @return This method always returns \c true.
     * @see InitializeCredentials()
     * @see CredentialsFound()
     * @see ProxyPath() const
     **/
    bool ProxyPath(const std::string& newProxyPath) { proxyPath = newProxyPath; return true;}
    /// Get path to user proxy.
    /**
     * Retrieve path to user proxy.
     *
     * @return Returns the path to the user proxy.
     * @see ProxyPath(const std::string&)
     **/
    const std::string& ProxyPath() const { return proxyPath; }

    /// Set path to certificate
    /**
     * The path to user certificate will be set by this method. The path
     * to the correcsponding key can be set with the
     * KeyPath(const std::string&) method. Note that the
     * InitializeCredentials() method will also try to set this path, by
     * searching in different locations.
     *
     * The attribute associated with this setter method is
     * 'certificatepath'.
     *
     * @param newCertificatePath is the path to the new certificate.
     * @return This method always returns \c true.
     * @see InitializeCredentials()
     * @see CredentialsFound() const
     * @see CertificatePath() const
     * @see KeyPath(const std::string&)
     **/
    bool CertificatePath(const std::string& newCertificatePath) { certificatePath = newCertificatePath; return true; }
    /// Get path to certificate
    /**
     * The path to the cerficate is returned when invoking this method.
     * @return The certificate path is returned.
     * @see InitializeCredentials()
     * @see CredentialsFound() const
     * @see CertificatePath(const std::string&)
     * @see KeyPath() const
     **/
    const std::string& CertificatePath() const { return certificatePath; }

    /// Set path to key
    /**
     * The path to user key will be set by this method. The path to the
     * corresponding certificate can be set with the
     * CertificatePath(const std::string&) method. Note that the
     * InitializeCredentials() method will also try to set this path, by
     * searching in different locations.
     *
     * The attribute associated with this setter method is 'keypath'.
     *
     * @param newKeyPath is the path to the new key.
     * @return This method always returns \c true.
     * @see InitializeCredentials()
     * @see CredentialsFound() const
     * @see KeyPath() const
     * @see CertificatePath(const std::string&)
     * @see KeyPassword(const std::string&)
     * @see KeySize(int)
     **/
    bool KeyPath(const std::string& newKeyPath) { keyPath = newKeyPath; return true; }
    /// Get path to key
    /**
     * The path to the key is returned when invoking this method.
     *
     * @return The path to the user key is returned.
     * @see InitializeCredentials()
     * @see CredentialsFound() const
     * @see KeyPath(const std::string&)
     * @see CertificatePath() const
     * @see KeyPassword() const
     * @see KeySize() const
     **/
    const std::string& KeyPath() const { return keyPath; }

    /// Set password for generated key
    /**
     * Set password to be used to encode private key of credentials
     * obtained from Short Lived Credentials Service.
     *
     * The attribute associated with this setter method is
     * 'keypassword'.
     *
     * @param newKeyPassword is the new password to the key.
     * @return This method always returns \c true.
     * @see KeyPassword() const
     * @see KeyPath(const std::string&)
     * @see KeySize(int)
     **/
    bool KeyPassword(const std::string& newKeyPassword) { keyPassword = newKeyPassword; return true; }
    /// Get password for generated key
    /**
     * Get password to be used to encode private key of credentials
     * obtained from Short Lived Credentials Service.
     *
     * @return The key password is returned.
     * @see KeyPassword(const std::string&)
     * @see KeyPath() const
     * @see KeySize() const
     **/
    const std::string& KeyPassword() const { return keyPassword; }

    /// Set key size
    /**
     * Set size/strengt of private key of credentials obtained from
     * Short Lived Credentials Service.
     *
     * The attribute associated with this setter method is 'keysize'.
     *
     * @param newKeySize is the size, an an integer, of the key.
     * @return This method always returns \c true.
     * @see KeySize() const
     * @see KeyPath(const std::string&)
     * @see KeyPassword(const std::string&)
     **/
    bool KeySize(int newKeySize) { keySize = newKeySize; return true;}
    /// Get key size
    /**
     * Get size/strengt of private key of credentials obtained from
     * Short Lived Credentials Service.
     *
     * @return The key size, as an integer, is returned.
     * @see KeySize(int)
     * @see KeyPath() const
     * @see KeyPassword() const
     **/
    int KeySize() const { return keySize; }

    /// Set CA-certificate path
    /**
     * The path to the file containing CA-certificate will be set
     * when calling this method. This configuration parameter is
     * deprecated - use CACertificatesDirectory instead. Only arcslcs
     * uses it.
     *
     * The attribute associated with this setter method is
     * 'cacertificatepath'.
     *
     * @param newCACertificatePath is the path to the CA-certificate.
     * @return This method always returns \c true.
     * @see CACertificatePath() const
     **/
    bool CACertificatePath(const std::string& newCACertificatePath) { caCertificatePath = newCACertificatePath; return true; }
    /// Get path to CA-certificate
    /**
     * Retrieve the path to the file containing CA-certificate.
     * This configuration parameter is deprecated.
     *
     * @return The path to the CA-certificate is returned.
     * @see CACertificatePath(const std::string&)
     **/
    const std::string& CACertificatePath() const { return caCertificatePath; }

    /// Set path to CA-certificate directory
    /**
     * The path to the directory containing CA-certificates will be set
     * when calling this method. Note that the InitializeCredentials()
     * method will also try to set this path, by searching in different
     * locations.
     *
     * The attribute associated with this setter method is
     * 'cacertificatesdirectory'.
     *
     * @param newCACertificatesDirectory is the path to the
     *        CA-certificate directory.
     * @return This method always returns \c true.
     * @see InitializeCredentials()
     * @see CredentialsFound() const
     * @see CACertificatesDirectory() const
     **/
    bool CACertificatesDirectory(const std::string& newCACertificatesDirectory) { caCertificatesDirectory = newCACertificatesDirectory; return true; }
    /// Get path to CA-certificate directory
    /**
     * Retrieve the path to the CA-certificate directory.
     *
     * @return The path to the CA-certificate directory is returned.
     * @see InitializeCredentials()
     * @see CredentialsFound() const
     * @see CACertificatesDirectory(const std::string&)
     **/
    const std::string& CACertificatesDirectory() const { return caCertificatesDirectory; }

    /// Set certificate life time
    /**
     * Sets lifetime of user certificate which will be obtained from
     * Short Lived Credentials Service.
     *
     * The attribute associated with this setter method is
     * 'certificatelifetime'.
     *
     * @param newCertificateLifeTime is the life time of a certificate,
     *        as a Period object.
     * @return This method always returns \c true.
     * @see CertificateLifeTime() const
     **/
    bool CertificateLifeTime(const Period& newCertificateLifeTime) { certificateLifeTime = newCertificateLifeTime; return true; }
    /// Get certificate life time
    /**
     * Gets lifetime of user certificate which will be obtained from
     * Short Lived Credentials Service.
     *
     * @return The certificate life time is returned as a Period object.
     * @see CertificateLifeTime(const Period&)
     **/
    const Period& CertificateLifeTime() const { return certificateLifeTime; }

    /// Set the URL to the Short Lived Certificate Service (SLCS).
    /**
     *
     * The attribute associated with this setter method is 'slcs'.
     *
     * @param newSLCS is the URL to the SLCS
     * @return This method always returns \c true.
     * @see SLCS() const
     **/
    bool SLCS(const URL& newSLCS) { slcs = newSLCS; return true; }
    /// Get the URL to the Short Lived Certificate Service (SLCS).
    /**
     *
     * @return The SLCS is returned.
     * @see SLCS(const URL&)
     **/
    const URL& SLCS() const { return slcs; }

    /// Set store directory
    /**
     * Sets directory which will be used to store credentials obtained
     * from Short Lived Credential Servide.
     *
     * The attribute associated with this setter method is
     * 'storedirectory'.
     * @param newStoreDirectory is the path to the store directory.
     * @return This method always returns \c true.
     * @see
     **/
    bool StoreDirectory(const std::string& newStoreDirectory) { storeDirectory = newStoreDirectory; return true; }
    /// Get store diretory
    /**
     * Sets directory which is used to store credentials obtained
     * from Short Lived Credential Servide.
     *
     * @return The path to the store directory is returned.
     * @see StoreDirectory(const std::string&)
     **/
    const std::string& StoreDirectory() const { return storeDirectory; }

    /// Set download directory
    /**
     * Sets directory which will be used to download the job
     * directory using arcget command.
     *
     * The attribute associated with this setter method is
     * 'jobdownloaddirectory'.
     * @param newDownloadDirectory is the path to the download directory.
     * @return This method always returns \c true.
     * @see
     **/
    bool JobDownloadDirectory(const std::string& newDownloadDirectory) { downloadDirectory = newDownloadDirectory; return true; }

    /// Get download directory
    /**
     * returns directory which will be used to download the job
     * directory using arcget command.
     *
     * The attribute associated with the method is
     * 'jobdownloaddirectory'.
     * @return This method returns the job download directory.
     * @see
     **/
    const std::string& JobDownloadDirectory() const { return downloadDirectory; }

    /// Set IdP name
    /**
     * Sets Identity Provider name (Shibboleth) to which user belongs.
     * It is used for contacting Short Lived Certificate Service.
     *
     * The attribute associated with this setter method is 'idpname'.
     * @param name is the new IdP name.
     * @return This method always returns \c true.
     * @see
     **/
    bool IdPName(const std::string& name) { idPName = name; return true; }
    /// Get IdP name
    /**
     * Gets Identity Provider name (Shibboleth) to which user belongs.
     *
     * @return The IdP name
     * @see IdPName(const std::string&)
     **/
    const std::string& IdPName() const { return idPName; }

    /// Set path to configuration overlay file
    /**
     * Content of specified file is a backdoor to configuration XML
     * generated from information stored in this class. The content
     * of file is passed to BaseConfig class in ApplyToConfig(BaseConfig&)
     * then merged with internal configuration XML representation.
     * This feature is meant for quick prototyping/testing/tuning of
     * functionality without rewriting code. It is meant for developers and
     * most users won't need it.
     *
     * The attribute associated with this setter method is 'overlayfile'.
     * @param path is the new overlay file path.
     * @return This method always returns \c true.
     * @see
     **/
    bool OverlayFile(const std::string& path) { overlayfile = path; return true; }
    /// Get path to configuration overlay file
    /**
     * @return The overlay file path
     * @see OverlayFile(const std::string&)
     **/
    const std::string& OverlayFile() const { return overlayfile; }

    /// Set path to directory storing utility files for DataPoints
    /**
     * Some DataPoints can store information on remote services in local
     * files. This method sets the path to the directory containing these
     * files. For example arc* tools set it to ARCUSERDIRECTORY and A-REX
     * sets it to the control directory. The directory is created if it
     * does not exist.
     * @param path is the new utils dir path.
     * @return This method always returns \c true.
     */
    bool UtilsDirPath(const std::string& dir);
    /// Get path to directory storing utility files for DataPoints
    /**
     * @return The utils dir path
     * @see UtilsDirPath(const std::string&)
     */
    const std::string& UtilsDirPath() const { return utilsdir; };

    /// Set User for filesystem access
    /**
     * Sometimes it is desirable to use the identity of another user
     * when accessing the filesystem. This user can be specified through
     * this method. By default this user is the same as the user running
     * the process.
     * @param u User identity to use
     */
    void SetUser(const User& u) { user = u; };

    /// Get User for filesystem access
    /**
     * @return The user identity to use for file system access
     * @see SetUser(const User&)
     */
    const User& GetUser() const { return user; };


    std::list<ConfigEndpoint> GetDefaultServices(ConfigEndpoint::Type type = ConfigEndpoint::ANY);

    ConfigEndpoint GetService(const std::string& alias);

    std::list<ConfigEndpoint> GetServices(const std::string& groupOrAlias, ConfigEndpoint::Type type = ConfigEndpoint::ANY);

    std::list<ConfigEndpoint> GetServicesInGroup(const std::string& group, ConfigEndpoint::Type type = ConfigEndpoint::ANY);


    const std::string& PreferredInfoInterface() const { return preferredinfointerface; };

    bool PreferredInfoInterface(const std::string& preferredinfointerface_) {
      preferredinfointerface = preferredinfointerface_;
      return true;
    }


    const std::string& PreferredJobInterface() const { return preferredjobinterface; };

    bool PreferredJobInterface(const std::string& preferredjobinterface_) {
      preferredjobinterface = preferredjobinterface_;
      return true;
    }


    const std::list<std::string>& RejectedURLs() const { return rejectedURLs; };


    /// Path to ARC user home directory
    /**
     * The \a ARCUSERDIRECTORY variable is the path to the ARC home
     * directory of the current user. This path is created using the
     * User::Home() method.
     * @see User::Home()
     **/
    static const std::string ARCUSERDIRECTORY;
    /// Path to system configuration
    /**
     * The \a SYSCONFIG variable is the path to the system configuration
     * file. This variable is only equal to SYSCONFIGARCLOC if ARC is installed
     * in the root (highly unlikely).
     **/
    static const std::string SYSCONFIG;
    /// Path to system configuration at ARC location.
    /**
     * The \a SYSCONFIGARCLOC variable is the path to the system configuration
     * file which reside at the ARC installation location.
     **/
    static const std::string SYSCONFIGARCLOC;
    /// Path to default configuration file
    /**
     * The \a DEFAULTCONFIG variable is the path to the default
     * configuration file used in case no configuration file have been
     * specified. The path is created from the
     * ARCUSERDIRECTORY object.
     **/
    static const std::string DEFAULTCONFIG;
    /// Path to example configuration
    /**
     * The \a EXAMPLECONFIG variable is the path to the example
     * configuration file.
     **/
    static const std::string EXAMPLECONFIG;

    /// Default timeout in seconds
    /**
     * The \a DEFAULT_TIMEOUT specifies interval which will be used
     * in case no timeout interval have been explicitly specified. For a
     * description about timeout see Timeout(int).
     * @see Timeout(int)
     * @see Timeout() const
     **/
    static const int DEFAULT_TIMEOUT = 20;

    /// Default broker
    /**
     * The \a DEFAULT_BROKER specifies the name of the broker which
     * should be used in case no broker is explicitly chosen.
     * @see Broker
     * @see Broker(const std::string&)
     * @see Broker(const std::string&, const std::string&)
     * @see Broker() const
     **/
    static const std::string DEFAULT_BROKER;

    static std::string GetInterfaceNameOfInfoInterface(std::string infointerface) {
      if (infointerface == "LDAPGLUE2") return "org.nordugrid.ldapglue2";
      if (infointerface == "LDAPGLUE1") return "org.nordugrid.ldapglue1";
      if (infointerface == "LDAPNG") return "org.nordugrid.ldapng";
      if (infointerface == "WSRFGLUE2") return "org.nordugrid.wsrfglue2";
      if (infointerface == "EMIES") return "org.ogf.emies";
      if (infointerface == "BES") return "org.ogf.bes";
      return "";
    }

    static std::string GetInterfaceNameOfJobInterface(std::string jobinterface) {
      if (jobinterface == "GRIDFTPJOB") return "org.nordugrid.gridftpjob";
      if (jobinterface == "BES") return "org.nordugrid.xbes";
      if (jobinterface == "EMIES") return "org.ogf.emies";
      return "";
    }

    static std::string GetInterfaceNameOfRegistryInterface(std::string registryinterface) {
      if (registryinterface == "EGIIS") return "org.nordugrid.ldapegiis";
      if (registryinterface == "EMIR") return "org.nordugrid.emir";
      return "";
    }


  private:

    static ConfigEndpoint ServiceFromLegacyString(std::string);

    void setDefaults();
    static bool makeDir(const std::string& path);
    static bool copyFile(const std::string& source,
                         const std::string& destination);
    bool ResolveAlias(std::list<std::string>& services, ServiceType st,
                      std::list<std::string>& resolvedAlias);
    bool ResolveAlias(ServiceList& services,
                      std::list<std::string>& resolvedAlias);
    bool CreateDefaultConfigurationFile() const;

    std::list<ConfigEndpoint> FilterServices(const std::list<ConfigEndpoint>&, ConfigEndpoint::Type);


    std::string joblistfile;

    int timeout;

    std::string verbosity;

    // Broker name and arguments.
    std::pair<std::string, std::string> broker;

    ServiceList selectedServices;
    ServiceList rejectedServices;

    std::list<ConfigEndpoint> defaultServices;
    std::map<std::string, ConfigEndpoint> allServices;
    std::map<std::string, std::list<ConfigEndpoint> > groupMap;
    std::list<std::string> rejectedURLs;

    // Vector needed for random access.
    std::vector<URL> bartenders;

    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string keyPassword;
    int keySize;
    std::string caCertificatePath;
    std::string caCertificatesDirectory;
    Period certificateLifeTime;

    URL slcs;

    std::string vomsesPath;

    std::string storeDirectory;
    std::string downloadDirectory;
    std::string idPName;

    std::string username;
    std::string password;

    std::string overlayfile;
    std::string utilsdir;

    std::string preferredjobinterface;
    std::string preferredinfointerface;
    // User whose identity (uid/gid) should be used to access filesystem
    // Normally this is the same as the process owner
    User user;
    // Private members not refered to outside this class:
    // Alias map.
    XMLNode aliasMap;

    bool ok;

    initializeCredentialsType initializeCredentials;

    static Logger logger;
  };


  class CertEnvLocker {
  public:
    CertEnvLocker(const UserConfig& cfg);
    ~CertEnvLocker(void);

  protected:
    std::string x509_user_key_old;
    std::string x509_user_key_new;
    bool x509_user_key_set;
    std::string x509_user_cert_old;
    std::string x509_user_cert_new;
    bool x509_user_cert_set;
    std::string x509_user_proxy_old;
    std::string x509_user_proxy_new;
    bool x509_user_proxy_set;
    std::string ca_cert_dir_old;
    bool ca_cert_dir_set;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
