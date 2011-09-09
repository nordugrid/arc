#ifndef DATA_DELIVERY_COMM_H_
#define DATA_DELIVERY_COMM_H_

#include "DTR.h"

namespace DataStaging {

  class DataDeliveryCommHandler;

  /// This class provides an abstract interface for the Delivery layer.
  /**
   * Different implementations provide different ways of providing Delivery
   * functionality. DataDeliveryLocalComm launches a local process to perform
   * the transfer and DataDeliveryRemoteComm contacts a remote service which
   * performs the transfer. The implementation is chosen depending on what is
   * set in the DTR, which the Scheduler should set based on various factors.
   *
   * CreateInstance() should be used to get a pointer to the instantiated
   * object. This also starts the transfer. Deleting this object stops the
   * transfer and cleans up any used resources. A singleton instance of
   * DataDeliveryCommHandler regularly polls all active transfers using
   * PullStatus() and fills the Status object with current information,
   * which can be obtained through GetStatus().
   */
  class DataDeliveryComm {

    friend class DataDeliveryCommHandler;

   public:
    /// Communication status with transfer
    enum CommStatusType {
      CommInit,    ///< Initializing/starting transfer, rest of information not valid
      CommNoError, ///< Communication going on smoothly
      CommTimeout, ///< Communication experienced timeout
      CommClosed,  ///< Communication channel was closed
      CommExited,  ///< Transfer exited. Mostly same as CommClosed but exit detected before pipe closed
      CommFailed   ///< Transfer failed. If we have CommFailed and no error code
                   ///< reported that normally means segfault or external kill.
    };
    #pragma pack(4)
    /// Plain C struct to pass information from executing process back to main thread
    struct Status {
      CommStatusType commstatus;         ///< Communication state (filled by main thread)
      time_t timestamp;                  ///< Time when information was generated (filled externally)
      DTRStatus::DTRStatusType status;   ///< Generic status
      DTRErrorStatus::DTRErrorStatusType error; ///< Error type
      DTRErrorStatus::DTRErrorLocation error_location; ///< Where error happened
      char error_desc[256];              ///< Error description
      unsigned int streams;              ///< Number of transfer streams active
      unsigned long long int transferred;///< Number of bytes transferred
      unsigned long long int offset;     ///< Last position to which file has no missing pieces
      unsigned long long int size;       ///< File size as obtained by protocol
      unsigned int speed;                ///< Current transfer speed in bytes/sec during last ~minute
      char checksum[128];                ///< Calculated checksum
    };
    #pragma pack()

   protected:

    /// Current status of transfer
    Status status_;
    /// Latest status of transfer is read into this buffer
    Status status_buf_;
    /// Reading position of Status buffer
    unsigned int status_pos_;
    /// Lock to protect access to status
    Glib::Mutex lock_;
    /// Pointer to singleton handler of all DataDeliveryComm objects
    DataDeliveryCommHandler* handler_;
    /// ID of the DTR this object is handling. Used in log messages.
    std::string dtr_id;
    /// Transfer limits
    TransferParameters transfer_params;
    /// Time transfer was started
    Arc::Time start_;
    /// Logger object. Pointer to DTR's Logger.
    Arc::Logger* logger_;

    /// Check for new state and fill state accordingly.
    /**
     * This method is periodically called by the comm handler to obtain status
     * info. It detects communication and delivery failures and delivery
     * termination.
     */
    virtual void PullStatus() = 0;

    /// Start transfer with parameters taken from DTR and supplied transfer limits.
    /** Constructor should not be used directly, CreateInstance() should be used
     * instead. */
    DataDeliveryComm(const DTR& dtr, const TransferParameters& params);

   public:
    /// Factory method to get concrete instance
    static DataDeliveryComm* CreateInstance(const DTR& dtr, const TransferParameters& params);

    /// Destroy object. This stops any ongoing transfer and cleans up resources.
    virtual ~DataDeliveryComm() {};

    /// Obtain status of transfer
    Status GetStatus() const;

    /// Get explanation of error
    std::string GetError() const { return status_.error_desc; };

    /// Returns true if transfer is currently active
    virtual operator bool() const = 0;
    /// Returns true if transfer is currently not active
    virtual bool operator!() const = 0;
  };

  /// Singleton class handling all active DataDeliveryComm objects
  class DataDeliveryCommHandler {

   private:
    Glib::Mutex lock_;
    static void func(void* arg);
    std::list<DataDeliveryComm*> items_;
    static DataDeliveryCommHandler* comm_handler;

    /// Constructor is private - getInstance() should be used instead
    DataDeliveryCommHandler();
    DataDeliveryCommHandler(const DataDeliveryCommHandler&);
    DataDeliveryCommHandler& operator=(const DataDeliveryCommHandler&);

   public:
    ~DataDeliveryCommHandler() {};
    /// Add a new DataDeliveryComm instance to the handler
    void Add(DataDeliveryComm* item);
    /// Remove a DataDeliveryComm instance from the handler
    void Remove(DataDeliveryComm* item);
    /// Get the singleton instance of the handler
    static DataDeliveryCommHandler* getInstance();
  };

} // namespace DataStaging

#endif // DATA_DELIVERY_COMM_H_
