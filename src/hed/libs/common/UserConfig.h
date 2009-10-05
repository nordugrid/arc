// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/DateTime.h>
#include <arc/URL.h>

namespace Arc {

  typedef std::map<std::string, std::list<URL> > URLListMap;

  class Logger;
  class XMLNode;

  enum ServiceType {
    COMPUTING,
    INDEX
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
   * - vomsserverpath / VOMSServerPath(const std::string&)
   * - username / UserName(const std::string&)
   * - password / Password(const std::string&)
   * - keypassword / KeyPassword(const std::string&)
   * - keysize / KeySize(int)
   * - certificatelifetime / CertificateLifeTime(const Period&)
   * - slcs / SLCS(const URL&)
   * - storedirectory / StoreDirectory(const std::string&)
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
    UserConfig(bool initializeCredentials = true);
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
               bool initializeCredentials = true,
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
               bool initializeCredentials = true,
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
     * searches in different locations. First the user proxy or the user
     * key/certificate pair is tried located in the following order:
     * - Proxy path specified by the environment variable
     *   X509_USER_PROXY
     * - Key/certificate path specified by the environment
     *   X509_USER_KEY and X509_USER_CERT
     * - Proxy path specified in either configuration file passed to the
     *   contructor or explicitly set using the setter method
     *   ProxyPath(const std::string&)
     * - Key/certificate path specified in either configuration file
     *   passed to the constructor or explicitly set using the setter
     *   methods KeyPath(const std::string&) and
     *   CertificatePath(const std::string&)
     * - ProxyPath with file name x509up_u concatenated with the user ID
     *   located in the OS temporary directory.
     *
     * If the proxy or key/certificate pair have been explicitly
     * specified only the specified path(s) will be tried, and if not
     * found a ::ERROR is reported.
     * If the proxy or key/certificate have not been specified and it is
     * not located in the temporary directory a ::WARNING will be
     * reported and the host key/certificate pair is tried and then the
     * Globus key/certificate pair and a ::ERROR will be reported
     * if not found in any of these locations.
     *
     * Together with the proxy and key/certificate pair, the path to the
     * directory containing CA certificates is also tried located when
     * invoking this method. The directory will be tried located in the
     * following order:
     * - Path specified by the X509_CERT_DIR environment variable.
     * - Path explicitly specified either in a parsed configuration file
     *   using the cacertficatecirectory or by using the setter method
     *   CACertificatesDirectory().
     * - Path created by concatenating the output of User::Home()
     *   with '.globus' and 'certificates' separated by the directory
     *   delimeter.
     * - Path created by concatenating the output of
     *   Glib::get_home_dir() with '.globus' and 'certificates'
     *   separated by the directory delimeter.
     * - Path created by concatenating the output of ArcLocation::Get(),
     *   with 'etc' and 'certificates' separated by the directory
     *   delimeter.
     * - Path created by concatenating the output of ArcLocation::Get(),
     *   with 'etc', 'grid-security' and 'certificates' separated by the
     *   directory delimeter.
     * - Path created by concatenating the output of ArcLocation::Get(),
     *   with 'share' and 'certificates' separated by the directory
     *   delimeter.
     * - Path created by concatenating 'etc', 'grid-security' and
     *   'certificates' separated by the directory delimeter.
     *
     * If the CA certificate directory have explicitly been specified
     * and the directory does not exist a ::ERROR is reported. If none
     * of the directories above does not exist a ::ERROR is reported.
     *
     * @see CredentialsFound()
     * @see ProxyPath(const std::string&)
     * @see KeyPath(const std::string&)
     * @see CertificatePath(const std::string&)
     * @see CACertificatesDirectory(const std::string&)
     **/
    void InitializeCredentials();
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
      return !(proxyPath.empty() && (certificatePath.empty() || keyPath.empty()) || caCertificatesDirectory.empty());
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
     * - vomsserverpath (VOMSServerPath(const std::string&))
     * - username (UserName(const std::string&))
     * - password (Password(const std::string&))
     * - keypassword (KeyPassword(const std::string&))
     * - keysize (KeySize(int))
     * - certificatelifetime (CertificateLifeTime(const Period&))
     * - slcs (SLCS(const URL&))
     * - storedirectory (StoreDirectory(const std::string&))
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
     **/
    bool LoadConfigurationFile(const std::string& conffile, bool ignoreJobListFile = true);

