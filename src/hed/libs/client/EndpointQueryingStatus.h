#ifndef __ARC_ENDPOINTQUERYINGSTATUS_H__
#define __ARC_ENDPOINTQUERYINGSTATUS_H__

#include <string>

namespace Arc {

///
/**
 *
 **/
class EndpointQueryingStatus {
public:
  enum EndpointQueryingStatusType { UNKNOWN, STARTED, FAILED, NOPLUGIN, SUCCESSFUL };

  //! Conversion to string.
  /*! Conversion from EndpointQueryingStatusType to string.
    @param s The EndpointQueryingStatusType to convert.
  */
  static std::string str(EndpointQueryingStatusType status);

  EndpointQueryingStatus(EndpointQueryingStatusType status = UNKNOWN, const std::string& description = "") : status(status), description(description) {};

  bool operator==(EndpointQueryingStatusType s)    { return status == s; };
  bool operator==(const EndpointQueryingStatus& s) { return status == s.status; };
  bool operator!=(EndpointQueryingStatusType s)    { return status != s; };
  bool operator!=(const EndpointQueryingStatus& s) { return status != s.status; };
  bool operator!() const { return status != SUCCESSFUL; };
  operator bool() const  { return status == SUCCESSFUL; };

  EndpointQueryingStatus& operator=(EndpointQueryingStatusType s) { status = s; return *this; };
  EndpointQueryingStatus& operator=(const EndpointQueryingStatus& s) { status = s.status; description = s.description; return *this; };

  EndpointQueryingStatus getStatus() const { return status; };
  const std::string& getDescription() const { return description; };
  std::string str() const { return str(status); };

private:
  EndpointQueryingStatusType status;
  std::string description;
};

} // namespace Arc

#endif // __ARC_ENDPOINTQUERYINGSTATUS_H__
