// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/CheckSum.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/client/JobDescription.h>

#include <Sandbox.h>

namespace Arc {

  Logger Sandbox::logger(Logger::getRootLogger(), "Sandbox");

  bool Sandbox::Add(const JobDescription& jobdesc, XMLNode& info) {
    logger.msg(DEBUG, "Adding job info to sandbox");
    // Create sandbox XML node
    for (std::list<std::string>::const_iterator it = jobdesc.Identification.ActivityOldId.begin();
         it != jobdesc.Identification.ActivityOldId.end(); it++)
      info.NewChild("OldJobID") = *it;
    // Store original job description string
    // - this is not quite the original - needs fixing
    std::string rep = jobdesc.UnParse("ARCJSDL");
    info.NewChild("JobDescription") = (std::string)rep;

    // Getting cksum of input files
    std::list<std::pair<std::string, std::string> > FileList;
    jobdesc.getUploadableFiles(FileList);

    NS ns;
    XMLNode LocalInput(ns, "LocalInputFiles");
    // Loop over input files
    for (std::list<std::pair<std::string, std::string> >::iterator
         file = FileList.begin(); file != FileList.end(); file++) {
      std::string cksum = GetCksum(file->second);
      XMLNode File(ns, "File");
      File.NewChild("Source") =
        (std::string)file->second;
      File.NewChild("CheckSum") =
        (std::string)cksum;
      LocalInput.NewChild(File);
    }
    info.NewChild(LocalInput);

    return true;
  }

  std::string Sandbox::GetCksum(const std::string& file) {

    std::string cksum = "";
    DataHandle source(file);

    // create buffer
    unsigned long int bufsize;
    bufsize = 65536;
    if (source->BufSize() > bufsize)
      bufsize = source->BufSize();
    bufsize = bufsize * 2;
    // prepare checksum
    CheckSumAny *CkSum = new CheckSumAny(CheckSumAny::md5);
    DataBuffer buffer;
    buffer.set(CkSum, bufsize);
    if (!buffer)
      logger.msg(WARNING, "Buffer creation failed !");
    DataStatus res = DataStatus::Success;
    if (!(res = source->StartReading(buffer))) {
      logger.msg(WARNING, "Failed to start reading from source: %s",
                 source->str());
      delete CkSum;
      return cksum;
    }
    buffer.speed.reset();
    DataStatus read_failure = DataStatus::Success;
    logger.msg(INFO, "Waiting for buffer");

    int handle;
    unsigned int length;
    unsigned long long int offset;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true))
        buffer.is_written(handle);

    logger.msg(INFO, "buffer: read eof : %i", (int)buffer.eof_read());
    logger.msg(INFO, "buffer: error    : %i", (int)buffer.error());
    logger.msg(INFO, "Closing read channel");
    read_failure = source->StopReading();

    if (buffer.checksum_valid()) {
      char buf[100];
      CkSum->print(buf, 100);
      source->SetCheckSum(buf);
      cksum = source->GetCheckSum();
    }

    delete CkSum;
    return cksum;
  }

} // namespace Arc
