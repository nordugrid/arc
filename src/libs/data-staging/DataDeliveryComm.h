#ifndef DATA_DELIVERY_COMM_H_
#define DATA_DELIVERY_COMM_H_

#include <arc/Run.h>
#include <arc/Thread.h>
#include <arc/Logger.h>

#include "DTR.h"

namespace DataStaging {

/// Singleton class handling all active DataDeliveryComm objects
class DataDeliveryCommHandler;

/// This class starts, monitors and controls a Delivery process
class DataDeliveryComm {

  friend class DataDeliveryCommHandler;

 public:
  /// Communication status with child process
  enum CommStatusType {
    CommInit,    ///< Initializing/starting child, rest of information not valid
    CommNoError, ///< Communication going on smoothly
    CommTimeout, ///< Communication experienced timeout
    CommClosed,  ///< Communication channel was closed
    CommExited,  ///< Child exited. Mostly same as CommClosed but exit detected before pipe closed
    CommFailed   ///< Child exited with exit code != 0. Child reports error
                 ///< in such way. If we have CommFailed and no error code
                 ///< reported that normally means segfault or external kill.
  };
  #pragma pack(4)
  /// Plain C struct for passing information from child process back to main thread
  struct Status {
    CommStatusType commstatus;         ///< Communication state (filled by parent)
    time_t timestamp;                  ///< Time when information was generated (filled by child)
    DTRStatus::DTRStatusType status;   ///< Generic status
    DTRErrorStatus::DTRErrorStatusType error; ///< Error type
    DTRErrorStatus::DTRErrorLocation error_location; ///< Where error happened
    char error_desc[256];              ///< Error description
    unsigned int streams;              ///< Number of transfer streams active
    unsigned long long int transfered; ///< Number of bytes transfered
    unsigned long long int offset;     ///< Last position to which file has no missing pieces
    unsigned long long int size;       ///< File size as obtained by protocol
    unsigned int speed;                ///< Current transfer speed in bytes/sec duiring last ~minute
    char checksum[128];                ///< Calculated checksum
  };
  #pragma pack()

 protected:
  /// Lock to protect access to child process
  Glib::Mutex lock_;
  /// Current status of transfer
  Status status_;
  /// Latest status from child is read into this buffer
  Status status_buf_;
  /// Reading position of Status buffer
  unsigned int status_pos_;
  /// Child process
  Arc::Run* child_;
  /// Pointer to singleton handler of all DataDeilveryComm objects
  DataDeliveryCommHandler* handler_;
  /// ID of the DTR this object is handling
  std::string dtr_id;
  /// Transfer limits
  TransferParameters transfer_params;
  /// Time last communication was received from child
  Arc::Time last_comm;
  /// Logger object
  Arc::Logger* logger_;

  /// Check for new state from child and fill state accordingly.
  /** Detects communication and delivery failures and delivery termination. */
  void PullStatus(void);

 public:
  /// Starts external executable with parameters taken from DTR and supplied transfer limits
  DataDeliveryComm(const DTR& dtr, const TransferParameters& params);

  /// Destroy object. This stops the child process
  ~DataDeliveryComm(void);

  /// Obtain status of transfer
  Status GetStatus(void) const;

  /// Get explanation of error
  const std::string GetError(void) const { return status_.error_desc; };

  /// Returns true child process exists
  operator bool(void) { return (child_ != NULL); };
  /// Returns true if child process does not exist
  bool operator!(void) { return (child_ == NULL); };
};

} // namespace DataStaging

#endif // DATA_DELIVERY_COMM_H_
