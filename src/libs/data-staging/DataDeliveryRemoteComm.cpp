#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/MCC.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/delegation/DelegationInterface.h>

#include "DataDeliveryRemoteComm.h"

namespace DataStaging {

  Arc::Logger DataDeliveryRemoteComm::logger(Arc::Logger::getRootLogger(), "DataStaging.DataDeliveryRemoteComm");

  DataDeliveryRemoteComm::DataDeliveryRemoteComm(DTR_ptr dtr, const TransferParameters& params)
    : DataDeliveryComm(dtr, params), client(NULL), dtr_full_id(dtr->get_id()), valid(false) {

    {
      Glib::Mutex::Lock lock(lock_);
      // Initial empty status
      memset(&status_,0,sizeof(status_));
      FillStatus();
    }

    if(!dtr->get_source()) return;
    if(!dtr->get_destination()) return;

    // check for alternative source or destination eg cache, mapped URL, TURL
    if (dtr->get_source()->TransferLocations().empty()) {
      logger_->msg(Arc::ERROR, "DTR %s: No locations defined for %s", dtr_id, dtr->get_source()->str());
      return;
    }
    std::string surl = dtr->get_source()->TransferLocations()[0].fullstr();
    bool caching = false;
    if (!dtr->get_mapped_source().empty())
      surl = dtr->get_mapped_source();

    if (dtr->get_destination()->TransferLocations().empty()) {
      logger_->msg(Arc::ERROR, "DTR %s: No locations defined for %s", dtr_id, dtr->get_destination()->str());
      return;
    }
    std::string durl = dtr->get_destination()->TransferLocations()[0].fullstr();
    if ((dtr->get_cache_state() == CACHEABLE) && !dtr->get_cache_file().empty()) {
      durl = dtr->get_cache_file();
      caching = true;
    }

    // connect to service and make a new transfer request
    Arc::MCCConfig cfg;
    if (dtr->host_cert_for_remote_delivery()) {
      Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::TryCredentials);
      Arc::UserConfig host_cfg(cred_type);
      host_cfg.ProxyPath(""); // to force using cert/key files instead of non-existent proxy
      host_cfg.ApplyToConfig(cfg);
    } else {
      dtr->get_usercfg().ApplyToConfig(cfg);
    }

