#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/delegation/DelegationInterface.h>

#include "DataDeliveryRemoteComm.h"

#define DELEGATION_NAMESPACE "http://www.nordugrid.org/schemas/delegation"

namespace DataStaging {

  Arc::Logger DataDeliveryRemoteComm::logger(Arc::Logger::getRootLogger(), "DataStaging.DataDeliveryRemoteComm");

  /// Readable name of a SOAP fault code
  static std::string soap_fault_code(Arc::SOAPFault::SOAPFaultCode code) {
    switch (code) {
      case Arc::SOAPFault::undefined:           return "undefined";
      case Arc::SOAPFault::unknown:             return "unknown";
      case Arc::SOAPFault::VersionMismatch:     return "VersionMismatch";
      case Arc::SOAPFault::MustUnderstand:      return "MustUnderstand";
      case Arc::SOAPFault::Sender:              return "Sender";
      case Arc::SOAPFault::Receiver:            return "Receiver";
      case Arc::SOAPFault::DataEncodingUnknown: return "DataEncodingUnknown";
    }
    return Arc::tostring((int)code);
  }

  DataDeliveryRemoteComm::QueryChannel::QueryChannel(const Arc::MCCConfig& cfg,
                                                     const Arc::URL& endpoint,
                                                     int timeout)
    : client(NULL), cfg(cfg), endpoint(endpoint), timeout(timeout) {
    client = new Arc::ClientSOAP(this->cfg, this->endpoint, this->timeout);
  }

  DataDeliveryRemoteComm::QueryChannel::~QueryChannel() {
    delete client;
  }

  void DataDeliveryRemoteComm::QueryChannel::ReconnectLocked() {
    delete client;
    client = new Arc::ClientSOAP(cfg, endpoint, timeout);
  }

  std::mutex DataDeliveryRemoteComm::EndpointPoller::pollers_lock;
  std::map<std::string, DataDeliveryRemoteComm::EndpointPoller*> DataDeliveryRemoteComm::EndpointPoller::pollers;
  unsigned int DataDeliveryRemoteComm::EndpointPoller::poll_interval_ms = 1000;

  const unsigned int DataDeliveryRemoteComm::EndpointPoller::max_query_faults;

  DataDeliveryRemoteComm::EndpointPoller::EndpointPoller(const std::string& endpoint)
    : channel_(NULL), endpoint_str_(endpoint), faults_(0) {
    // Started last: the thread reads the members above, so they must all be in
    // place before it can run.
    Arc::CreateThreadFunction(&func, this);
  }

  DataDeliveryRemoteComm::EndpointPoller*
  DataDeliveryRemoteComm::EndpointPoller::Get(const Arc::URL& endpoint) {
    std::unique_lock<std::mutex> lock(pollers_lock);
    std::string key(endpoint.str());
    auto i = pollers.find(key);
    if (i != pollers.end()) {
      return i->second;
    }
    EndpointPoller* poller = new EndpointPoller(key);
    pollers[key] = poller;
    return poller;
  }

  void DataDeliveryRemoteComm::EndpointPoller::Add(DataDeliveryRemoteComm* comm) {
    std::unique_lock<std::mutex> lock(state_lock_);
    // The connection exists only while there is something to ask about, so it
    // is never left idle for the service to close.
    if (!channel_) {
      channel_ = new QueryChannel(comm->cfg, comm->endpoint, comm->timeout);
    }
    items_.push_back(comm);
  }

  void DataDeliveryRemoteComm::EndpointPoller::Remove(DataDeliveryRemoteComm* comm) {
    std::unique_lock<std::mutex> lock(state_lock_);
    items_.remove(comm);
    // A sweep may be part way through polling this transfer, holding a pointer
    // it took before releasing the lock. Wait for it to be done.
    while (in_flight_.find(comm) != in_flight_.end()) {
      poll_done_.wait(lock);
    }
    if (items_.empty()) {
      delete channel_;
      channel_ = NULL;
    }
  }

