#include "SubmitterARC1.h"


namespace Arc {

  SubmitterARC1::SubmitterARC1(Config *cfg)
    : Submitter(cfg) {
    
  }

  SubmitterARC1::~SubmitterARC1() {
    
  }

  ACC *SubmitterARC1::Instance(Config *cfg, ChainContext *) {
    return new SubmitterARC1(cfg);
  }

  std::pair<URL, URL> SubmitterARC1::Submit(const std::string& jobdesc) {
	URL jobid;
	URL InfoEndpoint;
	
	//TODO: A-REX submission 
	
    return std::make_pair(jobid, InfoEndpoint);

  }

} // namespace Arc
