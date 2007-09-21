#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::GetFactoryAttributesDocument(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  GetFactoryAttributesDocument

  GetFactoryAttributesDocumentResponse
    FactoryResourceAttributesDocument
      BasicResourceAttributesDocument (optional)
      IsAcceptingNewActivities (boolean)
      CommonName (optional,string)
      LongDescription (optional,string)
      TotalNumberOfActivities (long)
      ActivityReference (wsa:EndpointReferenceType,unbounded)
      TotalNumberOfContainedResources (long)
      ContainedResource (anyType,unbounded)
      NamingProfile (anyURI,unbounded)
      BESExtension (anyURI,unbounded)
      LocalResourceManagerType (anyURI)
  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::DEBUG, "GetFactoryAttributesDocument: request = \n%s", s.c_str());
  };
  Arc::XMLNode doc = out.NewChild("bes-factory:FactoryResourceAttributesDocument");
  //doc.NewChild("bes-factory:BasicResourceAttributesDocument");
  doc.NewChild("bes-factory:IsAcceptingNewActivities")="true";
  //doc.NewChild("bes-factory:CommonName")="";
  //doc.NewChild("bes-factory:LongDescription")="";
  std::list<std::string> jobs = ARexJob::Jobs(config);
  doc.NewChild("bes-factory:TotalNumberOfActivities")=Arc::tostring(jobs.size());
  for(std::list<std::string>::iterator j = jobs.begin();j!=jobs.end();++j) {
    Arc::WSAEndpointReference identifier(doc.NewChild("bes-factory:ActivityReference"));
    // Make job's ID
    identifier.Address(config.Endpoint()); // address of service
    identifier.ReferenceParameters().NewChild("a-rex:JobID")=(*j);
    identifier.ReferenceParameters().NewChild("a-rex:JobSessionDir")=config.Endpoint()+"/"+(*j);
  };
  doc.NewChild("bes-factory:TotalNumberOfContainedResources")=Arc::tostring(0);
  doc.NewChild("bes-factory:NamingProfile")="http://schemas.ggf.org/bes/2006/08/bes/naming/BasicWSAddressing";
  doc.NewChild("bes-factory:BESExtension")="http://www.nordugrid.org/schemas/a-rex";
  doc.NewChild("bes-factory:LocalResourceManagerType")="uri:uknown";
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "GetFactoryAttributesDocument: response = \n%s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