  DataDeliveryRemoteComm::ExchangeResult
  DataDeliveryRemoteComm::EndpointPoller::Exchange(Arc::PayloadSOAP& request,
                                                   Arc::PayloadSOAP*& response,
                                                   std::string& fault) {
    response = NULL;
    fault.clear();

    // Safe to use outside the lock because every caller is still registered
    // here: a sweep's members are in items_ and held in in_flight_, and a
    // transfer cancelling itself has not reached Remove() yet. The list is
    // therefore not empty, and only an empty list closes the connection.
    QueryChannel* channel = NULL;
    {
      std::unique_lock<std::mutex> lock(state_lock_);
      channel = channel_;
    }
    if (!channel) {
      fault = "No connection to delivery service";
      return ExchangeFault;
    }

    // One request at a time: a sweep and a cancellation can reach this from
    // different threads.
    std::unique_lock<std::mutex> lock(channel->lock);

    Arc::MCC_Status status = channel->client->process(&request, &response);

    if (!status) {
      fault = (std::string)status;
    } else if (!response) {
      fault = "No SOAP response from delivery service";
    } else if (response->IsFault()) {
      Arc::SOAPFault& soapfault = *response->Fault();
      fault = "SOAP fault: " + soap_fault_code(soapfault.Code());
      for (int n = 0;; ++n) {
        if (soapfault.Reason(n).empty()) {
          break;
        }
        fault += ": " + soapfault.Reason(n);
      }
    }

    // Much the most likely cause is the service having closed a connection it
    // considered idle. Replace it here, while the lock is still held, so that
    // the next user does not have to discover the same thing.
    if (!fault.empty()) {
      channel->ReconnectLocked();
    }

    return fault.empty() ? ExchangeOK : ExchangeFault;
  }

  void DataDeliveryRemoteComm::EndpointPoller::func(void* arg) {
    // Each transfer logs through its own Logger, which is a child of the root
    // logger and so also reaches the service log. Detach this thread from the
    // root logger's destinations so data staging messages stay in their own log.
    Arc::Logger::getRootLogger().setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();

    EndpointPoller& it = *(EndpointPoller*)arg;
    for(;;) {
      it.Sweep();
      usleep(poll_interval_ms * 1000);
    }
  }

  void DataDeliveryRemoteComm::EndpointPoller::Sweep() {

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);
    Arc::XMLNode query = request.NewChild("DataDeliveryQuery");

    std::map<std::string, DataDeliveryRemoteComm*> by_id;
    std::list<DataDeliveryRemoteComm*> polled;
    DataDeliveryRemoteComm* first = NULL;

    // One walk of the list, under the lock: decide what is worth asking about,
    // build the request for it, and mark it so that Remove() waits. A transfer
    // which has finished, or which is on its way out, is passed over rather
    // than marked, so its destructor is not held up by a query it has no part
    // in. The service has always accepted several DTR elements per query.
    {
      std::unique_lock<std::mutex> lock(state_lock_);
      for (auto comm : items_) {
        std::unique_lock<std::mutex> comm_lock(comm->lock_);
        // Still running? A transfer which has reached a terminal state stays
        // registered until its owner notices and destroys it, and there is
        // nothing left to ask the service about it.
        if (!comm->valid) {
          continue;
        }
        if (!first) {
          first = comm;
        }
        query.NewChild("DTR").NewChild("ID") = comm->dtr_full_id;
        by_id[comm->dtr_full_id] = comm;
        polled.push_back(comm);
        in_flight_.insert(comm);
      }
    }
    if (polled.empty()) {
      return;
    }

    // Nothing is sent under the lock: a query is a network call which can block
    // for the client timeout, and holding the lock across it would stop
    // transfers registering and unregistering at this endpoint. Everything in
    // polled stays alive until it is cleared from in_flight_ at the end, so
    // first is safe to log through here.
    std::string xml;
    request.GetXML(xml, true);
    first->logger_->msg(Arc::DEBUG, "Bulk query for %u transfers at %s:\n%s",
                        (unsigned int)by_id.size(), endpoint_str_, xml);

