#ifndef __ARC_ARCREQUEST_H__
#define __ARC_ARCREQUEST_H__

#include <stdlib.h>

//#include "../../../../libs/common/ArcConfig.h"
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"


namespace Arc {

class Attribute{
 public:
  Attribute(){};
  virtual ~Attribute(void){};
  static Attribute* get_instance();
 private:
  AttributeValue value;
}

/**<subjects, actions, objects, condition> tuple */
class RequestItem{
 public:
  RequestItem(Arc::XMLNode* root);
  virtual ~RequestItem(void) { };

private:
  typedef std::set<Attribute*> Attributes;
  Attributes subjects;
  Attributes actions;
  Attributes objects;
  Attributes conditions;
}

/**A request can has a few <subjects, actions, objects> tuples */
class Request {
 public:
  Request(const Arc::XMLNode* root);
  virtual ~Request(void) { };

  //**Parse request information from a xml file*/
  //**There can be a RequestFactory that can produce different types of Request, such like XACMLRequest, ArcRequest, GACLRequest */
  static Request* get_instance(const char* filename);
  //**Parse request information from a xml stucture in memory*/
  static Request* get_instance(const Arc::XMLNode *node);  

 private:
  std::set<RequestItem*> requests;
};

} // namespace Arc

#endif /* __ARC_REQUEST_H__ */
