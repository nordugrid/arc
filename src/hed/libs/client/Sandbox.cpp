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

#include <Sandbox.h>
namespace Arc {

  Logger Sandbox::logger(Logger::getRootLogger(), "Sandbox");

  bool Sandbox::Add(JobDescription& jobdesc, XMLNode& info) {
    logger.msg(DEBUG, "Adding job info to sandbox");
    // Create sandbox XML node
    JobInnerRepresentation jir;
    if (jobdesc.getInnerRepresentation(jir))
      for (std::list<URL>::iterator it = jir.OldJobIDs.begin();
           it != jir.OldJobIDs.end(); it++)
        info.NewChild("OldJobID") = it->str();
    else
      logger.msg(INFO, "Unable to get inner representation of job");
    // Store original job description string
    std::string rep;
    jobdesc.getSourceString(rep);
    info.NewChild("JobDescription") = (std::string)rep;

    // Getting cksum of input files
    std::vector<std::pair<std::string, std::string> > FileList;
    jobdesc.getUploadableFiles(FileList);

    NS ns;
    XMLNode LocalInput(ns, "LocalInputFiles");
    // Loop over input files
    for (std::vector<std::pair<std::string, std::string> >::iterator file = FileList.begin();
         file != FileList.end(); file++) {
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
    while (!buffer.eof_read() && !buffer.error()) {
      if (buffer.eof_read())
        std::cout << IString("eof_read()") << std::endl;
      if (buffer.error())
        std::cout << IString("error()") << std::endl;
      buffer.wait_read();
    }
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
