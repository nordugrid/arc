#ifndef __ARC_ENDPOINT_H__
#define __ARC_ENDPOINT_H__

#include <string>
#include <set>
#include <algorithm>
#include <map>

#include <arc/URL.h>
#include <arc/compute/EndpointQueryingStatus.h>

namespace Arc {

class ConfigEndpoint;
class ExecutionTarget;
class Endpoint;
class EndpointQueryingStatus;
class ComputingEndpointAttributes;
  
/// Key comparison object definition for Endpoint objects
/**
 * \since Added in 3.0.0.
 * \ingroup compute
 * \headerfile Endpoint.h arc/compute/Endpoint.h 
 */
typedef bool (*EndpointCompareFn)(const Endpoint&, const Endpoint&);

/// Status map for Endpoint objects.
/**
 * Wrapper class inheriting from std::map providing no extra functionality than
 * that of std::map. It is needed due to limitations in the language wrapping
 * software (SWIG) that can't handle more than 2 template arguments.
 * 
 * \since Added in 3.0.0.
 * \ingroup compute
 * \headerfile Endpoint.h arc/compute/Endpoint.h 
 */
class EndpointStatusMap : public std::map<Endpoint, EndpointQueryingStatus, EndpointCompareFn> {
public:
  /// Creates a std::map with the key comparison object set to Endpoint::ServiceIDCompare
  EndpointStatusMap();
  /// Creates a std::map using \c fn as key comparison object
  EndpointStatusMap(EndpointCompareFn fn) : std::map<Endpoint, EndpointQueryingStatus, EndpointCompareFn>(fn) {}
  /// Copy constructor
  EndpointStatusMap(const EndpointStatusMap& m) : std::map<Endpoint, EndpointQueryingStatus, EndpointCompareFn>(m) {}
  ~EndpointStatusMap() {}
};

/// Represents an endpoint of a service with a given interface type and capabilities
/**
 * This class similar in structure to the %Endpoint entity in the %GLUE2
 * specification. The type of the interface is described by a string called
 * InterfaceName (from the %GLUE2 specification). An Endpoint object must have a
 * URL, and it is quite useless without capabilities (the system has to know if
 * an Endpoint is a service registry or a computing element), but the
 * InterfaceName is optional.
 * 
 * The Endpoint object also contains information about the health state and
 * quality level of the endpoint, and optionally the requested submission
 * interface name, which will be used later if a job will be submitted to a
 * computing element related to this endpoint.
 * 
 * \see CapabilityEnum where the capabilities are listed.
 * \since Added in 2.0.0.
 * \ingroup compute
 * \headerfile Endpoint.h arc/compute/Endpoint.h 
 */
class Endpoint {
public:
  /// Values for classifying capabilities of services
  enum CapabilityEnum {
    /// Service registry capable of returning endpoints
    REGISTRY,
    /// Local information system of a computing element capable of returning information about the resource    
    COMPUTINGINFO,
    /// Local information system of a computing element capable of returning the list of jobs on the resource
    JOBLIST,
    /// Interface of a computing element where jobs can be submitted
    JOBSUBMIT,
    /// Interface of a computing element where jobs can be created
    /**
     * \since Added in 3.0.0.
     **/
    JOBCREATION,
    /// Interface of a computing element where jobs can be managed
    JOBMANAGEMENT,
    /// Unspecified capability
    UNSPECIFIED
  };
  
  /// Get string representation of #CapabilityEnum.
  /**
   * \return The %GLUE2 capability string associated with the passed
   * #CapabilityEnum value is returned.
   **/
  static std::string GetStringForCapability(Endpoint::CapabilityEnum cap) {
    if (cap == Endpoint::REGISTRY) return "information.discovery.registry";
    if (cap == Endpoint::COMPUTINGINFO) return "information.discovery.resource";
    if (cap == Endpoint::JOBLIST) return "information.discovery.resource";
    if (cap == Endpoint::JOBSUBMIT) return "executionmanagement.jobexecution";
    if (cap == Endpoint::JOBCREATION) return "executionmanagement.jobcreation";
    if (cap == Endpoint::JOBMANAGEMENT) return "executionmanagement.jobmanager";
    return "";
  }
  
  /// Create a new Endpoint with a list of capability strings
  /**
    \param[in] URLString is a string representing the URL of the endpoint
    \param[in] Capability is a list of capability strings
               specifying the capabilities of the service
    \param[in] InterfaceName is a string specifying the type of the interface of the service
  */
  Endpoint(const std::string& URLString = "",
           const std::set<std::string>& Capability = std::set<std::string>(),
           const std::string& InterfaceName = "")
    : URLString(URLString), InterfaceName(InterfaceName), Capability(Capability) {}

  /// Create a new Endpoint with a single capability specified by the #CapabilityEnum
  /**
    \param[in] URLString is a string representing the URL of the endpoint
    \param[in] cap is a #CapabilityEnum specifying the single capability of the endpoint
    \param[in] InterfaceName is an optional string specifying the type of the interface
  */
  Endpoint(const std::string& URLString,
           const Endpoint::CapabilityEnum cap,
           const std::string& InterfaceName = "")
    : URLString(URLString), InterfaceName(InterfaceName), Capability() { Capability.insert(GetStringForCapability(cap)); }

