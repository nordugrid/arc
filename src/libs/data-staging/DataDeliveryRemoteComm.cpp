#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/delegation/DelegationInterface.h>

#include "DataDeliveryRemoteComm.h"

namespace DataStaging {

  Arc::Logger DataDeliveryRemoteComm::logger(Arc::Logger::getRootLogger(), "DataStaging.DataDeliveryRemoteComm");

  DataDeliveryRemoteComm::DataDeliveryRemoteComm(DTR_ptr dtr, const TransferParameters& params)
    : DataDeliveryComm(dtr, params),
      client(NULL),
      dtr_full_id(dtr->get_id()),
      query_retries(20),
      endpoint(dtr->get_delivery_endpoint()),
      timeout(dtr->get_usercfg().Timeout()),
      valid(false) {

    {
      Glib::Mutex::Lock lock(lock_);
      // Initial empty status
      memset(&status_,0,sizeof(status_));
      FillStatus();
    }

    if(!dtr->get_source()) return;
    if(!dtr->get_destination()) return;

    // check for alternative source or destination eg cache, mapped URL, TURL
    std::string surl;
    if (!dtr->get_mapped_source().empty()) {
      surl = dtr->get_mapped_source();
    }
    else if (!dtr->get_source()->TransferLocations().empty()) {
      surl = dtr->get_source()->TransferLocations()[0].fullstr();
    }
    else {
      logger_->msg(Arc::ERROR, "No locations defined for %s", dtr->get_source()->str());
      return;
    }

    if (dtr->get_destination()->TransferLocations().empty()) {
      logger_->msg(Arc::ERROR, "No locations defined for %s", dtr->get_destination()->str());
      return;
    }
    std::string durl = dtr->get_destination()->TransferLocations()[0].fullstr();
    bool caching = false;
    if ((dtr->get_cache_state() == CACHEABLE) && !dtr->get_cache_file().empty()) {
      durl = dtr->get_cache_file();
      caching = true;
    }

    if (dtr->host_cert_for_remote_delivery()) {
      Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::TryCredentials);
      Arc::UserConfig host_cfg(cred_type);
      host_cfg.ProxyPath(""); // to force using cert/key files instead of non-existent proxy
      host_cfg.ApplyToConfig(cfg);
    } else {
      dtr->get_usercfg().ApplyToConfig(cfg);
    }

