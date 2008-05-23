#include "JobControllerARC0.h"
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>

#include <iostream>
#include <algorithm>

namespace Arc {

  JobControllerARC0::JobControllerARC0(Arc::Config *cfg) : JobController(cfg, "ARC0") {}
  
  JobControllerARC0::~JobControllerARC0() {}

  ACC *JobControllerARC0::Instance(Config *cfg, ChainContext *) {
    return new JobControllerARC0(cfg);
  }
  
  void JobControllerARC0::GetJobInformation(){

    //Here implement reading information from clusters and query clusters in parallel    

  }

  void JobControllerARC0::PerformAction(std::string action){

  }



} // namespace Arc
