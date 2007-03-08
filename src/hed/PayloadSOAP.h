#include "Data.h"
#include "SOAPMessage.h"

// Combining DataPayload with SOAPMessage
class DataPayloadSOAP: public DataPayload, public SOAPMessage {
 public:
  // Create payload from SOAP message
  DataPayloadSOAP(const SOAPMessage& soap);
  // Create SOAP message from payload (PayloadRaw and derived classes
  // are supported).
  DataPayloadSOAP(const DataPayload& source);
  virtual ~DataPayloadSOAP(void);
};
