// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/loader/Loader.h>
#include <arc/ws-addressing/WSA.h>

#include "UNICOREClient.h"

namespace Arc {

  static const std::string BES_FACTORY_ACTIONS_BASE_URL("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/");
  static const std::string BES_MANAGEMENT_ACTIONS_BASE_URL("http://schemas.ggf.org/bes/2006/08/bes-management/BESManagementPortType/");

  static XMLNode find_xml_node(const XMLNode& node,
                               const std::string& el_name,
                               const std::string& attr_name,
                               const std::string& attr_value) {
    if (MatchXMLName(node, el_name) &&
        (((std::string)node.Attribute(attr_name)) == attr_value))
      return node;
    XMLNode cn = node[el_name];
    while (cn) {
      XMLNode fn = find_xml_node(cn, el_name, attr_name, attr_value);
      if (fn)
        return fn;
      cn = cn[1];
    }
    return XMLNode();
  }

  Logger UNICOREClient::logger(Logger::rootLogger, "UNICORE-Client");

  static void set_UNICORE_namespaces(NS& ns) {
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    ns["ns0"] = "urn:oasis:names:tc:SAML:2.0:assertion";
    ns["rp"] = "http://docs.oasis-open.org/wsrf/rp-2";
    ns["u6"] = "http://www.unicore.eu/unicore6";
    ns["jms"] = "http://unigrids.org/2006/04/services/jms";
  }

  static void set_bes_factory_action(SOAPEnvelope& soap, const char *op) {
    WSAHeader(soap).Action(BES_FACTORY_ACTIONS_BASE_URL + op);
  }

  // static void set_bes_management_action(SOAPEnvelope& soap,const char* op) {
  //   WSAHeader(soap).Action(BES_MANAGEMENT_ACTIONS_BASE_URL+op);
  // }

  UNICOREClient::UNICOREClient(const URL& url,
                               const MCCConfig& cfg,
                               int timeout)
    : client_config(cfg),
      client_loader(NULL),
      client(NULL),
      client_entry(NULL) {

    logger.msg(INFO, "Creating a UNICORE client");
    MCCConfig client_cfg(cfg);
    proxyPath = cfg.proxy;
    if (false) { //future test if proxy should be used or not
      client_cfg.AddProxy("");
    }
    client = new ClientSOAP(client_cfg, url, timeout);
    rurl = url;
    set_UNICORE_namespaces(unicore_ns);
  }

  UNICOREClient::~UNICOREClient() {
    if (client_loader)
      delete client_loader;
    if (client)
      delete client;
  }

