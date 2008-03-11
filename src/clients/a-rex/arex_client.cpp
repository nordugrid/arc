// arex_client.cpp

#include <arc/delegation/DelegationInterface.h>

#include "arex_client.h"

namespace Arc {

  // TODO: probably worth moving it to common library
  // Of course xpath can be used too. But such solution is probably an overkill.
  static Arc::XMLNode find_xml_node(const Arc::XMLNode& node,const std::string& el_name,
                                    const std::string& attr_name,const std::string& attr_value) {
    if(MatchXMLName(node,el_name) && 
       (((std::string)node.Attribute(attr_name)) == attr_value)) return node;
    XMLNode cn = node[el_name];
    while(cn) {
      XMLNode fn = find_xml_node(cn,el_name,attr_name,attr_value);
      if(fn) return fn;
      cn=cn[1];
    };
    return XMLNode();
  }

  AREXClientError::AREXClientError(const std::string& what) :
    std::runtime_error(what)
  {
  }

  Arc::Logger AREXClient::logger(Arc::Logger::rootLogger, "A-REX-Client");

  static void set_arex_namespaces(Arc::NS& ns) {
    ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";    
    ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
  }

  AREXClient::AREXClient(std::string configFile) throw(AREXClientError)
    :client_config(NULL),client_loader(NULL),client(NULL),client_entry(NULL)
  {
    logger.msg(Arc::INFO, "Creating an A-REX client.");

    if (configFile=="" && getenv("ARC_AREX_CONFIG"))
      configFile = getenv("ARC_AREX_CONFIG");
    if (configFile=="")
      configFile = "./arex_client.xml";

    client_config = new Arc::Config(configFile.c_str());
    if(!*client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration.");
      throw AREXClientError("Failed to load client configuration.");
    }

    client_loader = new Arc::Loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded.");
    client_entry = (*client_loader)["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point.");
      throw AREXClientError("Client chain does not have entry point.");
    }

