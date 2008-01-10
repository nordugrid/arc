#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/loader/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/Thread.h>

#include "arex2.h"

namespace Sched {

static Arc::LogStream logcerr(std::cerr);

// Static initializator
static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new SchedService(cfg);
}

Arc::MCC_Status SchedService::CreateActivity(Arc::XMLNode /*in*/,
                                             Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::GetActivityStatuses(Arc::XMLNode /*in*/,
                                                  Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::TerminateActivities(Arc::XMLNode /*in*/,
                                                 Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::GetActivityDocuments(Arc::XMLNode /*in*/,
                                                   Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::GetFactoryAttributesDocument(Arc::XMLNode /*in*/,
                                                          Arc::XMLNode /*out*/)
{
    return Arc::MCC_Status();
}


Arc::MCC_Status SchedService::StopAcceptingNewActivities(Arc::XMLNode /*in*/, 
                                                         Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::StartAcceptingNewActivities(Arc::XMLNode /*in*/,
                                                          Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::ChangeActivityStatus(Arc::XMLNode /*in*/,
                                                   Arc::XMLNode /*out*/) 
{
    return Arc::MCC_Status();
}

// Create Faults
Arc::MCC_Status SchedService::make_soap_fault(Arc::Message& outmsg) 
{
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status SchedService::make_fault(Arc::Message& /*outmsg*/) 
{
  return Arc::MCC_Status();
}

Arc::MCC_Status SchedService::make_response(Arc::Message& outmsg) 
{
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

// Main process 
Arc::MCC_Status SchedService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger_.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };

    // Get operation
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      logger_.msg(Arc::ERROR, "input does not define operation");
      return make_soap_fault(outmsg);
    };
    logger_.msg(Arc::DEBUG, "process: operation: %s",op.Name().c_str());
    // BES Factory operations
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
    Arc::PayloadSOAP& res = *outpayload;
    Arc::MCC_Status ret;
    if(MatchXMLName(op, "CreateActivity")) {
        res.NewChild("bes-factory:CreateActivityResponse");
        ret = CreateActivity(op, res);
    } else if(MatchXMLName(op, "GetActivityStatuses")) {
        res.NewChild("bes-factory:GetActivityStatusesResponse");
        ret = GetActivityStatuses(op, res);
    } else if(MatchXMLName(op, "TerminateActivities")) {
        res.NewChild("bes-factory:TerminateActivitiesResponse");
        ret = TerminateActivities(op, res);
    } else if(MatchXMLName(op, "GetActivityDocuments")) {
        res.NewChild("bes-factory:GetActivityDocumentsResponse");
        ret = GetActivityDocuments(op, res);
    } else if(MatchXMLName(op, "GetFactoryAttributesDocument")) {
        res.NewChild("bes-factory:GetFactoryAttributesDocumentResponse");
        ret = GetFactoryAttributesDocument(op, res);
    } else if(MatchXMLName(op, "StopAcceptingNewActivities")) {
        res.NewChild("bes-factory:StopAcceptingNewActivitiesResponse");
        ret = StopAcceptingNewActivities(op, res);
    } else if(MatchXMLName(op, "StartAcceptingNewActivities")) {
        res.NewChild("bes-factory:StartAcceptingNewActivitiesResponse");
        ret = StartAcceptingNewActivities(op, res);
    } else if(MatchXMLName(op, "ChangeActivityStatus")) {
        res.NewChild("bes-factory:ChangeActivityStatusResponse");
        ret = ChangeActivityStatus(op, res);
      // Delegation
    } else if(MatchXMLName(op, "DelegateCredentialsInit")) {
        if(!delegations_.DelegateCredentialsInit(*inpayload,*outpayload)) {
          delete inpayload;
          return make_soap_fault(outmsg);
        };
      // WS-Property
    } else if(MatchXMLNamespace(op,"http://docs.oasis-open.org/wsrf/rp-2")) {
        Arc::SOAPEnvelope* out_ = infodoc_.Process(*inpayload);
        if(out_) {
          *outpayload=*out_;
          delete out_;
        } else {
          delete inpayload; delete outpayload;
          return make_soap_fault(outmsg);
        };
    } else {
        logger_.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name().c_str());
        return make_soap_fault(outmsg);
    };
    {
        // DEBUG 
        std::string str;
        outpayload->GetXML(str);
        logger_.msg(Arc::DEBUG, "process: response=%s",str.c_str());
    };
    // Set output
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

// Start information collector 
static void thread_starter(void* arg) 
{
  if(!arg) return;
  ((SchedService*)arg)->InformationCollector();
}

// Constructor

SchedService::SchedService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "Sched") 
{
  logger_.addDestination(logcerr);
  // Define supported namespaces
  ns_["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
  ns_["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
  ns_["deleg"]="http://www.nordugrid.org/schemas/delegation";
  ns_["wsa"]="http://www.w3.org/2005/08/addressing";
  ns_["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
  ns_["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns_["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
  ns_["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  // CreateThreadFunction(&thread_starter,this);
}

// Destructor
SchedService::~SchedService(void) 
{
    // NOP
}

} // namespace Sched

service_descriptors ARC_SERVICE_LOADER = {
    { "sched", 0, &Sched::get_service },
    { NULL, 0, NULL }
};

