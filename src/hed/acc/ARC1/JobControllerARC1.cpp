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
  
  void JobControllerARC1::GetJobInformation(){}

  bool JobControllerARC1::GetThisJob(Job ThisJob, std::string downloaddir){};
  bool JobControllerARC1::CleanThisJob(Job ThisJob, bool force){};
  bool JobControllerARC1::CancelThisJob(Job ThisJob){};
  URL JobControllerARC1::GetFileUrlThisJob(Job ThisJob, std::string whichfile){};


} // namespace Arc