    // connect to service and make a new transfer request
    logger_->msg(Arc::VERBOSE, "Connecting to Delivery service at %s", endpoint.str());
    // TODO: implement pool of ClientSOAP objects instead of having one for each Comm
    // object. That shall reduce number of TCP connections. 
    client = new Arc::ClientSOAP(cfg, endpoint, timeout);

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);

    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryStart").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;
    dtrnode.NewChild("Source") = surl;
    dtrnode.NewChild("Destination") = durl;
    if (dtr->get_source()->CheckSize()) dtrnode.NewChild("Size") = Arc::tostring(dtr->get_source()->GetSize());
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
      logger_->msg(Arc::ERROR, "Failed to set up credential delegation with %s", endpoint.str());
      return;
    }

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "Could not connect to service %s: %s",
                   endpoint.str(), (std::string)status);
      if (response)
        delete response;
      return;
    }

    if (!response) {
      logger_->msg(Arc::ERROR, "No SOAP response from Delivery service %s", endpoint.str());
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Response:\n%s", xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      logger_->msg(Arc::ERROR, "Failed to start transfer request: %s", err);
      delete response;
      return;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryStartResponse"]["DataDeliveryStartResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "Bad format in XML response from service at %s: %s",
                   endpoint.str(), xml);
      delete response;
      return;
    }

    std::string resultcode = (std::string)(resultnode["ResultCode"]);
    if (resultcode != "OK") {
      logger_->msg(Arc::ERROR, "Could not make new transfer request: %s: %s",
                   resultcode, (std::string)(resultnode[0]["ErrorDescription"]));
      delete response;
      return;
    }
    logger_->msg(Arc::INFO, "Started remote Delivery at %s", endpoint.str());

    delete response;
    valid = true;
    GetHandler().Add(this);
  }

  DataDeliveryRemoteComm::~DataDeliveryRemoteComm() {
    // If transfer is still going, send cancellation request to service
    if (valid) CancelDTR();
    GetHandler().Remove(this);
    Glib::Mutex::Lock lock(lock_);
    delete client;
  }

  std::string DataDeliveryRemoteComm::DeliveryId() const {
    return endpoint.str();
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
    logger_->msg(Arc::DEBUG, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "Failed to send cancel request: %s", (std::string)status);
      if (response)
        delete response;
      return;
    }

    if (!response) {
      logger_->msg(Arc::ERROR, "Failed to cancel: No SOAP response");
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Response:\n%s", xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      logger_->msg(Arc::ERROR, "Failed to cancel transfer request: %s", err);
      delete response;
      return;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryCancelResponse"]["DataDeliveryCancelResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "Bad format in XML response: %s", xml);
      delete response;
      return;
    }

    if ((std::string)resultnode["ResultCode"] != "OK") {
      Arc::XMLNode errnode = resultnode["ErrorDescription"];
      logger_->msg(Arc::ERROR, "Failed to cancel: %s", (std::string)errnode);
    }
    delete response;
  }

  void DataDeliveryRemoteComm::PullStatus() {
    // send query request to service and fill status_
    Glib::Mutex::Lock lock(lock_);
    if (!client) return;

    // check time since last query - check every second for the first 20s and
    // after every 5s
    // TODO be more intelligent, using transfer rate and file size
    if (Arc::Time() - start_ < 20 && Arc::Time() - Arc::Time(status_.timestamp) < 1) return;
    if (Arc::Time() - start_ > 20 && Arc::Time() - Arc::Time(status_.timestamp) < 5) return;

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);
    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryQuery").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "%s", (std::string)status);
      status_.commstatus = CommFailed;
      if (response)
        delete response;
      valid = false;
      return;
    }

    if (!response) {
      if (--query_retries > 0) {
        HandleQueryFault("No SOAP response from delivery service");
        return;
      }
      logger_->msg(Arc::ERROR, "No SOAP response from delivery service");
      status_.commstatus = CommFailed;
      valid = false;
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Response:\n%s", xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      delete response;
      if (--query_retries > 0) {
        HandleQueryFault("Failed to query state: " + err);
        return;
      }
      logger_->msg(Arc::ERROR, "Failed to query state: %s", err);
      status_.commstatus = CommFailed;
      strncpy(status_.error_desc, "SOAP error in connection with delivery service", sizeof(status_.error_desc));
      valid = false;
      return;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryQueryResponse"]["DataDeliveryQueryResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "Bad format in XML response: %s", xml);
      delete response;
      status_.commstatus = CommFailed;
      valid = false;
      return;
    }

    // Fill status fields with results from service
    FillStatus(resultnode[0]);

    delete response;
  }

  bool DataDeliveryRemoteComm::CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg) {
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

    dtr->get_logger()->msg(Arc::VERBOSE, "Connecting to Delivery service at %s",
                           dtr->get_delivery_endpoint().str());
    Arc::ClientSOAP client(cfg, dtr->get_delivery_endpoint(), dtr->get_usercfg().Timeout());

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);

    Arc::XMLNode ping = request.NewChild("DataDeliveryPing");

    std::string xml;
    request.GetXML(xml, true);
    dtr->get_logger()->msg(Arc::DEBUG, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client.process(&request, &response);

    if (!status) {
      dtr->get_logger()->msg(Arc::ERROR, "Could not connect to service %s: %s",
                             dtr->get_delivery_endpoint().str(), (std::string)status);
      if (response)
        delete response;
      return false;
    }

    if (!response) {
      dtr->get_logger()->msg(Arc::ERROR, "No SOAP response from Delivery service %s",
                             dtr->get_delivery_endpoint().str());
      return false;
    }

    response->GetXML(xml, true);
    dtr->get_logger()->msg(Arc::DEBUG, "Response:\n%s", xml);

    if (response->IsFault()) {
      Arc::SOAPFault& fault = *response->Fault();
      std::string err("SOAP fault: %s", fault.Code());
      for (int n = 0;;++n) {
        if (fault.Reason(n).empty()) break;
        err += ": " + fault.Reason(n);
      }
      dtr->get_logger()->msg(Arc::ERROR, "SOAP fault from delivery service at %s: %s",
                             dtr->get_delivery_endpoint().str(), err);
      delete response;
      return false;
    }

    Arc::XMLNode resultnode = (*response)["DataDeliveryPingResponse"]["DataDeliveryPingResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      dtr->get_logger()->msg(Arc::ERROR, "Bad format in XML response from delivery service at %s: %s",
                             dtr->get_delivery_endpoint().str(), xml);
      delete response;
      return false;
    }

    std::string resultcode = (std::string)(resultnode["ResultCode"]);
    if (resultcode != "OK") {
      dtr->get_logger()->msg(Arc::ERROR, "Error pinging delivery service at %s: %s: %s",
                             dtr->get_delivery_endpoint().str(),
                             resultcode, (std::string)(resultnode[0]["ErrorDescription"]));
      delete response;
      return false;
    }
    for (Arc::XMLNode dir = resultnode["AllowedDir"]; dir; ++dir) {
      allowed_dirs.push_back((std::string)dir);
      dtr->get_logger()->msg(Arc::DEBUG, "Dir %s allowed at service %s",
                             (std::string)dir, dtr->get_delivery_endpoint().str());
    }

    if (resultnode["LoadAvg"]) {
      load_avg = (std::string)(resultnode["LoadAvg"]);
    } else {
      load_avg = "-1";
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
      status_.transfer_time = 0;
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
    datanode = node["TransferTime"];
    if (datanode) {
      unsigned long long int t;
      Arc::stringto(std::string(datanode), t);
      status_.transfer_time = t;
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
        logger_->msg(Arc::INFO, "DataDelivery log tail:\n%s", log);
      }
      valid = false;
    }
  }


  bool DataDeliveryRemoteComm::SetupDelegation(Arc::XMLNode& op, const Arc::UserConfig& usercfg) {
    const std::string& cert = (!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.CertificatePath());
    const std::string& key  = (!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.KeyPath());
    const std::string& credentials = usercfg.CredentialString();

    if (credentials.empty() && (key.empty() || cert.empty())) {
      logger_->msg(Arc::VERBOSE, "Failed locating credentials");
      return false;
    }

    if(!client->Load()) {
      logger_->msg(Arc::VERBOSE, "Failed to initiate client connection");
      return false;
    }

    Arc::MCC* entry = client->GetEntry();
    if(!entry) {
      logger_->msg(Arc::VERBOSE, "Client connection has no entry point");
      return false;
    }

    Arc::DelegationProviderSOAP * deleg = NULL;
    // Use in-memory credentials if set in UserConfig
    if (!credentials.empty()) deleg = new Arc::DelegationProviderSOAP(credentials);
    else deleg = new Arc::DelegationProviderSOAP(cert, key);
    logger_->msg(Arc::VERBOSE, "Initiating delegation procedure");
    if (!deleg->DelegateCredentialsInit(*entry, &(client->GetContext()))) {
      logger_->msg(Arc::VERBOSE, "Failed to initiate delegation credentials");
      delete deleg;
      return false;
    }
    deleg->DelegatedToken(op);
    delete deleg;
    return true;
  }

  void DataDeliveryRemoteComm::HandleQueryFault(const std::string& err) {
    // Just return without changing status
    logger_->msg(Arc::WARNING, err);
    status_.timestamp = time(NULL);
    // A reconnect may be needed after losing connection
    delete client;
    client = new Arc::ClientSOAP(cfg, endpoint, timeout);
  }

} // namespace DataStaging