    /// Apply credentials to BaseConfig
    /**
     * This methods sets the BaseConfig credentials to the credentials
     * contained in this object.
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

    /// Add selected and rejected services.
    /**
     * This method adds selected services and adds services to reject
     * from the specified list \a services, which contains
     * string objects. The syntax of a single element in the
     * list must be expressed in the following two formats:
     * \f[ [-]<flavour>:<service\_url>|[-]<alias> \f]
     * where the optional '-' indicate that the service should be added
     * to the private list of services to reject. In the first
     * format the \<flavour\> part indicates the type of ACC plugin to
     * use when contacting the service, which is specified by the URL
     * \<service_url\>, and in the second format the \<alias\> part
     * specifies a alias defined in a parsed configuration file, note
     * that the alias must not contain any of the charaters ':', '.',
     * ' ' or '\\t'.
     * If a alias cannot be resolved an ::ERROR will be reported to the
     * logger and the method will return false. If a element in the list
     * \a services cannot be parsed an ::ERROR will be reported, and
     * the element is skipped.
     *
     * Two attributes are indirectly associated with this setter method
     * 'defaultservices' and 'rejectservices'. The values specified with
     * the 'defaultservices' attribute will be added to the list of
     * selected services, and like-wise with the 'rejectservices'
     * attribute.
     *
     * @param services is a list of services to either select or reject.
     * @param st indicates the type of the specfied services.
     * @return This method returns \c false in case an alias cannot be
     *         resolved. In any other case \c true is returned.
     * @see AddServices(const std::string&, const std::string&, ServiceType)
     * @see GetSelectedServices()
     * @see GetRejectedServices()
     * @see ClearSelectedServices()
     * @see ClearRejectedServices()
     * @see LoadConfigurationFile()
     **/
    bool AddServices(const std::list<std::string>& services, ServiceType st);
    /// Add selected and rejected services.
    /**
     * The only diffence in behaviour of this method compared to the
     * AddServices(const std::list<std::string>&, ServiceType) method
     * is the input parameters and the format these parameters should
     * follow. Instead of having an optional '-' in front of the string
     * selected and rejected services should be specified in the two
     * different arguments.
     *
     * Two attributes are indirectly associated with this setter method
     * 'defaultservices' and 'rejectservices'. The values specified with
     * the 'defaultservices' attribute will be added to the list of
     * selected services, and like-wise with the 'rejectservices'
     * attribute.
     *
     * @param selected is a list of services which will be added to the
     *        selected services of this object.
     * @param rejected is a list of services which will be added to the
     *        rejected services of this object.
     * @param st specifies the ServiceType of the services to add.
     * @return This method return \c false in case an alias cannot be
     *         resolved. In any other case \c true is returned.
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see GetSelectedServices()
     * @see GetRejectedServices()
     * @see ClearSelectedServices()
     * @see ClearRejectedServices()
     * @see LoadConfigurationFile()
     **/
    bool AddServices(const std::list<std::string>& selected,
                     const std::list<std::string>& rejected,
                     ServiceType st);

    /// Get selected services
    /**
     * Get the selected services with the ServiceType specified by
     * \a st.
     *
     * @param st specifies which ServiceType should be returned by the
     *        method.
     * @return The selected services is returned.
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * @see GetRejectedServices(ServiceType) const
     * @see ClearSelectedServices()
     **/
    const URLListMap& GetSelectedServices(ServiceType st) const;
    /// Get rejected services
    /**
     * Get the rejected services with the ServiceType specified by
     * \a st.
     *
     * @param st specifies which ServiceType should be returned by the
     *        method.
     * @return The rejected services is returned.
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * @see GetSelectedServices(ServiceType)
     * @see ClearRejectedServices()
     **/
    const URLListMap& GetRejectedServices(ServiceType st) const;

    /// Clear selected services
    /**
     * Calling this method will cause the internally stored selected
     * services to be cleared.
     *
     * @see ClearSelectedServices(ServiceType)
     * @see ClearRejectedServices()
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * @see GetSelectedServices()
     **/
    void ClearSelectedServices();
    /// Clear selected services with specified ServiceType
    /**
     * Calling this method will cause the internally stored selected
     * services with the ServiceType \a st to be cleared.
     *
     * @see ClearSelectedServices()
     * @see ClearRejectedServices(ServiceType)
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * @see GetSelectedServices()
     **/
    void ClearSelectedServices(ServiceType st);
    /// Clear selected services
    /**
     * Calling this method will cause the internally stored rejected
     * services to be cleared.
     *
     * @see ClearRejectedServices(ServiceType)
     * @see ClearSelectedServices()
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * @see GetRejectedServices()
     **/
    void ClearRejectedServices();
    /// Clear rejected services with specified ServiceType
    /**
     * Calling this method will cause the internally stored rejected
     * services with the ServiceType \a st to be cleared.
     *
     * @see ClearRejectedServices()
     * @see ClearSelectedServices(ServiceType)
     * @see AddServices(const std::list<std::string>&, ServiceType)
     * @see AddServices(const std::list<std::string>&, const std::list<std::string>&, ServiceType)
     * @see GetRejectedServices()
     **/
    void ClearRejectedServices(ServiceType st);

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
     * Takes as input a list of Bartender URLs.
     *
     * The attribute associated with this setter method is 'bartender'.
     *
     * @param urls is a list of URL object to be set as bartenders.
     * @return This method always returns \c true.
     * @see AddBartender(const URL&)
     * @see Bartender() const
     **/
    bool Bartender(const std::list<URL>& urls) { bartenders = urls; return true; }
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
    const std::list<URL>& Bartender() const { return bartenders; }

