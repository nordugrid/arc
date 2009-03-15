// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BYTE_ARRAY_H__
#define __ARC_BYTE_ARRAY_H__

#include <string>
#define BYTE_ARRAY_PAD_SIZE (1024)

namespace Arc {

  class MemoryAllocationException
    : public std::exception {
    virtual const char* what() const throw() {
      return "Memory allocation error ";
    }
  };

  class ByteArray {
  private:
    char *data_;
    size_t size_;
    size_t length_;
    void resize(size_t new_size);
    void init(size_t length);
  public:
    ByteArray() {
      data_ = NULL;
      size_ = 0;
      length_ = 0;
    }
    ByteArray(size_t length);
    ByteArray(void *buf, size_t size);
    ~ByteArray();
    void append(const void *buf, size_t size);
    void append(const std::string& str);
    void append(ByteArray& buf);
    void clean(void);
    char* data(void) {
      return data_;
    }
    size_t size(void) {
      return size_;
    }
    operator std::string(void) const;
  };

} // namespace Arc

#endif // __ARC_BYTE_ARRAY_H__
