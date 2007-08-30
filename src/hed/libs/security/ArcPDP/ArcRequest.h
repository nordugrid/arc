#ifndef __ARC_ARCREQUEST_H__
#define __ARC_ARCREQUEST_H__

#include "Request.h"
#include <list>
#include "common/XMLNode.h"
#include "common/Logger.h"

/** ArcRequest, Parsing the specified Arc request format*/

namespace Arc {

//typedef std::list<RequestItem*> ReqItemList;

class ArcRequest : public Request {

public:
  virtual ReqItemList getRequestItems () const;
  virtual void setRequestItems (ReqItemList sl);
  

  //**Parse request information from a file*/
  ArcRequest (const std::string& filename);

  //**Parse request information from a xml stucture in memory*/
  ArcRequest (XMLNode& node);
  virtual ~ArcRequest();

private:
  void make_request(XMLNode& node);

};

} // namespace Arc

#endif /* __ARC_ARCREQUEST_H__ */
