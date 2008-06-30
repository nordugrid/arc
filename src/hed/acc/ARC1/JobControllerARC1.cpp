#include "JobControllerARC1.h"
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>
#include <arc/StringConv.h>

#include <map>
#include <iostream>
#include <algorithm>

namespace Arc {

  JobControllerARC1::JobControllerARC1(Arc::Config *cfg) : JobController(cfg, "ARC1") {}
  
  JobControllerARC1::~JobControllerARC1() {}

  ACC *JobControllerARC1::Instance(Config *cfg, ChainContext *) {
    return new JobControllerARC1(cfg);
  }
  
  void JobControllerARC1::GetJobInformation(){


  }

  void JobControllerARC1::DownloadJobOutput(bool keep, std::string downloaddir){
  
  }

  void JobControllerARC1::Clean(bool force){

  }

  void JobControllerARC1::Kill(bool keep){

  }

} // namespace Arc
