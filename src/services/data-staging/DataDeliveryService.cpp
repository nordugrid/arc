#include <sys/stat.h>

#include <arc/message/MessageAttributes.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>

#include <arc/GUID.h>
#include <arc/FileUtils.h>
#include <arc/Utils.h>

#include "DataDeliveryService.h"

namespace DataStaging {

  static Arc::Plugin *get_service(Arc::PluginArgument* arg)
  {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    DataDeliveryService* s = new DataDeliveryService((Arc::Config*)(*srvarg));
    if (*s)
      return s;
    delete s;
    return NULL;
  }

  Arc::Logger DataDeliveryService::logger(Arc::Logger::rootLogger, "DataDeliveryService");


  void DataDeliveryService::ArchivalThread(void* arg) {
    DataDeliveryService* service = (DataDeliveryService*)arg;
    service->ArchivalThread();
  }

  void DataDeliveryService::ArchivalThread() {
    // archive every 10 mins DTRs older than 1 hour
    // TODO: configurable, save to disk?
    int frequency = 600;

    while (true) {
      sleep(frequency);
      Arc::Time timelimit(Arc::Time()-Arc::Period(3600));

      active_dtrs_lock.lock();
      for (std::map<DTR*, std::stringstream*>::iterator i = active_dtrs.begin();
           i != active_dtrs.end(); ++i) {

        DTR* dtr = i->first;

        if (dtr->get_modification_time() < timelimit) {
          if (dtr->error()) {
            logger.msg(Arc::VERBOSE, "Archiving DTR %s, state ERROR", dtr->get_id());
            archived_dtrs[dtr->get_id()] = "ERROR";
          }
          else {
            logger.msg(Arc::VERBOSE, "Archiving DTR %s, state %s", dtr->get_id(), dtr->get_status().str());
            archived_dtrs[dtr->get_id()] = dtr->get_status().str();
          }
          // clean up DTR memory - delete DTR Logger and LogDestinations
          delete i->second;
          const std::list<Arc::LogDestination*> log_dests = dtr->get_logger()->getDestinations();
          for (std::list<Arc::LogDestination*>::const_iterator ld = log_dests.begin(); ld != log_dests.end(); ++ld)
            delete *ld;
          delete dtr->get_logger();
          delete dtr;

          active_dtrs.erase(i);
        }
      }
      active_dtrs_lock.unlock();
    }

  }

  void DataDeliveryService::receiveDTR(DTR& dtr) {
    // note: logger doesn't work here - to fix
    logger.msg(Arc::INFO, "Received DTR %s in state %s", dtr.get_id(), dtr.get_status().str());

    // delete temp proxy file
    std::string proxy_file(tmp_proxy_dir+"/DTR."+dtr.get_parent_job_id()+".proxy");
    logger.msg(Arc::DEBUG, "Removing temp proxy %s", proxy_file);
    if (unlink(proxy_file.c_str()) && errno != ENOENT) {
      logger.msg(Arc::WARNING, "Failed to remove temporary proxy %s: %s", proxy_file, Arc::StrError(errno));
    }
    --current_processes;
  }

