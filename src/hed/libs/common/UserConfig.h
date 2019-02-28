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

  /** \addtogroup common
   *  @{ */
  class Logger;
  class XMLNode;

  /// Type of service
  enum ServiceType {
    COMPUTING, ///< A service that processes jobs
    INDEX      ///< A service that provides information
  };

  /// Represents the endpoint of service with a given type and GLUE2 InterfaceName
  /**
    A ConfigEndpoint can be a service registry or a local information system of a
    computing element. It has a URL, and optionally GLUE2 InterfaceName and a
    RequestedSubmissionInterfaceName, which will be used to filter the possible
    job submission interfaces on a computing element.
    \headerfile UserConfig.h arc/UserConfig.h
  */
  class ConfigEndpoint {
  public:
    /// Types of ComputingEndpoint objects.
    enum Type {
      REGISTRY,      ///< a service registry
      COMPUTINGINFO, ///< a local information system of a computing element
      ANY            ///< both, only used for filtering, when both types are accepted
    };

    /// Creates a ConfigEndpoint from a URL an InterfaceName and a Type.
    /**
      \param[in] URLString is a string containing the URL of the ConfigEndpoint
      \param[in] InterfaceName is a string containing the type of the interface
        based on the InterfaceName attribute in the GLUE2 specification
      \param[in] type is either ConfigEndpoint::REGISTRY or ConfigEndpoint::COMPUTINGINFO
    */
    ConfigEndpoint(const std::string& URLString = "", const std::string& InterfaceName = "", ConfigEndpoint::Type type = ConfigEndpoint::ANY)
      : type(type), URLString(URLString), InterfaceName(InterfaceName) {}
      
    /// The type of the ConfigEndpoint: REGISTRY or COMPUTINGINFO.
    Type type;
    
    /// A string representing the URL of the ConfigEndpoint.
    std::string URLString;
    
    /// A string representing the interface type (based on the InterfaceName attribute of the GLUE2 specification).
    std::string InterfaceName;
        
    /// A GLUE2 InterfaceName requesting a job submission interface.
    /**
        This will be used when collecting information about the
        computing element. Only those job submission interfaces will be considered
        which has this requested InterfaceName.
    */
    std::string RequestedSubmissionInterfaceName;

    /// Return true if the URL is not empty.
    operator bool() const {
      return (!URLString.empty());
    }

    /// Returns true if the URL is empty.
    bool operator!() const {
      return (URLString.empty());
    }
    
    /// Returns true if the type, the URLString, the InterfaceName and the RequestedSubmissionInterfaceName matches.
    bool operator==(ConfigEndpoint c) const {
      return (type == c.type) && (URLString == c.URLString) && (InterfaceName == c.InterfaceName) && (RequestedSubmissionInterfaceName == c.RequestedSubmissionInterfaceName);
    }
  };

  /// Returns "computing" if st is COMPUTING, "index" if st is "INDEX", otherwise an empty string.
  std::string tostring(const ServiceType st);

  /// Defines how user credentials are looked for.
  /**
    * For complete information see description of
    *  UserConfig::InitializeCredentials(initializeCredentialsType)
    * method.
    * \headerfile UserConfig.h arc/UserConfig.h
    */
  class initializeCredentialsType {
   public:
    /// initializeType determines how UserConfig deals with credentials.
    typedef enum {
      SkipCredentials,         ///< Don't look for credentials
      NotTryCredentials,       ///< Look for credentials but don't evaluate them
      TryCredentials,          ///< Look for credentials and test if they are valid
      RequireCredentials,      ///< Look for credentials, test if they are valid and report errors if not valid
      SkipCANotTryCredentials, ///< Same as NotTryCredentials but skip checking CA certificates
      SkipCATryCredentials,    ///< Same as TryCredentials but skip checking CA certificates
      SkipCARequireCredentials ///< Same as RequireCredentials but skip checking CA certificates
    } initializeType;
    /// Construct a new initializeCredentialsType with initializeType TryCredentials.
    initializeCredentialsType(void):val(TryCredentials) { };
    /// Construct a new initializeCredentialsType with initializeType v.
    initializeCredentialsType(initializeType v):val(v) { };
    /// Returns true if this initializeType is the same as v.
    bool operator==(initializeType v) { return (val == v); };
    /// Returns true if this initializeType is not the same as v.
    bool operator!=(initializeType v) { return (val != v); };
    /// Operator returns initializeType.
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
   * - joblisttype / JobListType(const std::string&)
   * - verbosity / Verbosity(const std::string&)
   * - brokername / Broker(const std::string&) or Broker(const std::string&, const std::string&)
   * - brokerarguments / Broker(const std::string&) or Broker(const std::string&, const std::string&)
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
   * - submissioninterface / SubmissionInterface(const std::string&)
   * - infointerface / InfoInterface(const std::string&)
   *
   * where the first term is the name of the attribute used in the
   * configuration file, and the second term is the associated setter
   * method (for more information about a given attribute see the
   * description of the setter method).
   *
   * The configuration file should have a INI-style format and the
   * IniConfig class will thus be used to parse the file. The above
   * mentioned attributes should be placed in the common section.
   *
   * Besides the options above, the configuration file can contain
   * information about services (service registries and computing elements).
   * Each service has to be put in its on section. Each service has
   * an alias, which is a short name. The name of the section consists
   * of the word `registry` for service registries and `computing` for
   * computing elements, then contains a slash and the alias of the
   * service. e.g. `[registry/index1]` or `[computing/testce]`
   * In a service section the possible options are the following:
   *  - url: is the url of the service
   *  - default: if yes, then this service will be used if no other is specified
   *  - group: assigns the service to a group with a given name
   * 
   * For computing elements the following additional options exist:
   *  - infointerface: the GLUE2 InterfaceName of the local information system
   *  - submissioninterface: the GLUE2 InterfaceName to the job submission interface
   *
   * For a service registry the following additional option exist:
   *  - registryinterface: the GLUE2 InterfaceName of the service registry interface
   *
   * These services can be accessed by the #GetService, #GetServices,
   * #GetDefaultServices, #GetServicesInGroup methods, which return ConfigEndpoint
   * object(s). The ConfigEndpoint objects contain the URL and the InterfaceNames
   * of the services.
   *
   * The UserConfig class also provides a method InitializeCredentials()
   * for locating user credentials by searching in different standard
   * locations. The CredentialsFound() method can be used to test if
   * locating the credentials succeeded.
   * \headerfile UserConfig.h arc/UserConfig.h
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
     * and every attribute present in the file is only allowed once 
     * (except the `rejectmanagement` and `rejectdiscovery` attributes),
     * otherwise a ::WARNING will be reported. For the list of allowed
     * attributes see the detailed description of UserConfig.
     *
     * @param conffile is the path to the configuration file.
     * @param ignoreJobListFile is a optional boolean which indicates
     *        whether the joblistfile attribute in the configuration
     *        file should be ignored. Default is to ignored it
     *        (\c true).
     * @return If loading the configuration file succeeds \c true is
     *         returned, otherwise \c false is returned.
     * @see SaveToFile()
     * \since Changed in 4.0.0. Added the joblisttype attribute to attributes
     *  being parsed. Parsing of the retired bartender attribute is removed.
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
     * \since Changed in 4.0.0. %Credential string is checked first and used if
     *  non-empty (see
     *  \ref CredentialString(const std::string&) "CredentialString").
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
     * list file for storing and reading job information. This method always
     * return true.
     *
     * The attribute associated with this setter method is 'joblist'.
     *
     * @param path the path to the job list file.
     * @return true is always returned.
     * @see JobListFile() const
     * \since Changed in 4.0.0. Method now always returns true.
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
    
    /// Set type of job storage
    /**
     * Possible storage types are BDB and XML. This method always return true.
     * 
     * The attribute associated with this setter method is 'joblisttype'.
     * 
     * @param type of job storage
     * @return true is always returned.
     * @see JobListType()
     * \since Added in 4.0.0.
     **/
    bool JobListType(const std::string& type);
    /// Get type of job storage
    /**
     * @return The type of job storage is returned.
     * @see JobListType(const std::string&)
     * \since Added in 4.0.0.
     **/
    const std::string& JobListType() const { return joblisttype; }

    /// Set timeout
    /**
     * When communicating with a service the timeout specifies how long,
     * in seconds, the communicating instance should wait for a
     * response. If the response have not been received before this
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
     * \since Changed in 4.1.0. The argument string is now treated
     *  case-insensitively.
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
    bool Broker(const std::string& name, const std::string& argument) { broker = std::pair<std::string, std::string>(name, argument); return true;}
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

    /// Set credentials.
    /**
     * For code which does not need credentials stored in files, this method
     * can be used to set the credential as a string stored in memory.
     *
     * @param cred The credential represented as a string
     * \since Added in 4.0.0.
     */
    void CredentialString(const std::string& cred) { credentialString = cred; }
    /// Get credentials.
    /**
     * Returns the string representation of credentials previously set by
     * CredentialString(const std::string&).
     *
     * @return String representation of credentials
     * \since Added in 4.0.0.
     */
    const std::string& CredentialString() const { return credentialString; }

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

    /// Check if configuration represents same user identity (false negatives are likely)
    /**
     * Compare identity represented by this object to one provided.
     *
     * @param other configuration object to compare to
     * @return true if identities are definitely the same.
     * @see
     **/
    bool IsSameIdentity(UserConfig const & other) const {
      if(credentialString != other.credentialString) return false;
      if(proxyPath != other.proxyPath) return false;
      if(certificatePath != other.certificatePath) return false;
      if(username != other.username) return false;
      return true;
    }

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
     * @param dir is the new utils dir path.
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

    /// Set the default local information system interface
    /**
      For services which does not specify a local information system
      interface, this default will be used.

      If a local information system interface is given, the computing element
      will be only queried using this interface.
       
      \param infointerface_ is a string specifying a GLUE2 InterfaceName
      \return This method always returns \c true.
    */
    bool InfoInterface(const std::string& infointerface_) {
      infointerface = infointerface_;
      return true;
    }
    /// Get the default local information system interface
    /**
      \return the GLUE2 InterfaceName string specifying the default local information system interface
      \see InfoInterface(const std::string&)
    */
    const std::string& InfoInterface() const { return infointerface; };

    /// Set the default submission interface
    /**
      For services which does not specify a submission interface
      this default submission interface will be used.
     
      If a submission interface is given, then all the jobs will be
      submitted to this interface, no other job submission interfaces
      of the computing element will be tried.
       
      \param submissioninterface_ is a string specifying a GLUE2 InterfaceName
      \return This method always returns \c true.
    */
    bool SubmissionInterface(const std::string& submissioninterface_) {
      submissioninterface = submissioninterface_;
      return true;
    }
    /// Get the default submission interface
    /**
      \return the GLUE2 InterfaceName string specifying the default submission interface
      \see SubmissionInterface(const std::string&)
    */
    const std::string& SubmissionInterface() const { return submissioninterface; };

    /// Get the list of rejected service discovery URLs
    /**
      This list is populated by the (possibly multiple) `rejectdiscovery` configuration options.
      A service registry should not be queried if its URL matches any string in this list.
      \return a list of rejected service discovery URLs
    */
    const std::list<std::string>& RejectDiscoveryURLs() const { return rejectDiscoveryURLs; };
    /// Add list of URLs to ignored at service discovery
    /**
     * The passed list of strings will be added to the internal reject list
     * and they should represent URLs which should be ignored when doing service
     * discovery.
     * \param urls list of string representing URLs to ignore at service
     *        discovery
     **/
    void AddRejectDiscoveryURLs(const std::list<std::string>& urls) { rejectDiscoveryURLs.insert(rejectDiscoveryURLs.end(), urls.begin(), urls.end()); }
    /// Clear the rejected service discovery URLs
    /**
     * Clears the list of strings representing URLs which should be ignored
     * during service discovery.
     **/
    void ClearRejectDiscoveryURLs() { rejectDiscoveryURLs.clear(); }
    
    /// Get the list of rejected job managmenet URLs
    /**
      This list is populated by the (possibly multiple) `rejectmanagement` configuration options.
      Those jobs should not be managed, that reside on a computing element with a matching URL.
      \return a list of rejected job management URLs
    */
    const std::list<std::string>& RejectManagementURLs() const { return rejectManagementURLs; };


    /// Get the ConfigEndpoint for the service with the given alias
    /**
      Each service in the configuration file has its own section,
      and the name of the section contains the type of the service (`registry` or `computing`),
      and the alias of the service (separated by a slash).
      \param[in] alias is the alias of the service
      \return the ConfigEndpoint generated from the service with the given alias.
    */
    ConfigEndpoint GetService(const std::string& alias);

    /// Get the services in a given group filtered by type
    /**
      All services of the given group are returned if they match the type filter.
      \param[in] group is the name of the group
      \param[in] type is REGISTRY or COMPUTING if only those services are needed, or ANY if all
      \return a list of ConfigEndpoint objects, the services in the group,
        empty list if no such group, or no services matched the filter
    */
    std::list<ConfigEndpoint> GetServicesInGroup(const std::string& group, ConfigEndpoint::Type type = ConfigEndpoint::ANY);

    /// Get the services flagged as default filtered by type
    /**
      Return all the services which had `default=yes` in their configuration,
      if they have the given type.
      \param[in] type is REGISTRY or COMPUTING if only those services are needed, or ANY if all
      \return a list of ConfigEndpoint objects, the default services,
        empty list if there are no default service, or no services matched the filter
    */
    std::list<ConfigEndpoint> GetDefaultServices(ConfigEndpoint::Type type = ConfigEndpoint::ANY);

    /// Get one or more service with the given alias or in the given group filtered by type
    /**
      This is a convenience method for querying the configured services by both
      the name of a group or an alias of a service. If the name is a name of a group
      then all the services in the group will be returned (filtered by type). If
      there is no such group, then a service with the given alias is returned
      in a single item list (but only if it matches the filter).
      \param[in] groupOrAlias is either a name of a group or an alias of a service
      \param[in] type is REGISTRY or COMPUTING if only those services are needed, or ANY if all
      \return a list of ConfigEndpoint objects, the found services,
        empty list if no such group and no such alias or no services matched the filter
    */
    std::list<ConfigEndpoint> GetServices(const std::string& groupOrAlias, ConfigEndpoint::Type type = ConfigEndpoint::ANY);

    /// Get all services
    std::map<std::string, ConfigEndpoint> GetAllConfiguredServices() { return allServices; }


    /// Path to ARC user home directory
    /**
     * The \a ARCUSERDIRECTORY variable is the path to the ARC home
     * directory of the current user. This path is created using the
     * User::Home() method.
     * @see User::Home()
     **/
    static std::string ARCUSERDIRECTORY();
    /// Path to system configuration
    /**
     * The \a SYSCONFIG variable is the path to the system configuration
     * file. This variable is only equal to SYSCONFIGARCLOC if ARC is installed
     * in the root (highly unlikely).
     **/
    static std::string SYSCONFIG();
    /// Path to system configuration at ARC location.
    /**
     * The \a SYSCONFIGARCLOC variable is the path to the system configuration
     * file which reside at the ARC installation location.
     **/
    static std::string SYSCONFIGARCLOC();
    /// Path to default configuration file
    /**
     * The \a DEFAULTCONFIG variable is the path to the default
     * configuration file used in case no configuration file have been
     * specified. The path is created from the
     * ARCUSERDIRECTORY object.
     **/
    static std::string DEFAULTCONFIG();
    /// Path to example configuration
    /**
     * The \a EXAMPLECONFIG variable is the path to the example
     * configuration file.
     **/
    static std::string EXAMPLECONFIG();
    /// Path to default job list file
    /**
     * The \a JOBLISTFILE variable specifies the default path to the job list
     * file used by the ARC job management tools. The job list file is located
     * in the directory specified by the ARCUSERDIRECTORY variable with name
     * 'jobs.dat'.
     * @see ARCUSERDIRECTORY
     * \since Added in 4.0.0.
     **/
    static std::string JOBLISTFILE();

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
    static std::string DEFAULT_BROKER();

  private:

    static ConfigEndpoint ServiceFromLegacyString(std::string);

    void setDefaults();
    static bool makeDir(const std::string& path);
    static bool copyFile(const std::string& source,
                         const std::string& destination);
    bool CreateDefaultConfigurationFile() const;

    std::list<ConfigEndpoint> FilterServices(const std::list<ConfigEndpoint>&, ConfigEndpoint::Type);


    std::string joblistfile;
    std::string joblisttype;

    int timeout;

    std::string verbosity;

    // Broker name and arguments.
    std::pair<std::string, std::string> broker;

    std::list<ConfigEndpoint> defaultServices;
    std::map<std::string, ConfigEndpoint> allServices;
    std::map<std::string, std::list<ConfigEndpoint> > groupMap;
    std::list<std::string> rejectDiscoveryURLs;
    std::list<std::string> rejectManagementURLs;

    std::string credentialString;
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

    std::string submissioninterface;
    std::string infointerface;
    // User whose identity (uid/gid) should be used to access filesystem
    // Normally this is the same as the process owner
    User user;
    // Private members not refered to outside this class:
    bool ok;

    initializeCredentialsType initializeCredentials;

    static Logger logger;
  };


  /// Class for handling X509* variables in a multi-threaded environment.
  /**
   * This class is useful when using external libraries which depend on X509*
   * environment variables in a multi-threaded environment. When an instance of
   * this class is created it holds a lock on these variables until the
   * instance is destroyed. Additionally, if the credentials pointed to by the
   * those variables are owned by a different uid from the uid of the current
   * process, a temporary copy is made owned by the uid of the current process
   * and the X509 variable points there instead. This is to comply with some
   * restrictions in third-party libraries which insist on the credential files
   * being owned by the current uid.
   * \headerfile UserConfig.h arc/UserConfig.h
   */
  class CertEnvLocker {
  public:
    /// Create a lock on X509 environment variables. Blocks if another instance exists.
    CertEnvLocker(const UserConfig& cfg);
    /// Release lock on X509 environment variables and set back to old values if they were changed.
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

  /** @} */

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