    logger_->msg(Arc::VERBOSE, "DTR %s: Connecting to Delivery service at %s",
                 dtr_id, dtr->get_delivery_endpoint().str());
    client = new Arc::ClientSOAP(cfg, dtr->get_delivery_endpoint(), dtr->get_usercfg().Timeout());

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);

    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryStart").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;
    dtrnode.NewChild("Source") = surl;
    dtrnode.NewChild("Destination") = durl;
    if (dtr->get_source()->CheckCheckSum()) dtrnode.NewChild("CheckSum") = dtr->get_source()->GetCheckSum();
    dtrnode.NewChild("Uid") = Arc::tostring(dtr->get_local_user().get_uid());
    dtrnode.NewChild("Gid") = Arc::tostring(dtr->get_local_user().get_gid());
    // transfer parameters
    dtrnode.NewChild("MinAverageSpeed") = Arc::tostring(params.min_average_bandwidth);
    dtrnode.NewChild("AverageTime") = Arc::tostring(params.averaging_time);
    dtrnode.NewChild("MinCurrentSpeed") = Arc::tostring(params.min_current_bandwidth);
    dtrnode.NewChild("MaxInactivityTime") = Arc::tostring(params.max_inactivity_time);
    // caching
    if (caching) dtrnode.NewChild("Caching") = "true";
    else dtrnode.NewChild("Caching") = "false";

    // delegate credentials
    Arc::XMLNode op = request.Child(0);
    if (!SetupDelegation(op, dtr->get_usercfg())) {
      logger_->msg(Arc::ERROR, "DTR %s: Failed to set up credential delegation with %s",
                   dtr_id, dtr->get_delivery_endpoint().str());
      return;
    }

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Request:\n%s", dtr_id, xml);

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "DTR %s: Could not connect to service %s: %s",
                   dtr_id, dtr->get_delivery_endpoint().str(), (std::string)status);
      if (response)
        delete response;
      return;
    }

    if (!response) {
      logger_->msg(Arc::ERROR, "DTR %s: No SOAP response from Delivery service %s",
                   dtr_id, dtr->get_delivery_endpoint().str());
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Response:\n%s", dtr_id, xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      logger_->msg(Arc::ERROR, "DTR %s: Failed to start transfer request: %s", dtr_id, err);
      delete response;
      return;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryStartResponse"]["DataDeliveryStartResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "DTR %s: Bad format in XML response from service at %s: %s",
                   dtr_id, dtr->get_delivery_endpoint().str(), xml);
      delete response;
      return;
    }

    std::string resultcode = (std::string)(resultnode["ResultCode"]);
    if (resultcode != "OK") {
      logger_->msg(Arc::ERROR, "DTR %s: Could not make new transfer request: %s: %s",
                   dtr_id, resultcode, (std::string)(resultnode[0]["ErrorDescription"]));
      delete response;
      return;
    }
    logger_->msg(Arc::INFO, "DTR %s: Started remote Delivery at %s",
                 dtr_id, dtr->get_delivery_endpoint().str());

    delete response;
    valid = true;
    handler_->Add(this);
  }

  DataDeliveryRemoteComm::~DataDeliveryRemoteComm() {
    // If transfer is still going, send cancellation request to service
    if (valid) CancelDTR();
    if (handler_) handler_->Remove(this);
    Glib::Mutex::Lock lock(lock_);
    delete client;
  }

  void DataDeliveryRemoteComm::CancelDTR() {
    Glib::Mutex::Lock lock(lock_);
    if (!client) return;
    Arc::NS ns;
    Arc::PayloadSOAP request(ns);
    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryCancel").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Request:\n%s", dtr_id, xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "DTR %s: Failed to send cancel request: %s", dtr_id, (std::string)status);
      if (response)
        delete response;
      return;
    }

    if (!response) {
      logger_->msg(Arc::ERROR, "DTR %s: Failed to cancel: No SOAP response", dtr_id);
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Response:\n%s", dtr_id, xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      logger_->msg(Arc::ERROR, "DTR %s: Failed to cancel transfer request: %s", dtr_id, err);
      delete response;
      return;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryCancelResponse"]["DataDeliveryCancelResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "DTR %s: Bad format in XML response: %s", dtr_id, xml);
      delete response;
      return;
    }

    if ((std::string)resultnode["ResultCode"] != "OK") {
      Arc::XMLNode errnode = resultnode["ErrorDescription"];
      logger_->msg(Arc::ERROR, "DTR %s: Failed to cancel: %s", dtr_id, (std::string)errnode);
    }
    delete response;
  }

  void DataDeliveryRemoteComm::PullStatus() {
    // send query request to service and fill status_
    Glib::Mutex::Lock lock(lock_);
    if (!client) return;

    // check time since last query - for long transfers we do not need to query
    // at a high frequency. After 5s query every 5s.
    // TODO be more intelligent, using transfer rate and file size
    if (Arc::Time() - start_ > 5 && Arc::Time() - Arc::Time(status_.timestamp) < 5) return;

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);
    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryQuery").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Request:\n%s", dtr_id, xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "DTR %s: %s", dtr_id, (std::string)status);
      status_.commstatus = CommFailed;
      if (response)
        delete response;
      valid = false;
      return;
    }

    if (!response) {
      logger_->msg(Arc::ERROR, "DTR %s: No SOAP response", dtr_id);
      status_.commstatus = CommFailed;
      valid = false;
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Response:\n%s", dtr_id, xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      logger_->msg(Arc::ERROR, "DTR %s: Failed to query state: %s", dtr_id, err);
      delete response;
      status_.commstatus = CommFailed;
      strncpy(status_.error_desc, "SOAP error in connection with delivery service", sizeof(status_.error_desc));
      valid = false;
      return;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryQueryResponse"]["DataDeliveryQueryResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "DTR %s: Bad format in XML response: %s", dtr_id, xml);
      delete response;
      status_.commstatus = CommFailed;
      valid = false;
      return;
    }

    // Fill status fields with results from service
    FillStatus(resultnode[0]);

    delete response;
  }

  bool DataDeliveryRemoteComm::CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs) {
    // call Ping
    Arc::MCCConfig cfg;
    if (dtr->host_cert_for_remote_delivery()) {
      Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::TryCredentials);
      Arc::UserConfig host_cfg(cred_type);
      host_cfg.ProxyPath(""); // to force using cert/key files instead of non-existent proxy
      host_cfg.ApplyToConfig(cfg);
    } else {
      dtr->get_usercfg().ApplyToConfig(cfg);
    }

    dtr->get_logger()->msg(Arc::VERBOSE, "DTR %s: Connecting to Delivery service at %s",
                           dtr->get_short_id(), dtr->get_delivery_endpoint().str());
    Arc::ClientSOAP client(cfg, dtr->get_delivery_endpoint(), dtr->get_usercfg().Timeout());

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);

    Arc::XMLNode ping = request.NewChild("DataDeliveryPing");

    std::string xml;
    request.GetXML(xml, true);
    dtr->get_logger()->msg(Arc::DEBUG, "DTR %s: Request:\n%s", dtr->get_short_id(), xml);

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client.process(&request, &response);

    if (!status) {
      dtr->get_logger()->msg(Arc::ERROR, "DTR %s: Could not connect to service %s: %s",
                             dtr->get_short_id(), dtr->get_delivery_endpoint().str(), (std::string)status);
      if (response)
        delete response;
      return false;
    }

    if (!response) {
      dtr->get_logger()->msg(Arc::ERROR, "DTR %s: No SOAP response from Delivery service %s",
                             dtr->get_short_id(), dtr->get_delivery_endpoint().str());
      return false;
    }

    response->GetXML(xml, true);
    dtr->get_logger()->msg(Arc::DEBUG, "DTR %s: Response:\n%s", dtr->get_short_id(), xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      dtr->get_logger()->msg(Arc::ERROR, "DTR %s: SOAP fault from delivery service at %s: %s",
                             dtr->get_short_id(), dtr->get_delivery_endpoint().str(), err);
      delete response;
      return false;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryPingResponse"]["DataDeliveryPingResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      dtr->get_logger()->msg(Arc::ERROR, "DTR %s: Bad format in XML response from delivery service at %s: %s",
                             dtr->get_short_id(), dtr->get_delivery_endpoint().str(), xml);
      delete response;
      return false;
    }

    std::string resultcode = (std::string)(resultnode["ResultCode"]);
    if (resultcode != "OK") {
      dtr->get_logger()->msg(Arc::ERROR, "DTR %s: Error pinging delivery service at %s: %s: %s",
                             dtr->get_short_id(), dtr->get_delivery_endpoint().str(),
                             resultcode, (std::string)(resultnode[0]["ErrorDescription"]));
      delete response;
      return false;
    }
    for (Arc::XMLNode dir = resultnode["AllowedDir"]; dir; ++dir) {
      allowed_dirs.push_back((std::string)dir);
      dtr->get_logger()->msg(Arc::DEBUG, "Dir %s allowed at service %s",
                             (std::string)dir, dtr->get_delivery_endpoint().str());
    }

    delete response;
    return true;
  }

  void DataDeliveryRemoteComm::FillStatus(const Arc::XMLNode& node) {

    if (!node) {
      // initial state
      std::string empty("");
      status_.commstatus = DataDeliveryComm::CommInit;
      status_.timestamp = ::time(NULL);
      status_.status = DTRStatus::NULL_STATE;
      status_.error = DTRErrorStatus::NONE_ERROR;
      status_.error_location = DTRErrorStatus::NO_ERROR_LOCATION;
      strncpy(status_.error_desc, empty.c_str(), sizeof(status_.error_desc));
      status_.streams = 0;
      status_.transferred = 0;
      status_.size = 0;
      status_.offset = 0;
      status_.speed = 0;
      strncpy(status_.checksum, empty.c_str(), sizeof(status_.checksum));
      return;
    }

    Arc::XMLNode datanode = node["ResultCode"];
    if (std::string(datanode) == "TRANSFERRED") {
      status_.commstatus = CommExited;
      status_.status = DTRStatus::TRANSFERRED;
    }
    else if (std::string(datanode) == "TRANSFER_ERROR") {
      status_.commstatus = CommFailed;
      status_.status = DTRStatus::TRANSFERRED;
    }
    else if (std::string(datanode) == "SERVICE_ERROR") {
      status_.commstatus = CommFailed;
      status_.status = DTRStatus::TRANSFERRED;
    }
    else {
      status_.commstatus = CommNoError;
      status_.status = DTRStatus::TRANSFERRING;
    }

    status_.timestamp = time(NULL);

    datanode = node["ErrorStatus"];
    if (datanode) {
      int error_status;
      Arc::stringto(std::string(datanode), error_status);
      status_.error = (DTRErrorStatus::DTRErrorStatusType)error_status;
    }
    datanode = node["ErrorLocation"];
    if (datanode) {
      int error_location;
      Arc::stringto(std::string(datanode), error_location);
      status_.error_location = (DTRErrorStatus::DTRErrorLocation)error_location;
    }
    datanode = node["ErrorDescription"];
    if (datanode) {
      strncpy(status_.error_desc, ((std::string)datanode).c_str(), sizeof(status_.error_desc));
    }
    datanode = node["BytesTransferred"];
    if (datanode) {
      unsigned long long int bytes;
      Arc::stringto(std::string(datanode), bytes);
      status_.transferred = bytes;
    }
    // TODO size, offset, speed (currently not used)
    datanode = node["CheckSum"];
    if (datanode) {
      strncpy(status_.checksum, ((std::string)datanode).c_str(), sizeof(status_.checksum));
    }
    // if terminal state, write log
    if (status_.commstatus != CommNoError) {
      // log message is limited to 2048 chars so just print last few lines
      std::string log = (std::string)node["Log"];
      if (!log.empty()) {
        if (log.size() > 2000) log = log.substr(log.find('\n', log.size()-2000));
        logger_->msg(Arc::INFO, "DTR %s: DataDelivery log tail:\n%s", dtr_id, log);
      }
      valid = false;
    }
  }


  bool DataDeliveryRemoteComm::SetupDelegation(Arc::XMLNode& op, const Arc::UserConfig& usercfg) {
    const std::string& cert = (!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.CertificatePath());
    const std::string& key  = (!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.KeyPath());

    if (key.empty() || cert.empty()) {
      logger_->msg(Arc::VERBOSE, "DTR %s: Failed locating credentials", dtr_id);
      return false;
    }

    if(!client->Load()) {
      logger_->msg(Arc::VERBOSE, "DTR %s: Failed to initiate client connection", dtr_id);
      return false;
    }

    Arc::MCC* entry = client->GetEntry();
    if(!entry) {
      logger_->msg(Arc::VERBOSE, "DTR %s: Client connection has no entry point", dtr_id);
      return false;
    }

    Arc::DelegationProviderSOAP deleg(cert, key);
    logger_->msg(Arc::VERBOSE, "DTR %s: Initiating delegation procedure", dtr_id);
    if (!deleg.DelegateCredentialsInit(*entry, &(client->GetContext()))) {
      logger_->msg(Arc::VERBOSE, "DTR %s: Failed to initiate delegation credentials", dtr_id);
      return false;
    }
    deleg.DelegatedToken(op);
    return true;
  }

} // namespace DataStaging
