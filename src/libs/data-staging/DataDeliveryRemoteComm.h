#ifndef DATADELIVERYREMOTECOMM_H_
#define DATADELIVERYREMOTECOMM_H_

#include <condition_variable>
#include <set>

#include <arc/XMLNode.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>

#include "DataDeliveryComm.h"

namespace DataStaging {

  /// This class contacts a remote service to make a Delivery request.
  /**
   * \ingroup datastaging
   * \headerfile DataDeliveryRemoteComm.h arc/data-staging/DataDeliveryRemoteComm.h
   */
  class DataDeliveryRemoteComm : public DataDeliveryComm {
  public:
    /// Send the transfer request to the remote service.
    DataDeliveryRemoteComm(DTR_ptr dtr, const TransferParameters& params);
    /// If transfer is still ongoing, sends a cancellation message to the service.
    virtual ~DataDeliveryRemoteComm();

    /// Pings service to find allowed dirs
    static bool CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg);

    /// Returns true if service is still processing request
    virtual operator bool() const { return valid; };
    /// Returns true if service is not processing request or down
    virtual bool operator!() const { return !valid; };

  private:

    /// A SOAP connection shared by every transfer at one delivery endpoint.
    /**
     * Querying and cancelling carry no credentials of their own: the service
     * looks a transfer up by its ID, and authorises the caller at the endpoint
     * rather than per transfer. One connection can therefore serve every
     * transfer at that endpoint.
     *
     * Sharing also keeps the connection in use, which it has to be. The
     * service closes any connection left idle for its listen timeout - 60s,
     * and not settable from arc.conf - and does not tell the client, so a
     * connection which is not carrying queries is one which will be found
     * dead by whatever needs it next.
     *
     * Owned by the endpoint's poller, which opens it when the first transfer
     * registers and closes it when the last one leaves, so it exists only
     * while something is using it.
     */
    class QueryChannel {
     public:
      QueryChannel(const Arc::MCCConfig& cfg, const Arc::URL& endpoint, int timeout);
      ~QueryChannel();
      /// Drop the current connection and open a fresh one. Call with lock held.
      void ReconnectLocked();
      /// Serialises use of the connection, which carries one request at a time
      std::mutex lock;
      /// Current connection
      Arc::ClientSOAP* client;
     private:
      QueryChannel(const QueryChannel&);
      QueryChannel& operator=(const QueryChannel&);
      Arc::MCCConfig cfg;
      Arc::URL endpoint;
      int timeout;
    };

    /// Outcome of one exchange over the shared connection
    enum ExchangeResult {
      ExchangeOK,   ///< Response received and it is not a fault
      ExchangeFault ///< Something went wrong; the connection has been replaced
    };

    /// Polls every transfer at one delivery endpoint, from one thread.
    /**
     * One of these exists per endpoint. It owns the list of transfers running
     * there, the connection they share, and the thread that asks about them.
     *
     * A query is a network call which can block for the client timeout, so it
     * is made with the list lock released. The poller therefore holds raw
     * pointers to transfers it does not own while polling them, and Remove()
     * waits for that to finish before returning so its caller can destroy the
     * object safely.
     */
    class EndpointPoller {
     public:
      /// The poller for an endpoint, created with its thread on first use
      static EndpointPoller* Get(const Arc::URL& endpoint);
      /// Register a transfer, opening the shared connection if it is the first
      void Add(DataDeliveryRemoteComm* comm);
      /// Unregister a transfer.
      /** Blocks until any poll of it has finished, and closes the shared
          connection if it was the last one registered. */
      void Remove(DataDeliveryRemoteComm* comm);
      /// Send one request over the shared connection and collect the response.
      /**
       * On fault the connection is replaced before the channel lock is
       * released, since by far the most likely cause is the service having
       * closed it and the next user should not have to find that out again.
       */
      ExchangeResult Exchange(Arc::PayloadSOAP& request,
                              Arc::PayloadSOAP*& response, std::string& fault);
     private:
      EndpointPoller(const std::string& endpoint);
      EndpointPoller(const EndpointPoller&);
      EndpointPoller& operator=(const EndpointPoller&);
      static void func(void* arg);
      /// One polling pass over every transfer registered here
      void Sweep();

      /// Guards items_, in_flight_, channel_ and faults_.
      /** Those are what the sweep thread and a registering or departing
          transfer both touch. Distinct from QueryChannel::lock, which
          serialises use of the connection, and from the lock_ each
          DataDeliveryRemoteComm holds over its own status. Where this one and
          a transfer's are both held, this is taken first; QueryChannel::lock
          is only ever taken with this one released. */
      std::mutex state_lock_;
      /// Signalled when transfers finish being polled, so Remove() can return
      std::condition_variable poll_done_;
      std::list<DataDeliveryRemoteComm*> items_;
      /// Transfers currently being polled
      std::set<DataDeliveryRemoteComm*> in_flight_;
      /// Shared connection, open only while transfers are registered
      QueryChannel* channel_;
      /// Endpoint name, for logging
      std::string endpoint_str_;
      /// Consecutive failed queries at this endpoint.
      /** A query covers every transfer here, so it fails for all of them or
          for none, which makes this a property of the connection rather than
          of any one transfer. Reset as soon as a query gets through. */
      unsigned int faults_;
      /// Consecutive failures after which the transfers here are given up on.
      /** One sweep per second, so this is roughly that many seconds of an
          unreachable service. A transfer which registers while the count is
          already high inherits it rather than being given a fresh allowance:
          there is no sense in starting a new grace period against an endpoint
          already known to be failing. */
      static const unsigned int max_query_faults = 20;

      /// Pollers by endpoint, and its lock
      static std::mutex pollers_lock;
      static std::map<std::string, EndpointPoller*> pollers;
      /// Interval in milliseconds between polling sweeps
      static unsigned int poll_interval_ms;
    };

    /// Connection used to start the transfer.
    /** This one is per transfer because the credential delegation is bound to
        it. Closed as soon as the transfer has started, since everything after
        that goes over the shared channel. */
    Arc::ClientSOAP* client;
    /// Poller for this transfer's endpoint. NULL until the transfer starts.
    EndpointPoller* poller_;
    /// Full DTR ID
    std::string dtr_full_id;
    /// MCC configuration for connecting to service
    Arc::MCCConfig cfg;
    /// Endpoint of remote delivery service
    Arc::URL endpoint;
    /// Connection timeout
    int timeout;

    /// Flag to say whether transfer is running and service is still up
    bool valid;
    /// Logger object (main log, not DTR's log)
    static Arc::Logger logger;

    /// Cancel a DTR, by sending a cancel request to the service
    void CancelDTR();

    /// Fill Status object with data in node. If empty fields are initialised
    /// to default values.
    void FillStatus(const Arc::XMLNode& node = Arc::XMLNode());

    /// Set up delegation so the credentials can be used by the service
    bool SetupDelegation(Arc::XMLNode& op, const Arc::UserConfig& usercfg);

    /// Record a failed query of the delivery service.
    /** Whether to give up is decided by the poller, since a query covers the
        whole endpoint and a failure is a property of the connection rather
        than of this transfer. Until then the status is left alone and the
        transfer is simply asked about again on the next sweep. */
    void QueryFailed(const std::string& err, bool give_up);

    /// Fill status from one Result element of a query response, taking lock_
    void ApplyQueryResult(Arc::XMLNode resultnode);

  };

} // namespace DataStaging

#endif /* DATADELIVERYREMOTECOMM_H_ */
