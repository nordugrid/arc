#include <arc/message/MCC.h>
#include <arc/delegation/DelegationInterface.h>

#include "DataDeliveryRemoteComm.h"

namespace DataStaging {

  Arc::Logger DataDeliveryRemoteComm::logger(Arc::Logger::getRootLogger(), "DataStaging.DataDeliveryRemoteComm");

  DataDeliveryRemoteComm::DataDeliveryRemoteComm(const DTR& dtr, const TransferParameters& params)
    : DataDeliveryComm(dtr, params), client(NULL), dtr_full_id(dtr.get_id()), valid(false) {

    {
      Glib::Mutex::Lock lock(lock_);
      // Initial empty status
      memset(&status_,0,sizeof(status_));
      FillStatus();
    }

    if(!dtr.get_source()) return;
    if(!dtr.get_destination()) return;

    // check for alternative source or destination eg cache, mapped URL, TURL
    if (dtr.get_source()->TransferLocations().empty()) {
      logger_->msg(Arc::ERROR, "DTR %s: No locations defined for %s", dtr_id, dtr.get_source()->str());
      return;
    }
    std::string surl = dtr.get_source()->TransferLocations()[0].fullstr();
    bool caching = false;
    if (!dtr.get_mapped_source().empty())
      surl = dtr.get_mapped_source();

    if (dtr.get_destination()->TransferLocations().empty()) {
      logger_->msg(Arc::ERROR, "DTR %s: No locations defined for %s", dtr_id, dtr.get_destination()->str());
      return;
    }
    std::string durl = dtr.get_destination()->TransferLocations()[0].fullstr();
    if ((dtr.get_cache_state() == CACHEABLE) && !dtr.get_cache_file().empty()) {
      durl = dtr.get_cache_file();
      caching = true;
    }

    // connect to service and make a new transfer request
    Arc::MCCConfig cfg;
    dtr.get_usercfg().ApplyToConfig(cfg);

    logger_->msg(Arc::VERBOSE, "DTR %s: Connecting to Delivery service at %s",
                 dtr_id, dtr.get_delivery_endpoint().str());
    client = new Arc::ClientSOAP(cfg, dtr.get_delivery_endpoint(), dtr.get_usercfg().Timeout());

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);

    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryStart").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr.get_id();
    dtrnode.NewChild("Source") = surl;
    dtrnode.NewChild("Destination") = durl;
    if (dtr.get_source()->CheckCheckSum()) dtrnode.NewChild("CheckSum") = dtr.get_source()->GetCheckSum();
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
    if (!SetupDelegation(op, dtr.get_usercfg())) {
      logger_->msg(Arc::ERROR, "Failed to set up credential delegation");
      return;
    }

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Request:\n%s", dtr_id, xml);

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      logger_->msg(Arc::ERROR, "DTR %s: Could not connect to service %s: %s",
                   dtr_id, dtr.get_delivery_endpoint().str(), (std::string)status);
      if (response)
        delete response;
      return;
    }

    if (!response) {
      logger_->msg(Arc::ERROR, "DTR %s: No SOAP response from Delivery service %s",
                   dtr_id, dtr.get_delivery_endpoint().str());
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "DTR %s: Response:\n%s", dtr_id, xml);

    Arc::XMLNode resultnode = (*response)["DataDeliveryStartResponse"]["DataDeliveryStartResult"]["Result"][0];
    if (!resultnode || !resultnode["ResultCode"]) {
      logger_->msg(Arc::ERROR, "DTR %s: Bad format in XML response from service at %s: %s",
                   dtr_id, dtr.get_delivery_endpoint().str(), xml);
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
                 dtr_id, dtr.get_delivery_endpoint().str());

    delete response;
    valid = true;
    handler_->Add(this);
  }

  DataDeliveryRemoteComm::~DataDeliveryRemoteComm() {
    // if transfer is still going, send cancellation request to service
    if(handler_) handler_->Remove(this);
    delete client;
  }

  void DataDeliveryRemoteComm::PullStatus() {
    // send query request to service and fill status_
    Glib::Mutex::Lock lock(lock_);
    if (!client) return;

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);
    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryQuery").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;

    std::string xml;
    request.GetXML(xml, true);
    if (logger_) logger_->msg(Arc::DEBUG, "DTR %s: Request:\n%s", dtr_id, xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client->process(&request, &response);

    if (!status) {
      if (logger_) logger_->msg(Arc::ERROR, "DTR %s: %s", dtr_id, (std::string)status);
      status_.commstatus = CommFailed;
      if (response)
        delete response;
      valid = false;
      return;
    }

    if (!response) {
      if (logger_) logger_->msg(Arc::ERROR, "DTR %s: No SOAP response", dtr_id);
      status_.commstatus = CommFailed;
      valid = false;
      return;
    }

    response->GetXML(xml, true);
    if (logger_) logger_->msg(Arc::DEBUG, "DTR %s: Response:\n%s", dtr_id, xml);

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
      if (log.size() > 2000) log = log.substr(log.find('\n', log.size()-2000));
      logger_->msg(Arc::INFO, "DTR %s: DataDelivery log tail:\n%s", dtr_id, log);
    }
  }


  bool DataDeliveryRemoteComm::SetupDelegation(Arc::XMLNode& op, const Arc::UserConfig& usercfg) {
    const std::string& cert = (!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.CertificatePath());
    const std::string& key  = (!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.KeyPath());

    if (key.empty() || cert.empty()) {
      logger_->msg(Arc::VERBOSE, "Failed locating credentials.");
      return false;
    }

    if(!client->Load()) {
      logger_->msg(Arc::VERBOSE, "Failed initiate client connection.");
      return false;
    }

    Arc::MCC* entry = client->GetEntry();
    if(!entry) {
      logger_->msg(Arc::VERBOSE, "Client connection has no entry point.");
      return false;
    }

    Arc::DelegationProviderSOAP deleg(cert, key);
    logger_->msg(Arc::VERBOSE, "Initiating delegation procedure");
    if (!deleg.DelegateCredentialsInit(*entry, &(client->GetContext()))) {
      logger_->msg(Arc::VERBOSE, "Failed to initiate delegation credentials");
      return false;
    }
    deleg.DelegatedToken(op);
    return true;
  }
} // namespace DataStaging
