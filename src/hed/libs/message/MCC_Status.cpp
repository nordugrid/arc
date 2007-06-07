// MCC_Status.cpp

#include "MCC_Status.h"

namespace Arc {

  std::string string(StatusKind kind){
    if (kind==STATUS_UNDEFINED)
      return "STATUS_UNDEFINED";
    else if (kind==STATUS_OK)
      return "STATUS_OK";
    else if(kind==GENERIC_ERROR)
      return "GENERIC_ERROR";
    else if(kind==PARSING_ERROR)
      return "PARSING_ERROR";
    else if(kind==PROTOCOL_RECOGNIZED_ERROR)
      return "PROTOCOL_RECOGNIZED_ERROR";
    else if(kind==UNKNOWN_SERVICE_ERROR)
      return "UNKNOWN_SERVICE_ERROR";
    else if(kind==BUSY_ERROR)
      return "BUSY_ERROR";
    else  // There should be no other alternative!
      return "";
  }

  MCC_Status::MCC_Status(StatusKind kind,
			 const std::string& origin,
			 const std::string& explanation):
    kind(kind),
    origin(origin),
    explanation(explanation)
  {
  }

  bool MCC_Status::isOk() const{
    return kind==STATUS_OK;
  }

  StatusKind MCC_Status::getKind() const{
    return kind;
  }

  const std::string& MCC_Status::getOrigin() const{
    return origin;
  }

  const std::string& MCC_Status::getExplanation() const{
    return explanation;
  }

  MCC_Status::operator std::string() const{
    return origin + ": " + string(kind) + " (" + explanation + ")";
  }

}