  bool UNICOREClient::submit(const JobDescription& jobdesc, XMLNode& id,
                             bool delegate) {

    std::string faultstring;

    logger.msg(INFO, "Creating and sending request");

    // Create job request
    /*
       bes-factory:CreateActivity
         bes-factory:ActivityDocument
           jsdl:JobDefinition
     */
    PayloadSOAP req(unicore_ns);
    XMLNode op = req.NewChild("bes-factory:CreateActivity");
    XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    set_bes_factory_action(req, "CreateActivity");
    WSAHeader(req).To(rurl.str());
    //XMLNode proxyHeader = req.Header().NewChild("u6:Proxy");
    if (true) {
      std::string pem_str;
      std::ifstream proxy_file(proxyPath.c_str()/*, ifstream::in*/);
      std::getline<char>(proxy_file, pem_str, 0);
      req.Header().NewChild("u6:Proxy") = pem_str;
      //std::cout << "\n----\n" << "pem_str = " << pem_str << "\n----\n"; //debug code, remove!
    }
    //std::string jsdl_str;
    //std::getline<char>(jsdl_file, jsdl_str, 0);
    std::string jsdl_str = jobdesc.UnParse("ARCJSDL");
    XMLNode jsdl_doc = act_doc.NewChild(XMLNode(jsdl_str));
    //std::cout << "\n----\n" << jsdl_str << "\n----\n"; //Debug line to verify the activity document
    jsdl_doc.Namespaces(unicore_ns); // Unify namespaces
    PayloadSOAP *resp = NULL;

    XMLNode ds =
      act_doc["jsdl:JobDefinition"]["jsdl:JobDescription"]["jsdl:DataStaging"];
    for (; (bool)ds; ds = ds[1]) {
      // FilesystemName - ignore
      // CreationFlag - ignore
      // DeleteOnTermination - ignore
      XMLNode source = ds["jsdl:Source"];
      XMLNode target = ds["jsdl:Target"];
      if ((bool)source) {
        std::string s_name = ds["jsdl:FileName"];
        if (!s_name.empty()) {
          XMLNode x_url = source["jsdl:URI"];
          std::string s_url = x_url;
          if (s_url.empty())
            s_url = "./" + s_name;
          else {
            URL u_url(s_url);
            if (!u_url) {
              if (s_url[0] != '/')
                s_url = "./" + s_url;
            }
            else {
              if (u_url.Protocol() == "file") {
                s_url = u_url.Path();
                if (s_url[0] != '/')
                  s_url = "./" + s_url;
              }
              else
                s_url.resize(0);
            }
          }
          if (!s_url.empty())
            x_url.Destroy();
        }
      }
    }
    act_doc.GetXML(jsdl_str);
    logger.msg(DEBUG, "Job description to be sent: %s", jsdl_str);

    // Try to figure out which credentials are used
    // TODO: Method used is unstable beacuse it assumes some predefined
    // structure of configuration file. Maybe there should be some
    // special methods of ClientTCP class introduced.
    std::string deleg_cert;
    std::string deleg_key;
    if (delegate) {
      client->Load(); // Make sure chain is ready
      XMLNode tls_cfg = find_xml_node((client->GetConfig())["Chain"],
                                      "Component", "name", "tls.client");
      if (tls_cfg) {
        deleg_cert = (std::string)(tls_cfg["ProxyPath"]);
        if (deleg_cert.empty()) {
          deleg_cert = (std::string)(tls_cfg["CertificatePath"]);
          deleg_key = (std::string)(tls_cfg["KeyPath"]);
        }
        else
          deleg_key = deleg_cert;
      }
      if (deleg_cert.empty() || deleg_key.empty()) {
        logger.msg(ERROR, "Failed to find delegation credentials in "
                   "client configuration");
        return false;
      }
    }
    // Send job request + delegation
    if (client) {
      if (delegate) {
        DelegationProviderSOAP deleg(deleg_cert, deleg_key);
        logger.msg(INFO, "Initiating delegation procedure");
        if (!deleg.DelegateCredentialsInit(*(client->GetEntry()),
                                           &(client->GetContext()))) {
          logger.msg(ERROR, "Failed to initiate delegation");
          return false;
        }
        deleg.DelegatedToken(op);
      }
      MCC_Status status =
        client->process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                        "BESFactoryPortType/CreateActivity", &req, &resp);
      if (!status) {
        logger.msg(ERROR, "Submission request failed");
        return false;
      }
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://schemas.ggf.org/bes/2006/08/"
                         "bes-factory/BESFactoryPortType/CreateActivity");
      MessageAttributes attributes_rep;
      MessageContext context;

