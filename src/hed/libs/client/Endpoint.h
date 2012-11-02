#ifndef __ARC_ENDPOINT_H__
#define __ARC_ENDPOINT_H__

#include <string>
#include <set>
#include <algorithm>

namespace Arc {

class ConfigEndpoint;
class ExecutionTarget;
class ComputingEndpointAttributes;
  
/// Represents an endpoint of a service with a given interface type and capabilities
/**
The type of the interface is described by a string called InterfaceName (from the
GLUE2 specification).
An Endpoint object must have a URL, and it is quite useless without capabilities
(the system has to know if an Endpoint is a service registry or a computing element),
but the InterfaceName is optional.

The Endpoint object also contains information about the health state and quality level
of the endpoint, and optionally the requested submission interface name,
which will be used later if a job will be submitted to a computing element
related to this endpoint.

\see CapabilityEnum where the capabilities are listed.
*/
class Endpoint {
public:
  /** The capabilities:
  - REGISTRY: service registry capable of returning endpoints
  - COMPUTINGINFO: local information system of a computing element
                   capable of returning information about the resource
  - JOBLIST: local information system of a computing element
             capable of returning the list of jobs on the resource
  - JOBSUBMIT: interface of a computing element where jobs can be submitted
  - JOBMANAGEMENT: interface of a computing element where jobs can be managed
  - UNSPECIFIED: unspecified capability
  */
  enum CapabilityEnum { REGISTRY, COMPUTINGINFO, JOBLIST, JOBSUBMIT, JOBMANAGEMENT, UNSPECIFIED};
  
  /** Get the string representation of the given #CapabilityEnum. */
  static std::string GetStringForCapability(Endpoint::CapabilityEnum cap) {
    if (cap == Endpoint::REGISTRY) return "information.discovery.registry";
    if (cap == Endpoint::COMPUTINGINFO) return "information.discovery.resource";
    if (cap == Endpoint::JOBLIST) return "information.discovery.resource";
    if (cap == Endpoint::JOBSUBMIT) return "executionmanagement.jobexecution";
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
   **/
  Endpoint(const ExecutionTarget& e, const std::string& rsi = "");

  /// Create new Endpoint from ExecutionTarget object
  /**
   * \param cea ComputingEndpointAttributes object to create new Endpoint from.
   * \param rsi string specifying the requested submission interface if any.
   *        Default value is the empty string.
   **/
  Endpoint(const ComputingEndpointAttributes& cea, const std::string& rsi = "");
  
  /// Create a new Endpoint from a ConfigEndpoint
  /**
    The URL, InterfaceName and the RequestedSubmissionInterfaceName will be copied
    from the ConfigEndpoint, and if the type of the ConfigEndpoint is REGISTRY or
    COMPUTINGINFO, the given capability will be added to the new Endpoint object.
    \param[in] endpoint is the ConfigEndpoint object which will be converted to an Endpoint

    This will call #operator=.
  */
  Endpoint(const ConfigEndpoint& endpoint) { *this = endpoint; }
  
  /** Checks if the Endpoint has the given capability specified by a CapabilityEnum
    \param[in] cap is the specified CapabilityEnum
    \return true if the Endpoint has the given capability
  */
  bool HasCapability(Endpoint::CapabilityEnum cap) const;
  
  /** Checks if the Endpoint has the given capability specified by a string
    \param[in] cap is a string specifying a capability
    \return true if the Endpoint has the given capability
  */
  bool HasCapability(const std::string& cap) const;

  /** Returns a string representation of the Endpoint containing the URL,
    the main capability and the InterfaceName
  */
  std::string str() const;

  /** A string identifying the service exposing this endpoint.  It currently
    extracts the host name from the URL, but this may be refined later. */
  std::string getServiceName() const;
  
  /** Needed for std::map to be able to sort the keys */
  bool operator<(const Endpoint& other) const;
  
  /** Copy a ConfigEndpoint into the Endpoint */
  Endpoint& operator=(const ConfigEndpoint& e);
  
  /** The string representation of the URL of the Endpoint */
  std::string URLString;
  /** The type of the interface (GLUE2 InterfaceName) */
  std::string InterfaceName;  
  /** GLUE2 HealthState */
  std::string HealthState;
  /** GLUE2 HealthStateInfo */
  std::string HealthStateInfo;
  /** GLUE2 QualityLevel */
  std::string QualityLevel;
  /** Set of GLUE2 Capability strings */
  std::set<std::string> Capability;
  /** A GLUE2 InterfaceName requesting an InterfaceName used for job submission.
  
    If a user specifies an InterfaceName for submitting jobs, that information
    will be stored here and will be used when collecting information about the
    computing element. Only those job submission interfaces will be considered
    which has this requested InterfaceName.
  */
  std::string RequestedSubmissionInterfaceName;
  /** The ID of the service this Endpoint belongs to */
  std::string ServiceID;
};

} // namespace Arc

#endif // __ARC_ENDPOINT_H__
