#include <vector>

#include "Data.h"

// Virtual interface for managing main part of content
class PayloadRawInterface: public MessagePayload {
 public:
  PayloadRawInterface(void) { };
  virtual ~PayloadRawInterface(void) { };
  // Get byte at specified position
  virtual char operator[](int pos) const = 0;
  // Get pointer to content at pos. By default to main part.
  virtual char* Content(int pos = -1) = 0;
  // Cumulative length of all records
  virtual int Size(void) const = 0;
  // Create a place for new record (filled with 0)
  virtual char* Insert(int pos = 0,int size = 0) = 0;
  // Acquire s as part object. It won't be freed. 
  // If size == 0 s is expected to be null-terminated
  virtual char* Insert(const char* s,int pos = 0,int size = 0) = 0;
  // Pointer to num'th buffer.
  virtual char* Buffer(int num) = 0;
  // Length of num'th record.
  virtual int BufferSize(int num) const = 0;
};

// Raw byte multi-buffer. Direct implementation of 
// PayloadRawInterface
class PayloadRaw: public PayloadRawInterface {
 public:
  typedef struct {
    char* data;
    int size;
    int length;
    bool allocated;
  } Buf;
 protected:
  std::vector<Buf> buf_;
 public:
  PayloadRaw(void) { };
  virtual ~PayloadRaw(void);
  virtual char operator[](int pos) const;
  virtual char* Content(int pos = -1);
  virtual int Size(void) const;
  virtual char* Insert(int pos = 0,int size = 0);
  virtual char* Insert(const char* s,int pos = 0,int size = 0);
  virtual char* Buffer(int num = 0);
  virtual int BufferSize(int num = 0) const;
};

const char* ContentFromPayload(const MessagePayload& payload);

