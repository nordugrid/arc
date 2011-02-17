#ifndef DATA_DELIVERY_COMM_H_
#define DATA_DELIVERY_COMM_H_

#include <arc/Run.h>
#include <arc/Thread.h>
#include <arc/Logger.h>

#include "DTR.h"

namespace DataStaging {

class DataDelivery;
class DataDeliveryCommHandler;

class DataDeliveryComm {
 friend class DataDeliveryCommHandler;
 public:
  typedef enum {
    CommInit,    // initializing/starting child, rest of information not valid
    CommNoError, // communication going on smoothly
    CommTimeout, // communication experienced timeout
    CommClosed,  // communication channel was closed
    CommExited,  // child exited. Mostly same as CommClosed but exit 
                 // detected before pipe closed.
    CommFailed   // child exited with exit code != 0. Child reports error 
                 // in such way. If we have CommFailed and no error code 
                 // reported that normally means segfault or external kill.
  } CommStatusType;
  #pragma pack(4)
  typedef struct { 
    // just plain C struct
    // easy to serialize
    CommStatusType commstatus; // communication state (filled by parent)
    time_t timestamp; // time when information was generated (filled by child)
    DTRStatus::DTRStatusType status; // generic status
    DTRErrorStatus::DTRErrorStatusType error; // error
    char error_desc[256]; // error description
    unsigned int streams; // number of transfer streams active
    unsigned long long int transfered; // number of bytes transfered
    unsigned long long int offset; // last position to which file has no missing pieces 
    unsigned long long int size; // file size as obtained by protocol
    unsigned int speed; // current transfer speed in bytes/sec duiring last ~minute
  } Status;
  #pragma pack()

 protected:
  Glib::Mutex lock_;
  Status status_;
  Status status_buf_;
  unsigned int status_pos_;
  Arc::Run* child_;
  std::string errstr_;
  DataDeliveryCommHandler* handler_;
  std::string dtr_id;
  Arc::Logger* logger_;
  void PullStatus(void);

 public:
  // Starts external executable with proper parameters
  // derived from DTR
  DataDeliveryComm(const DTR& dtr);
  // Destroy object also stoping managed executable
  ~DataDeliveryComm(void);
  // Obtain status of transfer
  Status GetStatus(void) const;
  const std::string GetError(void) const { return errstr_; };
  // Other actions releated to delivery go here

  operator bool(void) { return (child_ != NULL); };
  bool operator!(void) { return (child_ == NULL); };
};

} // namespace DataStaging

#endif // DATA_DELIVERY_COMM_H_