    set_arex_namespaces(arex_ns);
  }
  
  AREXClient::AREXClient(const Arc::URL& url,
                         const Arc::MCCConfig& cfg) throw(AREXClientError)
    :client_config(NULL),client_loader(NULL),client(NULL),client_entry(NULL) {

    logger.msg(Arc::INFO, "Creating an A-REX client.");
    client = new Arc::ClientSOAP(cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
    set_arex_namespaces(arex_ns);
  }
  
  AREXClient::~AREXClient()
  {
    if(client_loader) delete client_loader;
    if(client_config) delete client_config;
    if(client) delete client;
  }
  
  std::string AREXClient::submit(std::istream& jsdl_file,AREXFileList& file_list,bool delegate)
    throw(AREXClientError)
  {
    std::string jobid, faultstring;
    file_list.resize(0);

    logger.msg(Arc::INFO, "Creating and sending request.");

    // Create job request
    /*
      bes-factory:CreateActivity
        bes-factory:ActivityDocument
          jsdl:JobDefinition
    */
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode op = req.NewChild("bes-factory:CreateActivity");
    Arc::XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    std::string jsdl_str; 
    std::getline<char>(jsdl_file,jsdl_str,0);
    act_doc.NewChild(Arc::XMLNode(jsdl_str));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces
    Arc::PayloadSOAP* resp = NULL;

    XMLNode ds = act_doc["jsdl:JobDefinition"]["jsdl:JobDescription"]["jsdl:DataStaging"];
    for(;(bool)ds;ds=ds[1]) {
      // FilesystemName - ignore
      // CreationFlag - ignore
      // DeleteOnTermination - ignore
      XMLNode source = ds["jsdl:Source"];
      XMLNode target = ds["jsdl:Target"];
      if((bool)source) {
        std::string s_name = ds["jsdl:FileName"];
        if(!s_name.empty()) {
          XMLNode x_url = source["jsdl:URI"];
          std::string s_url = x_url;
          if(s_url.empty()) {
            s_url="./"+s_name;
          } else {
            URL u_url(s_url);
            if(!u_url) {
              if(s_url[0] != '/') s_url="./"+s_url;
            } else {
              if(u_url.Protocol() == "file") {
                s_url=u_url.Path();
                if(s_url[0] != '/') s_url="./"+s_url;
              } else {
                s_url.resize(0);
              };
            };
          };
          if(!s_url.empty()) {
            x_url.Destroy();
            AREXFile file(s_name,s_url);
            file_list.push_back(file);
          };
        };
      };
    }; 
    act_doc.GetXML(jsdl_str);
    logger.msg(Arc::VERBOSE, "Job description to be sent: %s",jsdl_str.c_str());

    // Try to figure out which credentials are used
    // TODO: Method used is unstable beacuse it assumes some predefined 
    // structure of configuration file. Maybe there should be some 
    // special methods of ClientTCP class introduced.
    std::string deleg_cert;
    std::string deleg_key;
    if(delegate) {
      client->Load(); // Make sure chain is ready
      Arc::XMLNode tls_cfg = find_xml_node((client->GetConfig())["Chain"],"Component","name","tls.client");
      if(tls_cfg) {
        deleg_cert=(std::string)(tls_cfg["ProxyPath"]);
        if(deleg_cert.empty()) {
          deleg_cert=(std::string)(tls_cfg["CertificatePath"]);
          deleg_key=(std::string)(tls_cfg["KeyPath"]);
        } else {
          deleg_key=deleg_cert;
        };
      };
      if(deleg_cert.empty() || deleg_key.empty()) {
std::string s;
client->GetConfig().GetXML(s);
std::cerr<<s<<std::endl;
        logger.msg(Arc::ERROR,"Failed to find delegation credentials in client configuration.");
        throw AREXClientError("Failed to find delegation credentials in client configuration.");
      };
    };
    // Send job request + delegation
    if(client) {
      {
        if(delegate) {
          Arc::DelegationProviderSOAP deleg(deleg_cert,deleg_key);
          logger.msg(Arc::INFO, "Initiating delegation procedure");
          if(!deleg.DelegateCredentialsInit(*(client->GetEntry()),&(client->GetContext()))) {
            logger.msg(Arc::ERROR,"Failed to initiate delegation.");
            throw AREXClientError("Failed to initiate delegation.");
          };
          deleg.DelegatedToken(op);
        };
      };
      Arc::MCC_Status status = client->process(
         "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity",
         &req,&resp);
      if(!status) {
        logger.msg(Arc::ERROR, "Submission request failed.");
        throw AREXClientError("Submission request failed.");
      }
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      };
    } else if (client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      {
        if(delegate) {
          Arc::DelegationProviderSOAP deleg(deleg_cert,deleg_key);
          logger.msg(Arc::INFO, "Initiating delegation procedure");
          if(!deleg.DelegateCredentialsInit(*client_entry,&context)) {
            logger.msg(Arc::ERROR,"Failed to initiate delegation.");
            throw AREXClientError("Failed to initiate delegation.");
          };
          deleg.DelegatedToken(op);
        };
      };
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "Submission request failed.");
        throw AREXClientError("Submission request failed.");
      }
      logger.msg(Arc::INFO, "Submission request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There were no response to a submission request.");
        throw AREXClientError("There were no response to the submission request.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"A response to a submission request was not a SOAP message.");
        delete repmsg.Payload();
        throw AREXClientError("The response to the submission request was not a SOAP message.");
      };
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };
    Arc::XMLNode id, fs;
    (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
    (*resp)["Fault"]["faultstring"].New(fs);
    id.GetDoc(jobid);
    faultstring=(std::string)fs;
    delete resp;
    if (faultstring=="")
      return jobid;
    else
      throw AREXClientError(faultstring);
  }
  
  std::string AREXClient::stat(const std::string& jobid)
    throw(AREXClientError)
  {
    std::string state, substate, faultstring;
    logger.msg(Arc::INFO, "Creating and sending a status request.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:GetActivityStatuses").
      NewChild(Arc::XMLNode(jobid));
    
    // Send status request
    Arc::PayloadSOAP* resp = NULL;

    if(client) {
      Arc::MCC_Status status = client->process(
          "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetActivityStatuses",
          &req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetActivityStatuses");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A status request failed.");
        throw AREXClientError("The status request failed.");
      }
      logger.msg(Arc::INFO, "A status request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There were no response to a status request.");
        throw AREXClientError("There were no response.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
		 "The response of a status request was not a SOAP message.");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };
    Arc::XMLNode st, fs;
    (*resp)["GetActivityStatusesResponse"]["Response"]
           ["ActivityStatus"].New(st);
    state = (std::string)st.Attribute("state");
    Arc::XMLNode sst;
    (*resp)["GetActivityStatusesResponse"]["Response"]
           ["ActivityStatus"]["state"].New(sst);
    substate = (std::string)sst;
    (*resp)["Fault"]["faultstring"].New(fs);
    faultstring=(std::string)fs;
    delete resp;
    if (faultstring!="")
      throw AREXClientError(faultstring);
    else if (state=="")
      throw AREXClientError("The job status could not be retrieved.");
    else
      return state+"/"+substate;
  }
  
  std::string AREXClient::sstat(void)
    throw(AREXClientError)
  {
    std::string state, faultstring;
    logger.msg(Arc::INFO, "Creating and sending a service status request.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:GetFactoryAttributesDocument");
    
    // Send status request
    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process(
         "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetFactoryAttributesDocument",
         &req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetFactoryAttributesDocument");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A service status request failed.");
        throw AREXClientError("The service status request failed.");
      }
      logger.msg(Arc::INFO, "A service status request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There were no response to a service status request.");
        throw AREXClientError("There were no response.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
		 "The response of a servicee status request was not a SOAP message.");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };
    Arc::XMLNode st;
    (*resp)["GetFactoryAttributesDocumentResponse"]
           ["FactoryResourceAttributesDocument"].New(st);
    st.GetDoc(state);
    delete resp;
    if (state=="")
      throw AREXClientError("The service status could not be retrieved.");
    else
      return state;
  }

  void AREXClient::kill(const std::string& jobid)
    throw(AREXClientError)
  {
    std::string result, faultstring;
    logger.msg(Arc::INFO, "Creating and sending request to terminate a job.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:TerminateActivities").
      NewChild(Arc::XMLNode(jobid));
    
    // Send kill request
    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process(
         "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/TerminateActivities",
         &req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/TerminateActivities");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A job termination request failed.");
        throw AREXClientError("The job termination request failed.");
      }
      logger.msg(Arc::INFO, "A job termination request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR,
		 "There was no response to a job termination request.");
        throw AREXClientError
	("There was no response to the job termination request.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
	"The response of a job termination request was not a SOAP message");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };

    Arc::XMLNode cancelled, fs;
    (*resp)["TerminateActivitiesResponse"]
           ["Response"]["Cancelled"].New(cancelled);
    result = (std::string)cancelled;
    (*resp)["Fault"]["faultstring"].New(fs);
    faultstring=(std::string)fs;
    delete resp;
    if (faultstring!="")
      throw AREXClientError(faultstring);
    if (result!="true")
      throw AREXClientError("Job termination failed.");
  }
  
  void AREXClient::clean(const std::string& jobid)
    throw(AREXClientError)
  {
    std::string result, faultstring;
    logger.msg(Arc::INFO, "Creating and sending request to terminate a job.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode op = req.NewChild("a-rex:ChangeActivityStatus");
    Arc::XMLNode jobref = op.NewChild(Arc::XMLNode(jobid));
    Arc::XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state")="Finished";
    jobstate.NewChild("a-rex:state")="Deleted";
    // Send clean request
    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process("",&req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A job cleaning request failed.");
        throw AREXClientError("The job cleaning request failed.");
      }
      logger.msg(Arc::INFO, "A job cleaning request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR,
		 "There was no response to a job cleaning request.");
        throw AREXClientError
	("There was no response to the job cleaning request.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
        "The response of a job cleaning request was not a SOAP message");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };

    if(!((*resp)["ChangeActivityStatusResponse"])) {
      delete resp;
      Arc::XMLNode fs;
      (*resp)["Fault"]["faultstring"].New(fs);
      faultstring=(std::string)fs;
      if (faultstring!="")
        throw AREXClientError(faultstring);
      if (result!="true")
        throw AREXClientError("Job termination failed.");
    };
    delete resp;
  }

}