  /*
   Accepts:
   <DataDeliveryStart>
     <DTR>
       <ID>id</ID>
       <Source>url</Source>
       <Destination>url</Destination>
       <Caching>true</Caching>
       <CheckSum>adler32:12345678</CheckSum>
       <MinAverageSpeed>100</MinAverageSpeed>
       <AverageTime>60</AverageTime>
       <MinCurrentSpeed>100</MinCurrentSpeed>
       <MaxInactivityTime>120</MaxInactivityTime>
     </DTR>
     <DTR>
      ...
   </DataDeliveryStart>

   Returns
   <DataDeliveryStartResponse>
     <DataDeliveryStartResult>
       <Result>
         <ID>id</ID>
         <ReturnCode>SERVICE_ERROR</ReturnCode>
         <ErrorDescription>...</ErrorDescription>
       </Result>
       ...
     </DataDeliveryStartResult>
   </DataDeliveryStartResponse>
   */
  Arc::MCC_Status DataDeliveryService::Start(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& user) {

    Arc::XMLNode resp = out.NewChild("DataDeliveryStartResponse");
    Arc::XMLNode results = resp.NewChild("DataDeliveryStartResult");

    // Save credentials to temp file and set in UserConfig
    Arc::XMLNode delegated_token = in["DataDeliveryStart"]["deleg:DelegatedToken"];
    if (!delegated_token) {
      logger.msg(Arc::ERROR, "No delegation token in request");
      return Arc::MCC_Status(Arc::GENERIC_ERROR, "DataDeliveryService", "No delegation token received");
    }

    // Check credentials were already delegated
    std::string credential;
    if (!delegation.DelegatedToken(credential, delegated_token)) {
      // Failed to accept delegation
      logger.msg(Arc::ERROR, "Failed to accept delegation");
      return Arc::MCC_Status(Arc::GENERIC_ERROR, "DataDeliveryService", "Failed to accept delegation");
    }

    // Store proxy, only readable by user. Use DTR job id as proxy name.
    std::string groupid(Arc::UUID());
    std::string proxy_file(tmp_proxy_dir+"/DTR."+groupid+".proxy");
    logger.msg(Arc::VERBOSE, "Storing temp proxy at %s", proxy_file);

    if (!Arc::FileCreate(proxy_file, credential)) {
      logger.msg(Arc::ERROR, "Failed to create temp proxy at %s: %s", proxy_file, Arc::StrError(errno));
      return Arc::MCC_Status(Arc::GENERIC_ERROR, "DataDeliveryService", "Failed to create temp proxy");
    }

    if (chown(proxy_file.c_str(), user.get_uid(), user.get_gid()) != 0) {
      logger.msg(Arc::ERROR, "Failed to change owner of temp proxy at %s to %i:%i: %s",
                 proxy_file, user.get_uid(), user.get_gid(), Arc::StrError(errno));
      return Arc::MCC_Status(Arc::GENERIC_ERROR, "DataDeliveryService", "Failed to create temp proxy");
    }

    Arc::UserConfig usercfg;
    usercfg.ProxyPath(proxy_file);

    for(int n = 0;;++n) {
      Arc::XMLNode dtrnode = in["DataDeliveryStart"]["DTR"][n];

      if (!dtrnode) break;

      std::string dtrid((std::string)dtrnode["ID"]);
      std::string src((std::string)dtrnode["Source"]);
      std::string dest((std::string)dtrnode["Destination"]);

      Arc::XMLNode resultelement = results.NewChild("Result");
      resultelement.NewChild("ID") = dtrid;

      if (current_processes >= max_processes) {
        logger.msg(Arc::WARNING, "All %u process slots used", max_processes);
        resultelement.NewChild("ResultCode") = "SERVICE_ERROR";
        resultelement.NewChild("ErrorDescription") = "No free process slot available";
        continue;
      }

      // Logger for this DTR. Uses a string stream so log can easily be sent
      // back to the client. LogStream keeps a reference to the stream so we
      // cannot delete it until deleting LogStream. These pointers are
      // deleted when the DTR is archived.
      std::stringstream * stream = new std::stringstream();
      Arc::LogDestination * output = new Arc::LogStream(*stream);
      Arc::Logger * log = new Arc::Logger(Arc::Logger::getRootLogger(), "DataStaging");
      log->removeDestinations();
      log->addDestination(*output);

      DTR * dtr = new DTR(src, dest, usercfg, groupid, user.get_uid(), log);
      if (!(*dtr)) {
        logger.msg(Arc::ERROR, "Invalid DTR");
        resultelement.NewChild("ResultCode") = "SERVICE_ERROR";
        resultelement.NewChild("ErrorDescription") = "Could not create DTR";
        continue;
      }
      ++current_processes;

      // Set source checksum to validate against
      if (dtrnode["CheckSum"]) dtr->get_source()->SetCheckSum((std::string)dtrnode["CheckSum"]);

      // Get the callbacks sent to Scheduler and connect Delivery
      dtr->registerCallback(this, SCHEDULER);
      dtr->registerCallback(&delivery, DELIVERY);

      // TODO transfer parameters, caching

      dtr->set_id(dtrid);
      dtr->set_status(DTRStatus::TRANSFER);

      dtr->push(DELIVERY);

      active_dtrs_lock.lock();
      active_dtrs[dtr] = stream;
      active_dtrs_lock.unlock();

      resultelement.NewChild("ResultCode") = "OK";
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  /*
   Accepts:
   <DataDeliveryQuery>
     <DTR>
       <ID>id</ID>
     </DTR>
     <DTR>
     ...
   </DataDeliveryQuery>

   Returns:
   <DataDeliveryQueryResponse>
     <DataDeliveryQueryResult>
       <Result>
         <ID>id</ID>
         <ReturnCode>ERROR</ReturnCode>
         <ErrorDescription>...</ErrorDescription>
         <ErrorStatus>2</ErrorStatus>
         <ErrorLocation>1</ErrorLocation>
         <Log>...</Log>
         <BytesTransferred>1234</BytesTransferred>
         <Checksum>adler32:a123a45</Checksum>
       </Result>
       ...
     </DataDeliveryQueryResult>
   </DataDeliveryQueryResponse>
   */
  Arc::MCC_Status DataDeliveryService::Query(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& user) {

    Arc::XMLNode resp = out.NewChild("DataDeliveryQueryResponse");
    Arc::XMLNode results = resp.NewChild("DataDeliveryQueryResult");

    for(int n = 0;;++n) {
      Arc::XMLNode dtrnode = in["DataDeliveryQuery"]["DTR"][n];

      if (!dtrnode) break;

      std::string dtrid((std::string)dtrnode["ID"]);

      Arc::XMLNode resultelement = results.NewChild("Result");
      resultelement.NewChild("ID") = dtrid;

      active_dtrs_lock.lock();
      std::map<DTR*, std::stringstream*>::iterator dtr_it = active_dtrs.begin();
      for (; dtr_it != active_dtrs.end(); ++dtr_it) {
        if (dtr_it->first->get_id() == dtrid) break;
      }

      if (dtr_it == active_dtrs.end()) {
        active_dtrs_lock.unlock();

        // if not in active list, look in archived list
        std::map<std::string, std::string>::const_iterator arc_it = archived_dtrs.find(dtrid);
        if (arc_it != archived_dtrs.end()) {
          resultelement.NewChild("ResultCode") = archived_dtrs[dtrid];
          continue;
        }

        logger.msg(Arc::ERROR, "No active DTR %s", dtrid);
        resultelement.NewChild("ResultCode") = "SERVICE_ERROR";
        resultelement.NewChild("ErrorDescription") = "No such active DTR";
        continue;
      }
      DTR * dtr = dtr_it->first;
      // check user matches - is this necessary?
      if (dtr->get_local_user().get_uid() != user.get_uid()) {
        logger.msg(Arc::ERROR, "Local user does not match user of DTR %s", dtrid);
        resultelement.NewChild("ResultCode") = "SERVICE_ERROR";
        resultelement.NewChild("ErrorDescription") = "Mapped user does not match DTR user";
        active_dtrs_lock.unlock();
        continue;
      }

      resultelement.NewChild("Log") = dtr_it->second->str();
      resultelement.NewChild("BytesTransferred") = Arc::tostring(dtr->get_bytes_transferred());

      if (dtr->error()) {
        logger.msg(Arc::INFO, "DTR %s failed", dtrid);
        resultelement.NewChild("ResultCode") = "TRANSFER_ERROR";
        resultelement.NewChild("ErrorDescription") = dtr->get_error_status().GetDesc();
        resultelement.NewChild("ErrorStatus") = Arc::tostring(dtr->get_error_status().GetErrorStatus());
        resultelement.NewChild("ErrorLocation") = Arc::tostring(dtr->get_error_status().GetErrorLocation());
      }
      else if (dtr->get_status() == DTRStatus::TRANSFERRED) {
        logger.msg(Arc::INFO, "DTR %s finished successfully", dtrid);
        resultelement.NewChild("ResultCode") = "TRANSFERRED";
        // pass calculated checksum back to Scheduler (eg to insert in catalog)
        if (dtr->get_destination()->CheckCheckSum()) resultelement.NewChild("CheckSum") = dtr->get_destination()->GetCheckSum();
      }
      else {
        logger.msg(Arc::INFO, "DTR %s still in progress", dtrid);
        resultelement.NewChild("ResultCode") = "TRANSFERRING";
      }
      active_dtrs_lock.unlock();
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  /*
   Accepts:
   <DataDeliveryCancel>
     <DTR>
       <ID>id</ID>
     </DTR>
     <DTR>
     ...
   </DataDeliveryCancel>

   Returns:
   <DataDeliveryCancelResponse>
     <DataDeliveryCancelResult>
       <Result>
         <ID>id</ID>
         <ReturnCode>ERROR</ReturnCode>
         <ReturnExplanation>...</ReturnExplanation>
       </Result>
       ...
     </DataDeliveryCancelResult>
   </DataDeliveryCancelResponse>
   */
  Arc::MCC_Status DataDeliveryService::Cancel(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& user) {
    return Arc::MCC_Status(Arc::STATUS_OK);
  }


  DataDeliveryService::DataDeliveryService(Arc::Config *cfg)
    : RegisteredService(cfg),
      max_processes(100),
      current_processes(0) {
    // Start archival thread
    if (!Arc::CreateThreadFunction(ArchivalThread, this)) {
      logger.msg(Arc::ERROR, "Failed to start archival thread");
      return;
    }
    // TODO get from configuration
    tmp_proxy_dir = "/tmp/arc";
    if (!Arc::DirCreate(tmp_proxy_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, true)) {
      logger.msg(Arc::ERROR, "Failed to create dir %s for temp proxies: %s", tmp_proxy_dir, Arc::StrError(errno));
      return;
    }
    // Set restrictive umask
    umask(0077);
    // Start new DataDelivery
    delivery.start();
    valid = true;
  }

  DataDeliveryService::~DataDeliveryService() {
    // Stop accepting new requests and cancel all active transfers
    // DataDelivery destructor automatically calls stop()
    valid = false;
  }

  Arc::MCC_Status DataDeliveryService::process(Arc::Message &inmsg, Arc::Message &outmsg) {

    // Check authorization
    if(!ProcessSecHandlers(inmsg, "incoming")) {
      logger.msg(Arc::ERROR, "Unauthorized");
      return make_soap_fault(outmsg, "Authorization failed");
    }

    std::string method = inmsg.Attributes()->get("HTTP:METHOD");

    // find local user
    std::string mapped_username = inmsg.Attributes()->get("SEC:LOCALID");
    if (mapped_username.empty()) {
      logger.msg(Arc::ERROR, "No local user mapping found");
      return make_soap_fault(outmsg, "No local user mapping found");
    }
    Arc::User mapped_user(mapped_username);
    // User must be the same as user running job. This will work if normal mapfiles
    // are used but may cause problems when using user pools.

    if(method == "POST") {
      logger.msg(Arc::VERBOSE, "process: POST");
      logger.msg(Arc::INFO, "Identity is %s", inmsg.Attributes()->get("TLS:PEERDN"));
      // Both input and output are supposed to be SOAP
      // Extracting payload
      Arc::PayloadSOAP* inpayload = NULL;
      try {
        inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
      } catch(std::exception& e) { };
      if(!inpayload) {
        logger.msg(Arc::ERROR, "input is not SOAP");
        return make_soap_fault(outmsg);
      }
      // Applying known namespaces
      inpayload->Namespaces(ns);
      if(logger.getThreshold() <= Arc::DEBUG) {
          std::string str;
          inpayload->GetDoc(str, true);
          logger.msg(Arc::DEBUG, "process: request=%s",str);
      }
      // Analyzing request
      Arc::XMLNode op = inpayload->Child(0);
      if(!op) {
        logger.msg(Arc::ERROR, "input does not define operation");
        return make_soap_fault(outmsg);
      }
      logger.msg(Arc::VERBOSE, "process: operation: %s",op.Name());

      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns);
      outpayload->Namespaces(ns);

      Arc::MCC_Status result(Arc::STATUS_OK);
      // choose operation
      // Make a new request
      if (MatchXMLName(op,"DataDeliveryStart")) {
        result = Start(*inpayload, *outpayload, mapped_user);
      }
      // Query a request
      else if (MatchXMLName(op,"DataDeliveryQuery")) {
        result = Query(*inpayload, *outpayload, mapped_user);
      }
      // Cancel a request
      else if (MatchXMLName(op,"DataDeliveryCancel")) {
        result = Cancel(*inpayload, *outpayload, mapped_user);
      }
      // Delegate credentials. Should be called before making a new request
      else if (delegation.MatchNamespace(*inpayload)) {
        if (!delegation.Process(*inpayload, *outpayload)) {
          delete outpayload;
          return make_soap_fault(outmsg);
        }
      }
      // Unknown operation
      else {
        logger.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name());
        delete outpayload;
        return make_soap_fault(outmsg);
      }

      if (!result)
        return make_soap_fault(outmsg, result.getExplanation());

      if (logger.getThreshold() <= Arc::DEBUG) {
        std::string str;
        outpayload->GetDoc(str, true);
        logger.msg(Arc::DEBUG, "process: response=%s", str);
      }
      outmsg.Payload(outpayload);

      if (!ProcessSecHandlers(outmsg,"outgoing")) {
        logger.msg(Arc::ERROR, "Security Handlers processing failed");
        delete outmsg.Payload(NULL);
        return Arc::MCC_Status();
      }
    }
    else {
      // only POST supported
      logger.msg(Arc::ERROR, "Only POST is supported in DataDeliveryService");
      return Arc::MCC_Status();
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  bool DataDeliveryService::RegistrationCollector(Arc::XMLNode &doc) {
    Arc::NS isis_ns; isis_ns["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";
    Arc::XMLNode regentry(isis_ns, "RegEntry");
    regentry.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.execution.datadeliveryservice";
    regentry.New(doc);
    return true;
  }

  Arc::MCC_Status DataDeliveryService::make_soap_fault(Arc::Message& outmsg, const std::string& reason) {
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns,true);
    Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
    if(fault) {
      fault->Code(Arc::SOAPFault::Sender);
      if (reason.empty())
        fault->Reason("Failed processing request");
      else
        fault->Reason("Failed processing request: "+reason);
    }
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

} // namespace DataStaging

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "datadeliveryservice", "HED:SERVICE", 0, &DataStaging::get_service },
    { NULL, NULL, 0, NULL }
};

