#include <iostream>

#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>

#include "JobARC0.h"

namespace Arc {

  JobARC0::JobARC0(Config *cfg)
    : Job(cfg) {}

  JobARC0::~JobARC0() {}

  ACC *JobARC0::Instance(Config *cfg, ChainContext *) {
    return new JobARC0(cfg);
  }

  void JobARC0::GetStatus() {

    DataHandle handler(InfoEndpoint);
    DataBufferPar buffer;
    if (!handler->StartReading(buffer))
      return;

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
	result.append(buffer[handle], length);
	buffer.is_written(handle);
      }

    if (!handler->StopReading())
      return;

    std::cout << "Job info: " << result << std::endl;

  }

  void JobARC0::Kill() {}

} // namespace Arc
