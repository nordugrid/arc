// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "ByteArray.h"

namespace Arc {

  void ByteArray::init(size_t length) {
    size_t new_length = length + BYTE_ARRAY_PAD_SIZE;
    data_ = (char*)calloc(new_length, sizeof(char));
    if (data_ == NULL)
      throw MemoryAllocationException();
    size_ = 0;
    length_ = new_length;
  }

  ByteArray::ByteArray(size_t length) {
    init(length);
  }

  ByteArray::ByteArray(void *buf, size_t s) {
    init(s);
    append(buf, s);
  }

  ByteArray::~ByteArray() {
    if (data_ != NULL)
      free(data_);
    size_ = 0;
    length_ = 0;
  }

  void ByteArray::resize(size_t new_size) {
    size_t new_length = new_size + BYTE_ARRAY_PAD_SIZE;
    char *new_data = (char*)calloc(new_length, sizeof(char));
    if (new_data == NULL)
      throw MemoryAllocationException();
    // memmove?
    memcpy(new_data, data_, size_);
    free(data_);
    data_ = new_data;
    length_ = new_length;
  }

  void ByteArray::append(const void *buf, size_t s) {
    unsigned int new_size = size_ + s;
    if (new_size > length_)
      resize(new_size);
    memcpy(data_ + size_, buf, s);
    size_ = new_size;
  }

  void ByteArray::append(const std::string& str) {
    append(str.c_str(), str.size() + 1);
  }

  void ByteArray::append(ByteArray& buf) {
    size_t s = buf.size();
    append(&s, sizeof(size_t));
    append(buf.data(), buf.size());
  }

  void ByteArray::clean(void) {
    memset(data_, 0, length_);
    size_ = 0;
  }

  ByteArray::operator std::string(void) const {
    std::ostringstream o;
    for (int i = 0; i < length_; i++) {
      o << std::hex << std::setiosflags(std::ios_base::showbase);
      o << (int)data_[i] << " ";
    }
    return o.str();
  }

} // namespace Arc
