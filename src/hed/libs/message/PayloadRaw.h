#ifndef __ARC_PAYLOADRAW_H__
#define __ARC_PAYLOADRAW_H__

#include <vector>

#include "Message.h"

namespace Arc {

/** Virtual interface for managing arbitrarily accessible Message payload.
  This class implements memory-resident or memory-mapped content made 
  of optionally multiple chunks/buffers. Buffers have own size and offset.
  This class is purely virtual. */
class PayloadRawInterface: public MessagePayload {
 public:
  PayloadRawInterface(void) { };
  virtual ~PayloadRawInterface(void) { };
  /** Returns content of byte at specified position. Specified position 'pos' 
    is treated as global one and goes through all buffers placed one 
    after another. */  
  virtual char operator[](int pos) const = 0;
  /** Get pointer to buffer content at global position 'pos'. By default to
    beginning of main buffer whatever that means. */
  virtual char* Content(int pos = -1) = 0;
  /** Returns logical size of whole structure. */
  virtual int Size(void) const = 0;
  /**  Create new buffer at global position 'pos' of size 'size'. */
  virtual char* Insert(int pos = 0,int size = 0) = 0;
  /** Create new buffer at global position 'pos' of size 'size'.
    Created buffer is filled with content of memory at 's'.
    If 'size' is 0 content at 's' is expected to be null-terminated. */
  virtual char* Insert(const char* s,int pos = 0,int size = 0) = 0;
  /** Returns pointer to num'th buffer */
  virtual char* Buffer(unsigned int num) = 0;
  /** Returns length of num'th buffer */
  virtual int BufferSize(unsigned int num) const = 0;
  /** Returns position of num'th buffer */
  virtual int BufferPos(unsigned int num) const = 0;
  /** Change size of stored information.
    If size exceeds end of allocated buffer, buffers are not 
    re-allocated, only logical size is extended.
    Buffers with location behind new size are deallocated. */
  virtual bool Truncate(unsigned int size) = 0;
};

/* Buffer type for PayloadRaw */
typedef struct {
    char* data;     /** pointer to buffer in memory */
    int size;       /** size of allocated mmemory */
    int length;     /** size of used memory - size of buffer */
    bool allocated; /** true if memory has to free by destructor */
} PayloadRawBuf;

/** Implementation of PayloadRawInterface - raw byte multi-buffer. */ 
class PayloadRaw: public PayloadRawInterface {
 protected:
  int offset_;
  int size_;
  std::vector<PayloadRawBuf> buf_; /** List of handled buffers. */
 public:
  /** Constructor. Created object contains no buffers. */
  PayloadRaw(void):offset_(0),size_(0) { };
  /** Destructor. Frees allocated buffers. */
  virtual ~PayloadRaw(void);
  virtual char operator[](int pos) const;
  virtual char* Content(int pos = -1);
  virtual int Size(void) const;
  virtual char* Insert(int pos = 0,int size = 0);
  virtual char* Insert(const char* s,int pos = 0,int size = 0);
  virtual char* Buffer(unsigned int num = 0);
  virtual int BufferSize(unsigned int num = 0) const;
  virtual int BufferPos(unsigned int num = 0) const;
  virtual bool Truncate(unsigned int size);
};

/** Returns pointer to main buffer of Message payload. 
  NULL if no buffer is present or if payload is not of PayloadRawInterface type. */ 
const char* ContentFromPayload(const MessagePayload& payload);

} // namespace Arc

#endif /* __ARC_PAYLOADRAW_H__ */