  /// Create new Endpoint from ExecutionTarget object
  /**
   * \param e ExecutionTarget object to create new Endpoint from.
   * \param rsi string specifying the requested submission interface if any.
   *        Default value is the empty string.
   * \since Added in 3.0.0.
   **/
  Endpoint(const ExecutionTarget& e, const std::string& rsi = "");

  /// Create new Endpoint from ComputingEndpointAttributes object
  /**
   * \param cea ComputingEndpointAttributes object to create new Endpoint from.
   * \param rsi string specifying the requested submission interface if any.
   *        Default value is the empty string.
   * \since Added in 3.0.0.
   **/
  Endpoint(const ComputingEndpointAttributes& cea, const std::string& rsi = "");
  
  /// Create a new Endpoint from a ConfigEndpoint
  /**
   * The ConfigEndpoint::URLString, ConfigEndpoint::InterfaceName and the
   * ConfigEndpoint::RequestedSubmissionInterfaceName attributes will be copied
   * from the ConfigEndpoint, and if the type of the ConfigEndpoint is #REGISTRY
   * or #COMPUTINGINFO, the given capability will be added to the new Endpoint
   * object.
   * 
   * \param[in] endpoint is the ConfigEndpoint object which will be converted to
   *  an Endpoint
   **/
  Endpoint(const ConfigEndpoint& endpoint) { *this = endpoint; }
  
  /// Check for capability
  /**
   * Checks if the Endpoint has the given capability specified by a
   * #CapabilityEnum value.
   * 
   * \param[in] cap is the specified #CapabilityEnum
   * \return true if the Endpoint has the given capability
  */
  bool HasCapability(Endpoint::CapabilityEnum cap) const;
  
  /// Check for capability
  /**
   * Checks if the Endpoint has the given capability specified by a string.
   * 
   * \param[in] cap is a string specifying a capability.
   * \return true if the Endpoint has the given capability.
  */
  bool HasCapability(const std::string& cap) const;

  /// Get string representation of this object
  /**
   * \return String formatted as:
   * \verbatim
   <URLString> (<InterfaceName>[, capabilities: <Capabilities space separated>])
   \endverbatim
   * where if #InterfaceName is empty, "<empty InterfaceName>" is used.
   **/
  std::string str() const;

  /// Get name of service from #URLString attribute
  /**
   * \return If #URLString contains "://", a URL object will be created from it
   * and if the host part of it is non empty it is returned, otherwise
   * #URLString is returned.
   * \since Added in 3.0.0.
   **/
  std::string getServiceName() const;
  
  /// Key comparison method
  /**
   * Compare passed Endpoint object with this by value returned by #str().
   * 
   * \param[in] other Endpoint object to compare with.
   * \return The result of lexicographically less between this object (lhs) and
   * other (rhs) compared using value returned by #str() method, is returned.
   **/
  bool operator<(const Endpoint& other) const;
  
  /// Key comparison function for comparing Endpoint objects.
  /**
   * Compare endpoints by #ServiceID, #URLString and #InterfaceName in that
   * order. The attributes are compared lexicographically.
   * 
   * \return If the ServiceID attributes are unequal lexicographically less
   * between the ServiceID attributes of a and b is returned. If they equal then
   * same procedure is done with the URLString attribute, if they equal
   * lexicographically less between the InterfaceName attributes of a and b is
   * returned.
   * 
   * \since Added in 3.0.0.
   **/
  static bool ServiceIDCompare(const Endpoint& a, const Endpoint& b);

  /// Set from a ConfigEndpoint object
  /**
   * \return \c *this is returned.
   **/
  Endpoint& operator=(const ConfigEndpoint& e);
  
  /// The string representation of the URL of the Endpoint
  std::string URLString;
  /// The type of the interface (%GLUE2 InterfaceName)
  std::string InterfaceName;  
  /// %GLUE2 HealthState
  std::string HealthState;
  /// %GLUE2 HealthStateInfo
  std::string HealthStateInfo;
  /// %GLUE2 QualityLevel
  std::string QualityLevel;
  /// Set of %GLUE2 Capability strings
  std::set<std::string> Capability;
  /// A %GLUE2 InterfaceName requesting an InterfaceName used for job submission.
  /**
   * If a user specifies an InterfaceName for submitting jobs, that information
   * will be stored here and will be used when collecting information about the
   * computing element. Only those job submission interfaces will be considered
   * which has this requested InterfaceName.
   **/
  std::string RequestedSubmissionInterfaceName;
  /// ID of service this Endpoint belongs to
  /**
   * \since Added in 3.0.0.
   **/
  std::string ServiceID;

  /// Get bounds in EndpointStatusMap corresponding to Endpoint
  /**
   * \param[in] endpoint An Endpoint object for which the bounds of equivalent
   *  Endpoint objects in the EndpointStatusMap should be found.
   * \param[in] statusMap See description above.
   * \return The lower and upper bound of the equivalent to the passed Endpoint
   *  object is returned as a pair (lower, upper).
   **/
  static std::pair<EndpointStatusMap::const_iterator, EndpointStatusMap::const_iterator> getServiceEndpoints(const Endpoint&, const EndpointStatusMap&);
};

} // namespace Arc

#endif // __ARC_ENDPOINT_H__