    /// Set path to ???
    /**
     * ???
     *
     * The attribute associated with this setter method is
     * 'vomsserverpath'.
     *
     * @param path the path to ???
     * @return This method always return true.
     * @see VOMSServerPath() const
     **/
    bool VOMSServerPath(const std::string& path) { vomsServerPath = path; return true; }
    /// Get path to ???
    /**
     * ???
     *
     * @return The path to ??? is returned.
     * @see VOMSServerPath(const std::string&)
     **/
    const std::string& VOMSServerPath() const { return vomsServerPath; }

    /// Set user-name
    /**
     * ???
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
     * ???
     *
     * @return The username is returned.
     * @see UserName(const std::string&)
     **/
    const std::string& UserName() const { return username; }

    /// Set password
    /**
     * ???
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
     * ???
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

    /// Set password for key
    /**
     * ???
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
    /// Get password for key
    /**
     * ???
     *
     * @return The key password is returned.
     * @see KeyPassword(const std::string&)
     * @see KeyPath() const
     * @see KeySize() const
     **/
    const std::string& KeyPassword() const { return keyPassword; }

    /// Set key size
    /**
     * ???
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
     * ???
     *
     * @return The key size, as an integer, is returned.
     * @see KeySize(int)
     * @see KeyPath() const
     * @see KeyPassword() const
     **/
    int KeySize() const { return keySize; }

    /// Set CA-certificate path
    /**
     * ???
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
     * ???
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
     * ???
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
     * ???
     *
     * @return The certificate life time is returned as a Period object.
     * @see CertificateLifeTime(const Period&)
     **/
    const Period& CertificateLifeTime() const { return certificateLifeTime; }

    /// Set the URL to the Short Lived Certificate Service (SLCS).
    /**
     * ???
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
     * ???
     *
     * @return The SLCS is returned.
     * @see SLCS(const URL&)
     **/
    const URL& SLCS() const { return slcs; }

    /// Set store directory
    /**
     * ???
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
     * ???
     *
     * @return The path to the store directory is returned.
     * @see StoreDirectory(const std::string&)
     **/
    const std::string& StoreDirectory() const { return storeDirectory; }

    /// Set IdP name
    /**
     * ???
     *
     * The attribute associated with this setter method is 'idpname'.
     * @param name is the new IdP name.
     * @return This method always returns \c true.
     * @see
     **/
    bool IdPName(const std::string& name) { idPName = name; return true; }
    /// Get IdP name
    /**
     * ???
     *
     * @return The IdP name
     * @see IdPName(const std::string&)
     **/
    const std::string& IdPName() const { return idPName; }

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
     * file.
     **/
    static const std::string SYSCONFIG;
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

  private:
    void setDefaults();
    static bool makeDir(const std::string& path);
    static bool copyFile(const std::string& source, const std::string& destination);
    bool ResolveAlias(URLListMap& services, ServiceType st,
                      std::list<std::string>& resolvedAlias);
    bool ResolveAlias(std::pair<URLListMap, URLListMap>& services,
                      std::list<std::string>& resolvedAlias);
    bool CreateDefaultConfigurationFile() const;

    std::string joblistfile;

    int timeout;

    std::string verbosity;

    // Broker name and arguments.
    std::pair<std::string, std::string> broker;

    std::pair<URLListMap, URLListMap> selectedServices;
    std::pair<URLListMap, URLListMap> rejectedServices;

    std::list<URL> bartenders;

    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string keyPassword;
    int keySize;
    std::string caCertificatePath;
    std::string caCertificatesDirectory;
    Period certificateLifeTime;

    URL slcs;

    std::string vomsServerPath;

    std::string storeDirectory;

    std::string idPName;

    std::string username;
    std::string password;

    // Private members not refered to outside this class:
    // Alias map.
    XMLNode aliasMap;

    bool ok;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
