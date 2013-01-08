#ifndef __ARC_ENDPOINTQUERYINGSTATUS_H__
#define __ARC_ENDPOINTQUERYINGSTATUS_H__

#include <string>

namespace Arc {

/// Represents the status in the EntityRetriever of the query process of an Endpoint (service registry, computing element).
/**
 * An object of this class is returned by the instances of the EntityRetriever
 * (e.g. #ServiceEndpointRetriever, #TargetInformationRetriever,
 * #JobListRetriever) representing the state of the process of querying an
 * Endpoint. It contains an #EndpointQueryingStatusType enum (#getStatus), and
 * a description string (#getDescription) 
 * 
 * \ingroup compute
 * \headerfile EndpointQueryingStatus.h arc/compute/EndpointQueryingStatus.h 
 */
class EndpointQueryingStatus {
public:
  /** The possible states: */
  enum EndpointQueryingStatusType {
    UNKNOWN, /**< the state is unknown */
    SUSPENDED_NOTREQUIRED, /**< Querying of the endpoint is suspended since querying it is not required. */
    STARTED, /**< the query process was started */
    FAILED, /**< the query process failed */
    NOPLUGIN, /**< there is no plugin for the given Endpoint InterfaceName (so the query process was not even started) */
    NOINFORETURNED, /**< query was successful but the response didn't contain entity information */
    SUCCESSFUL /**< the query process was successful */
  };

  /** String representation of the states in the enum #EndpointQueryingStatusType */
      static std::string str(EndpointQueryingStatusType status);

  /** A new EndpointQueryingStatus is created with #UNKNOWN status and with an empty description by default */
  EndpointQueryingStatus(EndpointQueryingStatusType status = UNKNOWN, const std::string& description = "") : status(status), description(description) {};

  /** This EndpointQueryingStatus object equals to an enum #EndpointQueryingStatusType if it contains the same state */
  bool operator==(EndpointQueryingStatusType s) const { return status == s; };
  /** This EndpointQueryingStatus object equals to another EndpointQueryingStatus object, if their state equals.
    The description doesn't matter.
  */
  bool operator==(const EndpointQueryingStatus& s) const { return status == s.status; };
  /** Inequality. \see operator==(EndpointQueryingStatusType) */
  bool operator!=(EndpointQueryingStatusType s) const { return status != s; };
  /** Inequality. \see operator==(const EndpointQueryingStatus&) */
  bool operator!=(const EndpointQueryingStatus& s) const { return status != s.status; };
  /** \return true if the status is not successful */
  bool operator!() const { return status != SUCCESSFUL; };
  /** \return true if the status is successful */
  operator bool() const  { return status == SUCCESSFUL; };

  /** Setting the EndpointQueryingStatus object's state
    \param[in] s the new enum #EndpointQueryingStatusType status 
  */
  EndpointQueryingStatus& operator=(EndpointQueryingStatusType s) { status = s; return *this; };
  /** Copying the EndpointQueryingStatus object into this one.
    \param[in] s the EndpointQueryingStatus object whose status and description will be copied into this object 
  */
  EndpointQueryingStatus& operator=(const EndpointQueryingStatus& s) { status = s.status; description = s.description; return *this; };

  /** Return the enum #EndpointQueryingStatusType contained within this EndpointQueryingStatus object */
  EndpointQueryingStatusType getStatus() const { return status; };
  /** Return the description string contained within this EndpointQueryingStatus object */
  const std::string& getDescription() const { return description; };
  /** String representation of the EndpointQueryingStatus object,
    which is currently simply the string representation of the enum #EndpointQueryingStatusType
  */
  std::string str() const { return str(status); };

  friend bool operator==(EndpointQueryingStatusType, const EndpointQueryingStatus&);

private:
  EndpointQueryingStatusType status;
  std::string description;
};

inline bool operator==(EndpointQueryingStatus::EndpointQueryingStatusType eqst, const EndpointQueryingStatus& eqs) { return eqs == eqst; }

} // namespace Arc

#endif // __ARC_ENDPOINTQUERYINGSTATUS_H__
