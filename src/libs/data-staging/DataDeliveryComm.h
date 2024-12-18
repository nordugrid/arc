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
   * \ingroup datastaging
   * \headerfile DataDeliveryComm.h arc/data-staging/DataDeliveryComm.h
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
    /** \ingroup datastaging */
    struct Status {
      CommStatusType commstatus;         ///< Communication state (filled by main thread)
      time_t timestamp;                  ///< Time when information was generated (filled externally)
      DTRStatus::DTRStatusType status;   ///< Generic status
      DTRErrorStatus::DTRErrorStatusType error; ///< Error type
      DTRErrorStatus::DTRErrorLocation error_location; ///< Where error happened
      char error_desc[1024];             ///< Error description
      unsigned int streams;              ///< Number of transfer streams active
      unsigned long long int transferred;///< Number of bytes transferred
      unsigned long long int offset;     ///< Last position to which file has no missing pieces
      unsigned long long int size;       ///< File size as obtained by protocol
      unsigned int speed;                ///< Current transfer speed in bytes/sec during last ~minute
      char checksum[128];                ///< Calculated checksum
      unsigned long long int transfer_time; ///< Time in ns to complete transfer (0 if not completed)
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
    /// Transfer limits
    TransferParameters transfer_params;
    /// Time transfer was started
    Arc::Time start_;
    /// Logger object. Pointer to DTR's Logger.
    DTRLogger logger_;

    /// Check for new state and fill state accordingly.
    /**
     * This method is periodically called by the comm handler to obtain status
     * info. It detects communication and delivery failures and delivery
     * termination.
     */
    virtual void PullStatus() = 0;

    /// Returns identifier of the handler/delivery service this object uses to perform transfers.
    virtual std::string DeliveryId() const = 0;

    /// Start transfer with parameters taken from DTR and supplied transfer limits.
    /**
     * Constructor should not be used directly, CreateInstance() should be used
     * instead.
     */
    DataDeliveryComm(DTR_ptr dtr, const TransferParameters& params);

    /// Access handler used for this DataDeliveryComm object
    DataDeliveryCommHandler& GetHandler();

   private:
    /// Pointer to handler used for this DataDeliveryComm object
    DataDeliveryCommHandler* handler_;

   public:
    /// Factory method to get DataDeliveryComm instance.
    static DataDeliveryComm* CreateInstance(DTR_ptr dtr, const TransferParameters& params);

    /// Destroy object. This stops any ongoing transfer and cleans up resources.
    virtual ~DataDeliveryComm() {};

    /// Obtain status of transfer
    Status GetStatus() const;

    /// Check the delivery method is available. Calls CheckComm of the appropriate subclass.
    /**
     * \param dtr DTR from which credentials are used
     * \param allowed_dirs filled with list of dirs that this comm is allowed
     * to read/write
     * \param load_avg filled with the load average reported by the service
     * \return true if selected delivery method is available
     */
    static bool CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg);

    /// Get explanation of error
    std::string GetError() const { return status_.error_desc; };

    /// Returns true if transfer is currently active
    virtual operator bool() const = 0;
    /// Returns true if transfer is currently not active
    virtual bool operator!() const = 0;
  };

  /// Singleton class handling all active DataDeliveryComm objects
  /**
   * \ingroup datastaging
   * \headerfile DataDeliveryComm.h arc/data-staging/DataDeliveryComm.h
   */
  class DataDeliveryCommHandler {

   private:
    Glib::Mutex lock_;
    static void func(void* arg);
    std::list<DataDeliveryComm*> items_;
    static Glib::Mutex comm_lock;
    static std::map<std::string, DataDeliveryCommHandler*> comm_handler;

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
    /// Get the instance of the handler for specified delivery id
    static DataDeliveryCommHandler* getInstance(std::string const & id);
  };

} // namespace DataStaging

#endif // DATA_DELIVERY_COMM_H_