    Arc::PayloadSOAP* response = NULL;
    std::string fault;

    if (Exchange(request, response, fault) != ExchangeOK) {
      // A query covers the whole endpoint, so it fails for everything here or
      // for nothing. The count of consecutive failures therefore belongs to the
      // connection rather than to any one transfer: one dead socket is one
      // fault, not one per transfer waiting on it.
      unsigned int faults;
      {
        std::unique_lock<std::mutex> lock(state_lock_);
        faults = ++faults_;
      }
      bool give_up = (faults >= max_query_faults);
      for (auto const& entry : by_id) {
        entry.second->QueryFailed("Failed to query state: " + fault, give_up);
      }
      delete response;
    } else {
      {
        std::unique_lock<std::mutex> lock(state_lock_);
        faults_ = 0;
      }
      response->GetXML(xml, true);
      first->logger_->msg(Arc::DEBUG, "Bulk query response:\n%s", xml);

      Arc::XMLNode results = (*response)["DataDeliveryQueryResponse"]["DataDeliveryQueryResult"];
      for (Arc::XMLNode resultnode = results["Result"]; resultnode; ++resultnode) {
        if (!resultnode["ResultCode"]) {
          continue;
        }
        std::string id((std::string)resultnode["ID"]);
        auto i = by_id.find(id);
        if (i == by_id.end()) {
          first->logger_->msg(Arc::WARNING, "Unexpected DTR %s in bulk query response", id);
          continue;
        }
        i->second->ApplyQueryResult(resultnode);
        by_id.erase(i);
      }

      delete response;

      // Anything still in by_id was asked about but not answered for. It keeps
      // its previous status and is asked about again on the next sweep. This is
      // about one transfer, so it goes to that transfer's own log.
      for (auto const& entry : by_id) {
        entry.second->logger_->msg(Arc::WARNING,
                                   "Delivery service at %s gave no result for this transfer",
                                   endpoint_str_);
      }
    }

