#ifndef __ARC_SEC_XACMLREQUEST_H__
#define __ARC_SEC_XACMLREQUEST_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/Request.h>

//#include "ArcEvaluator.h"

///XACMLRequest, Parsing the xacml request format

namespace ArcSec {

class XACMLRequest : public Request {
//friend class ArcEvaluator;

public:
  /**Get the name of corresponding evaulator*/
  virtual const char* getEvalName() const { return NULL; };

  /**Get the name of this request*/
  virtual const char* getName() const { return NULL; };

  virtual Arc::XMLNode& getReqNode() { return reqnode; };

  static Arc::Plugin* get_request(Arc::PluginArgument* arg);

  //**Set the attribute factory for the usage of Request*/
  virtual void setAttributeFactory(AttributeFactory* attributefactory) { attrfactory = attributefactory; };

  //**Default constructor*/
  XACMLRequest ();

  //**Parse request information from external source*/
  XACMLRequest (const Source& source);

  virtual ~XACMLRequest();

  //**Create the objects included in Request according to the node attached to the Request object*/
  virtual void make_request();

private:
  //**AttributeFactory which is in charge of producing Attribute*/
  AttributeFactory * attrfactory;

  //**A XMLNode structure which includes the xml structure of a request*/
  Arc::XMLNode reqnode;

  Subject sub;
  Resource res;
  Action act;
  Context env;

protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLREQUEST_H__ */
