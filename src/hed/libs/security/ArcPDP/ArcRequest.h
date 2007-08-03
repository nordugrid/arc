#ifndef __ARC_ARCREQUEST_H__
#define __ARC_ARCREQUEST_H__

#include "Request.h"
#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"

/** ArcRequest, Parsing the specified Arc request format*/

namespace Arc {

typedef std::list<RequestItem*> ReqItemList;

class ArcRequest : public Request {

public:
  virtual ReqItemList getRequestItems () const {};
  virtual void setRequestItems (const ReqItemList* sl) {};

  //**Parse request information from a input stream, such as one file*/
  ArcRequest (const std::ifstream& input) {};

  //**Parse request information from a xml stucture in memory*/
  ArcRequest (const Arc::XMLNode& node) {};
  virtual ~ArcRequest();
};

} // namespace Arc

#endif /* __ARC_ARCREQUEST_H__ */
