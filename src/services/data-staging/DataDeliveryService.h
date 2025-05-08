#ifndef DATADELIVERYSERVICE_H_
#define DATADELIVERYSERVICE_H_

#include <string>

#include <arc/delegation/DelegationInterface.h>
#include <arc/message/Service.h>
#include <arc/message/Message.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>

#include <arc/data-staging/DataDelivery.h>

namespace DataStaging {

  /// Service for the Delivery layer of data staging.
  /**
   * This service starts and controls data transfers. It assumes that the
   * files in any request submitted are ready for immediate transfer and
   * so do not need to be resolved or prepared in any way.
   *
   * It implements DTRCallback to get callbacks when a DTR has finished
   * transfer.
   *
   * Status codes in results returned:
   *  - OK             - successful submission/cancellation
   *  - TRANSFERRING   - transfer still ongoing
   *  - TRANSFERRED    - transfer finished successfully
   *  - TRANSFER_ERROR - transfer failed
   *  - SERVICE_ERROR  - something went wrong in the service itself
   *
   * An internal list of active transfers is held in memory. After the first
   * query of a finished transfer (successful or not) the DTR is moved to an
   * archived list where only summary information is kept about the transfer
   * (DTR ID, state and short error description). The DTR object is then
   * deleted. This archived list is also kept in memory. In case a transfer is
   * never queried, a separate thread moves any transfers which completed more
   * than one hour ago to the archived list.
   */
  class DataDeliveryService: public Arc::Service, DTRCallback {

    /// Managed pointer to stringstream used to hold log output
    typedef Arc::ThreadedPointer<std::stringstream> sstream_ptr;

   private:
    /// Construct a SOAP error message with optional extra reason string
    Arc::MCC_Status make_soap_fault(Arc::Message& outmsg, const std::string& reason = "");
    /// DataDeliveryService namespace
    Arc::NS ns;
    /// Directories the service is allowed to copy files from or to
    std::list<std::string> allowed_dirs;
    /// Process limit read from cache service configuration
    unsigned int max_processes;
    /// Current processes - using gint to guarantee atomic thread-safe operations
    gint current_processes;
    /// Internal list of active DTRs, mapped to the stream with the transfer log
    std::map<DTR_ptr, sstream_ptr> active_dtrs;
    /// Lock for active DTRs list
    Arc::SimpleCondition active_dtrs_lock;
    /// Archived list of finished DTRs, just ID and final state and short explanation
    /// TODO: save to file, DB?
    std::map<std::string, std::pair<std::string, std::string> > archived_dtrs;
    /// Lock for archive DTRs list
    Arc::SimpleCondition archived_dtrs_lock;
    /// Object to manage Delivery processes
    DataDelivery delivery;
    /// Container for delegated credentials
    Arc::DelegationContainerSOAP delegation;
    /// Directory in which to store temporary delegated proxies
    std::string tmp_proxy_dir;
    /// Root logger destinations, to use when logging messages in methods
    /// called from Delivery layer where root logger is disabled
    std::list<Arc::LogDestination*> root_destinations;
    /// Logger object
    static Arc::Logger logger;

    /// Log a message to root destinations
    void LogToRootLogger(Arc::LogLevel level, const std::string& message);

    /// Static version of ArchivalThread, used when thread is created
    static void ArchivalThread(void* arg);
    /// Archival thread
    void ArchivalThread(void);

    /// Sanity check on file sources and destinations
    bool CheckInput(const std::string& url, const Arc::UserConfig& usercfg,
                    Arc::XMLNode& resultelement, bool& require_credential_file);

    /* individual operations */
    /// Start a new transfer
    Arc::MCC_Status Start(Arc::XMLNode in, Arc::XMLNode out);

    /// Query status of transfer
    Arc::MCC_Status Query(Arc::XMLNode in, Arc::XMLNode out);

    /// Cancel a transfer
    Arc::MCC_Status Cancel(Arc::XMLNode in, Arc::XMLNode out);

    /// Check service is ok and return service information
    Arc::MCC_Status Ping(Arc::XMLNode in, Arc::XMLNode out);

   public:
    /// Make a new DataDeliveryService. Sets up the process handler.
    DataDeliveryService(Arc::Config *cfg, Arc::PluginArgument* parg);
    /// Destroy the DataDeliveryService
    virtual ~DataDeliveryService();

    /// Main method called by HED when service is invoked. Directs call to appropriate internal method.
    virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);

    /// Implementation of callback method from DTRCallback
    virtual void receiveDTR(DTR_ptr dtr);

  };

} // namespace DataStaging

#endif /* DATADELIVERYSERVICE_H_ */