    {
      std::unique_lock<std::mutex> lock(state_lock_);
      for (auto comm : polled) {
        in_flight_.erase(comm);
      }
    }
    poll_done_.notify_all();
  }

  DataDeliveryRemoteComm::DataDeliveryRemoteComm(DTR_ptr dtr, const TransferParameters& params)
    : DataDeliveryComm(dtr, params),
      client(NULL),
      poller_(NULL),
      dtr_full_id(dtr->get_id()),
      endpoint(dtr->get_delivery_endpoint()),
      timeout(dtr->get_usercfg().Timeout()),
      valid(false) {

    {
      std::unique_lock<std::mutex> lock(lock_);
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
    else if (!dtr->get_mapped_destination().empty()) {
      durl = dtr->get_mapped_destination();
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

    // This connection carries the delegation and nothing else - the transfer is
    // polled over the endpoint's shared channel from here on. Saying so lets the
    // service drop it as soon as it has answered, instead of holding a thread on
    // it until the idle timeout.
    std::multimap<std::string, std::string> http_attr;
    http_attr.insert(std::make_pair("KEEPALIVE", "false"));

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client->process(http_attr, &request, &response);

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
      std::string err("SOAP fault: " + soap_fault_code(fault.Code()));
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

    // The transfer is running. Querying and cancelling it need no credentials
    // of their own, so from here on it shares one connection with every other
    // transfer at this endpoint. The connection that carried the delegation has
    // done its job.
    EndpointPoller* poller = EndpointPoller::Get(endpoint);
    {
      std::unique_lock<std::mutex> lock(lock_);
      poller_ = poller;
      delete client;
      client = NULL;
    }

    poller->Add(this);
  }

  DataDeliveryRemoteComm::~DataDeliveryRemoteComm() {
    // If transfer is still going, send cancellation request to service
    if (valid) CancelDTR();
    {
      std::unique_lock<std::mutex> lock(lock_);
      delete client;
      client = NULL;
    }
    // Set once when the transfer started and never cleared, so it needs no
    // lock to read here. Remove() waits for any poll of this transfer to
    // finish, and closes the shared connection if this was the last transfer
    // at the endpoint. Nothing clears poller_ beforehand: until this call the
    // transfer is a legitimate member of the poller's list, and after it the
    // transfer is out of that list and about to be gone. Clearing it first
    // would leave it registered but disowned, a state every sweep would then
    // have to test for.
    if (poller_) {
      poller_->Remove(this);
    }
  }

  void DataDeliveryRemoteComm::CancelDTR() {
    // poller_ and dtr_full_id are both set when the transfer started and
    // never change afterwards, so neither needs the lock to read.
    if (!poller_) {
      return;
    }

    Arc::NS ns;
    Arc::PayloadSOAP request(ns);
    Arc::XMLNode dtrnode = request.NewChild("DataDeliveryCancel").NewChild("DTR");

    dtrnode.NewChild("ID") = dtr_full_id;

    std::string xml;
    request.GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;
    std::string fault;

    if (poller_->Exchange(request, response, fault) != ExchangeOK) {
      logger_->msg(Arc::ERROR, "Failed to cancel transfer request: %s", fault);
      delete response;
      return;
    }

    response->GetXML(xml, true);
    logger_->msg(Arc::DEBUG, "Response:\n%s", xml);

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

  void DataDeliveryRemoteComm::ApplyQueryResult(Arc::XMLNode resultnode) {
    std::unique_lock<std::mutex> lock(lock_);
    FillStatus(resultnode);
  }

  void DataDeliveryRemoteComm::QueryFailed(const std::string& err, bool give_up) {
    std::unique_lock<std::mutex> lock(lock_);
    if (!poller_) {
      return;
    }
    if (!give_up) {
      // Status is left alone - the connection has already been replaced, and
      // this only records that the transfer went unpolled this time round.
      logger_->msg(Arc::WARNING, err);
      status_.timestamp = time(NULL);
      return;
    }
    logger_->msg(Arc::ERROR, "%s", err);
    status_.commstatus = CommFailed;
    strncpy(status_.error_desc, "SOAP error in connection with delivery service", sizeof(status_.error_desc));
    status_.error_desc[sizeof(status_.error_desc)-1] = 0;
    valid = false;
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

    // One request and done, so the service need not keep the connection.
    std::multimap<std::string, std::string> http_attr;
    http_attr.insert(std::make_pair("KEEPALIVE", "false"));

    Arc::PayloadSOAP *response = NULL;
    Arc::MCC_Status status = client.process(http_attr, &request, &response);

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
      std::string err("SOAP fault: " + soap_fault_code(fault.Code()));
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

    // Every transfer at an endpoint is queried in one request, and there is no
    // one-at-a-time path to fall back to. A service too old to advertise
    // BulkQuery answers such a request only in part - it stops at the first
    // transfer still running - so it is refused outright rather than left to
    // lose status updates for everything after that transfer in every query.
    std::string bulk((std::string)resultnode["BulkQuery"]);
    if (bulk != "true" && bulk != "1") {
      dtr->get_logger()->msg(Arc::ERROR, "Delivery service at %s does not support querying several"
                                         " transfers at once and is too old to be used",
                             dtr->get_delivery_endpoint().str());
      delete response;
      return false;
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
    const std::string& token = usercfg.OToken();

    if (credentials.empty() && (key.empty() || cert.empty()) && token.empty()) {
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

    if(token.empty()) {
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
    } else {
      Arc::NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
      Arc::XMLNode deleg_token = op.NewChild("deleg:DelegatedToken",ns);
      deleg_token.NewAttribute("deleg:Format")="token";
      deleg_token.NewChild("deleg:Id")="";
      deleg_token.NewChild("deleg:Value")=token;
    }

    return true;
  }

} // namespace DataStaging
