#ifndef DATADELIVERYSERVICE_H_
#define DATADELIVERYSERVICE_H_

#include <string>

#include <arc/infosys/RegisteredService.h>
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
   */
  class DataDeliveryService: public Arc::RegisteredService, DTRCallback {

   private:
    /// Construct a SOAP error message with optional extra reason string
    Arc::MCC_Status make_soap_fault(Arc::Message& outmsg, const std::string& reason = "");
    /// DataDeliveryService namespace
    Arc::NS ns;
    /// Process limit read from cache service configuration
    unsigned int max_processes;
    /// Current processes - using gint to guarantee atomic thread-safe operations
    gint current_processes;
    /// Flag to say whether DataDeliveryService is valid
    bool valid;
    /// Internal list of active DTRs, mapped to the stream with the transfer log
    std::map<DTR*, std::stringstream*> active_dtrs;
    /// Lock for DTRs list
    Arc::SimpleCondition active_dtrs_lock;
    /// Archived list of finished DTRs, just ID and final state
    /// TODO: save to file, DB?
    std::map<std::string, std::string> archived_dtrs;
    /// Object to manage Delivery processes
    DataDelivery delivery;
    /// Logger object
    static Arc::Logger logger;

    /// Static version of ArchivalThread, used when thread is created
    static void ArchivalThread(void* arg);
    /// Archival thread
    void ArchivalThread(void);

    /* individual operations */
    /// Start a new transfer
    Arc::MCC_Status Start(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& user);

    /// Query status of transfer
    Arc::MCC_Status Query(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& user);

    /// Cancel a transfer
    Arc::MCC_Status Cancel(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& user);

   public:
    /// Make a new DataDeliveryService. Sets up the process handler.
    DataDeliveryService(Arc::Config *cfg);
    /// Destroy the DataDeliveryService
    virtual ~DataDeliveryService();

    /// Main method called by HED when service is invoked. Directs call to appropriate internal method.
    virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);

    /// Implementation of callback method from DTRCallback
    virtual void receiveDTR(DTR& dtr);

    /// Supplies information on the service for use in the information system.
    bool RegistrationCollector(Arc::XMLNode &doc);

    /// Returns true if the CacheService is valid.
    operator bool() const { return valid; };
    /// Returns true if the CacheService is not valid.
    bool operator!() const { return !valid; };
  };

} // namespace DataStaging

#endif /* DATADELIVERYSERVICE_H_ */