      if (delegate) {
        DelegationProviderSOAP deleg(deleg_cert, deleg_key);
        logger.msg(INFO, "Initiating delegation procedure");
        if (!deleg.DelegateCredentialsInit(*client_entry, &context)) {
          logger.msg(ERROR, "Failed to initiate delegation");
          return false;
        }
        deleg.DelegatedToken(op);
      }
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "Submission request failed");
        return false;
      }
      logger.msg(INFO, "Submission request succeed");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR, "There was no response to a submission request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "A response to a submission request was not "
                   "a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }
    //XMLNode id;
    SOAPFault fs(*resp);
    if (!fs) {
      (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
      //id.GetDoc(jobid);
      //std::cout << "\n---\nActivityIdentifier:\n" << (std::string)((*resp)["CreateActivityResponse"]["ActivityIdentifier"]) << "\n---\n";//debug code
      delete resp;

      UNICOREClient luc((std::string)id["Address"], client_config); //local unicore client
      //std::cout << "\n---\nid element containing (?) Job Address:\n" << (std::string)id << "\n---\n";//debug code
      return luc.uasStartJob();
      //return true;
    }
    else {
      faultstring = fs.Reason();
      std::string s;
      resp->GetXML(s);
      delete resp;
      logger.msg(DEBUG, "Submission returned failure: %s", s);
      logger.msg(ERROR, "Submission failed, service returned: %s", faultstring);
      return false;
    }
  }

  bool UNICOREClient::uasStartJob(){
        std::string state, faultstring;
    logger.msg(INFO, "Creating and sending a start job request");

    PayloadSOAP req(unicore_ns);
    XMLNode SOAPMethod =
      req.NewChild("jms:Start");
    WSAHeader(req).To(rurl.str());
    WSAHeader(req).Action("http://schemas.ggf.org/bes/2006/08/bes-activity/BESActivityPortType/StartRequest");

    // Send status request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status =
        client->process("http://schemas.ggf.org/bes/2006/08/bes-activity/BESActivityPortType/StartRequest",
                        &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://schemas.ggf.org/bes/2006/08/bes-activity/BESActivityPortType/StartRequest");
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "A start job request failed");
        return false;
      }
      logger.msg(INFO, "A start job request succeeded");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR,
                   "There was no response to a start job request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "The response of a start job request was "
                   "not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }
    SOAPFault fs(*resp);
    if (!fs) {
      return true;
    }
    else {
      faultstring = fs.Reason();
      std::string s;
      resp->GetXML(s);
      delete resp;
      logger.msg(DEBUG, "Submission returned failure: %s", s);
      logger.msg(ERROR, "Submission failed, service returned: %s", faultstring);
      return false;
    }


  }

  bool UNICOREClient::stat(const std::string& jobid, std::string& status) {

    std::string state, substate, faultstring;
    logger.msg(INFO, "Creating and sending a status request");

    PayloadSOAP req(unicore_ns);
    XMLNode jobref =
      req.NewChild("bes-factory:GetActivityStatuses").
      NewChild(XMLNode(jobid));
    set_bes_factory_action(req, "GetActivityStatuses");
    WSAHeader(req).To(rurl.str());

    // Send status request
    PayloadSOAP *resp = NULL;

    if (client) {
      MCC_Status status =
        client->process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                        "BESFactoryPortType/GetActivityStatuses", &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://schemas.ggf.org/bes/2006/08/"
                         "bes-factory/BESFactoryPortType/GetActivityStatuses");
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "A status request failed");
        return false;
      }
      logger.msg(INFO, "A status request succeed");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR, "There was no response to a status request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR,
                   "The response of a status request was not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }
    XMLNode st, fs;
    (*resp)["GetActivityStatusesResponse"]["Response"]
    ["ActivityStatus"].New(st);
    state = (std::string)st.Attribute("state");
    XMLNode sst;
    (*resp)["GetActivityStatusesResponse"]["Response"]
    ["ActivityStatus"]["state"].New(sst);
    substate = (std::string)sst;
    (*resp)["Fault"]["faultstring"].New(fs);
    faultstring = (std::string)fs;
    delete resp;
    if (faultstring != "") {
      logger.msg(ERROR, faultstring);
      return false;
    }
    else if (state == "") {
      logger.msg(ERROR, "The job status could not be retrieved");
      return false;
    }
    else {
      status = state + "/" + substate;
      return true;
    }
  }

  bool UNICOREClient::listTargetSystemFactories(std::list< std::pair<URL, ServiceType> >& tsf) {

    logger.msg(INFO, "Creating and sending an index service query");
    PayloadSOAP req(unicore_ns);
    XMLNode query = req.NewChild("rp:QueryResourceProperties");
    XMLNode exp = query.NewChild("rp:QueryExpression");
    exp.NewAttribute("Dialect") = "http://www.w3.org/TR/1999/REC-xpath-19991116";
    exp = "//*";
    PayloadSOAP *resp = NULL;
    client->process("http://docs.oasis-open.org/wsrf/rpw-2"
                    "/QueryResourceProperties/QueryResourcePropertiesRequest",
                    &req, &resp);
    if (resp == NULL) {
      logger.msg(ERROR, "There was no SOAP response");
      return false;
    }

    XMLNodeList memberServices = resp->Body().Path("QueryResourcePropertiesResponse/Entry/MemberServiceEPR");
    for (XMLNodeList::iterator it = memberServices.begin(); it != memberServices.end(); it++) {
      if (((std::string)(*it)["Metadata"]["InterfaceName"]).find("BESFactoryPortType") != std::string::npos) { // it.Metadata.InterfaceName should contain 'BESFactoryPortType'...
        tsf.push_back(std::pair<URL, ServiceType>(URL((std::string)(*it)["Address"]), COMPUTING));
      }
    }

    return true;
  }

  bool UNICOREClient::sstat(std::string& status) {

    std::string state, faultstring;
    logger.msg(INFO, "Creating and sending a service status request");

    PayloadSOAP req(unicore_ns);
    XMLNode jobref =
      req.NewChild("bes-factory:GetFactoryAttributesDocument");
    set_bes_factory_action(req, "GetFactoryAttributesDocument");
    WSAHeader(req).To(rurl.str());

    // Send status request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status =
        client->process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                        "BESFactoryPortType/GetFactoryAttributesDocument",
                        &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://schemas.ggf.org/bes/2006/08/"
                         "bes-factory/BESFactoryPortType/"
                         "GetFactoryAttributesDocument");
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "A service status request failed");
        return false;
      }
      logger.msg(INFO, "A service status request succeeded");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR,
                   "There was no response to a service status request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "The response of a service status request was "
                   "not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }
    XMLNode st;
    logger.msg(DEBUG, "Response:\n%s", (std::string)(*resp));
    (*resp)["GetFactoryAttributesDocumentResponse"]
    ["FactoryResourceAttributesDocument"].New(st);
    st.GetDoc(state, true);
    delete resp;
    if (state == "") {
      logger.msg(ERROR, "The service status could not be retrieved");
      return false;
    }
    else {
      status = state;
      return true;
    }
  }

  bool UNICOREClient::kill(const std::string& jobid) {

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to terminate a job");

    PayloadSOAP req(unicore_ns);
    XMLNode jobref =
      req.NewChild("bes-factory:TerminateActivities").
      NewChild(XMLNode(jobid));
    set_bes_factory_action(req, "TerminateActivities");
    WSAHeader(req).To(rurl.str());

    // Send kill request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status =
        client->process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                        "BESFactoryPortType/TerminateActivities", &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://schemas.ggf.org/bes/2006/08/"
                         "bes-factory/BESFactoryPortType/TerminateActivities");
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "A job termination request failed");
        return false;
      }
      logger.msg(INFO, "A job termination request succeed");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR,
                   "There was no response to a job termination request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "The response of a job termination request was "
                   "not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    XMLNode cancelled, fs;
    (*resp)["TerminateActivitiesResponse"]
    ["Response"]["Cancelled"].New(cancelled);
    result = (std::string)cancelled;
    (*resp)["Fault"]["faultstring"].New(fs);
    faultstring = (std::string)fs;
    delete resp;
    if (faultstring != "") {
      logger.msg(ERROR, faultstring);
      return false;
    }
    if (result != "true") {
      logger.msg(ERROR, "Job termination failed");
      return false;
    }
    return true;
  }

  bool UNICOREClient::clean(const std::string& jobid) {

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to terminate a job");

    PayloadSOAP req(unicore_ns);
    XMLNode op = req.NewChild("a-rex:ChangeActivityStatus");
    XMLNode jobref = op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Finished";
    jobstate.NewChild("a-rex:state") = "Deleted";
    // Send clean request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status = client->process("", &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "A job cleaning request failed");
        return false;
      }
      logger.msg(INFO, "A job cleaning request succeed");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR,
                   "There was no response to a job cleaning request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "The response of a job cleaning request was not "
                   "a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    if (!((*resp)["ChangeActivityStatusResponse"])) {
      delete resp;
      XMLNode fs;
      (*resp)["Fault"]["faultstring"].New(fs);
      faultstring = (std::string)fs;
      if (faultstring != "") {
        logger.msg(ERROR, faultstring);
        return false;
      }
      if (result != "true") {
        logger.msg(ERROR, "Job termination failed");
        return false;
      }
    }
    delete resp;
    return true;
  }

}
