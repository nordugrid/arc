#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "../../hed/acc/ARC1/AREXClient.h"
#include "../../hed/libs/client/JobDescription.h"
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::MigrateActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out,const std::string& clientid) {
  /*
  MigrateActivity
    ActivityIdentifier (wsa:EndpointReferenceType)
    ActivityDocument
      jsdl:JobDefinition
    ForceMigration

  MigrateActivityResponse
    ActivityIdentifier (wsa:EndpointReferenceType)
    ActivityDocument
      jsdl:JobDefinition

  NotAuthorizedFault
  NotAcceptingNewActivitiesFault
  UnsupportedFeatureFault
  InvalidRequestMessageFault
  */
  {
    std::string s;
    in.GetXML(s);
    logger_.msg(Arc::DEBUG, "MigrateActivity: request = \n%s", s);
  };
  Arc::WSAEndpointReference id(in["ActivityIdentifier"]);
  if(!(Arc::XMLNode)id) {
    // Wrong request
    logger_.msg(Arc::ERROR, "MigrateActivitys: no ActivityIdentifier found");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find ActivityIdentifier element in request");
    InvalidRequestMessageFault(fault,"jsdl:ActivityIdentifier","Element is missing");
    out.Destroy();
    return Arc::MCC_Status();
  };
  std::string migrateid = Arc::WSAEndpointReference(id).Address() + "/" + (std::string)Arc::WSAEndpointReference(id).ReferenceParameters()["a-rex:JobID"];
  if(migrateid.empty()) {
    // EPR is wrongly formated or not an A-REX EPR
    logger_.msg(Arc::ERROR, "MigrateActivity: EPR contains no JobID");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find JobID element in ActivityIdentifier");
    InvalidRequestMessageFault(fault,"a-rex:JobID","Element is missing");
    out.Destroy();
    return Arc::MCC_Status();
  };

  // HPC Basic Profile 1.0 comply (these fault handlings are defined in the KnowARC standards 
  // conformance roadmap 2nd release)

 // End of the HPC BP 1.0 fault handling part

  std::string delegation;
  Arc::XMLNode delegated_token = in["deleg:DelegatedToken"];
  if(delegated_token) {
    // Client wants to delegate credentials
    if(!delegations_.DelegatedToken(delegation,delegated_token)) {
      // Failed to accept delegation (report as bad request)
      logger_.msg(Arc::ERROR, "MigrateActivity: Failed to accept delegation");
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Failed to accept delegation");
      InvalidRequestMessageFault(fault,"deleg:DelegatedToken","This token does not exist");
      out.Destroy();
      return Arc::MCC_Status();
    };
  };

  if( !(in["ActivityDocument"]["JobDefinition"])) {
    /*
    // First try to get job desc from old cluster
    logger_.msg(Arc::DEBUG, "MigrateActivity: no job description found try to get it from old cluster");
    Arc::MCCConfig cfg;
    // TODO:
    //if (!proxyPath.empty())
    cfg.AddProxy(delegation);
    //if (!certificatePath.empty())
      //cfg.AddCertificate(certificatePath);
    //if (!keyPath.empty())
      //cfg.AddPrivateKey(keyPath);
    //if (!caCertificatesDir.empty())
      //cfg.AddCADir(caCertificatesDir);
    Arc::URL url(migrateid);
    Arc::PathIterator pi(url.Path(), true);
    url.ChangePath(*pi);
    Arc::AREXClient ac(url, cfg);
    Arc::NS ns;
    ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    Arc::XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    std::string desc_str;
    if (ac.getdesc(idstr,desc_str)){
      Arc::JobDescription desc;
      desc.setSource(desc_str);
      if (desc.isValid()) {
        logger_.msg(Arc::INFO,"Valid job description obtained");
        if ( !( in["ActivityDocument"] ) ) in.NewChild("bes-factory:ActivityDocument");
        Arc::XMLNode XMLdesc;
        desc.getXML(XMLdesc);
        in["ActivityDocument"].NewChild(XMLdesc);
      } else {
        // Wrongly formatted job description
        logger_.msg(Arc::ERROR, "MigrateActivity: job description could not be fetch from old cluster");
        Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find JobDefinition element in request");
        InvalidRequestMessageFault(fault,"jsdl:JobDefinition","Element is missing");
        out.Destroy();
        return Arc::MCC_Status();
      }
    }
    */
    //else {
      // Not able to get job description
      logger_.msg(Arc::ERROR, "MigrateActivity: no job description found");
      //logger_.msg(Arc::ERROR, "MigrateActivity: job description could not be fetch from old cluster");
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find JobDefinition element in request");
      InvalidRequestMessageFault(fault,"jsdl:JobDefinition","Element is missing");
      out.Destroy();
      return Arc::MCC_Status();
    //}
  };

  Arc::XMLNode jsdl = in["ActivityDocument"]["JobDefinition"];

  Arc::NS ns;
  // Creating migration XMLNode
  Arc::XMLNode migration(ns, "Migration");
  migration.NewChild("ActivityIdentifier") = migrateid;
  if( (bool)in["ForceMigration"]){
    migration.NewChild("ForceMigration") = (std::string)in["ForceMigration"];
  } else {
    migration.NewChild("ForceMigration") = "true";
  }

  std::string migrationStr;
  migration.GetDoc(migrationStr, true);
  logger_.msg(Arc::INFO, "Migration XML sent to AREXJob: %s", migrationStr);

  ARexJob job(jsdl,config,delegation,clientid,logger_,migration);
  if(!job) {
    ARexJobFailure failure_type = job;
    std::string failure = job.Failure();
    switch(failure_type) {
      case ARexJobDescriptionUnsupportedError: {
        Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Unsupported feature in job description");
        UnsupportedFeatureFault(fault,failure);
      }; break;
      case ARexJobDescriptionMissingError: {
        Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Missing needed element in job description");
        UnsupportedFeatureFault(fault,failure);
      }; break;
      case ARexJobDescriptionLogicalError: {
        std::string element;
        std::string::size_type pos = failure.find(' ');
        if(pos != std::string::npos) {
          element=failure.substr(0,pos);
          failure=failure.substr(pos+1);
        };
        Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Logical error in job description");
        InvalidRequestMessageFault(fault,element,failure);
      }; break;
      default: {
        logger_.msg(Arc::ERROR, "MigrateActivity: Failed to migrate new job: %s",failure);
        // Failed to migrate new job (no corresponding BES fault defined - using generic SOAP error)
        logger_.msg(Arc::ERROR, "MigrateActivity: Failed to migrate new job");
        Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,("Failed to migrate new activity: "+failure).c_str());
        GenericFault(fault);
      };
    };
    out.Destroy();
    return Arc::MCC_Status();
  };

  // Make SOAP response
  Arc::WSAEndpointReference identifier(out.NewChild("bes-factory:ActivityIdentifier"));
  // Make job's ID
  identifier.Address(config.Endpoint()); // address of service
  identifier.ReferenceParameters().NewChild("a-rex:JobID")=job.ID();
  identifier.ReferenceParameters().NewChild("a-rex:JobSessionDir")=config.Endpoint()+"/"+job.ID();
  out.NewChild(in["ActivityDocument"]);
  logger_.msg(Arc::DEBUG, "MigrateActivity finished successfully");
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::DEBUG, "MigrateActivity: response = \n%s", s);
  };
  /* Needs to kill old job */

  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

