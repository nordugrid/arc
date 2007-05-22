// MCCPlexer.cpp

#include "MCCPlexer.h"

namespace Arc {
  
  MCC_Plexer::MCC_Plexer(Config *cfg) : MCC(cfg) {
  }

  MCC_Plexer::~MCC_Plexer(){
  }

  void MCC_Plexer::Next(MCCInterface* next, const std::string& label){
    if (next==0)
      services_.erase(label);
    else
      services_[label]=next;
  }
  
  MCC_Status MCC_Plexer::process(Message& request, Message& response){

    // TODO: Decide the name and the content of the attribute that
    // determines the destination service of a message. Is it a full
    // URI or only the "path part" of an URI? In the latter case we
    // should find a more appropriate name than "Request-URI". The
    // code below should then be adjusted accordingly.

    std::string requestURI =
      request.Attributes()->get("Request-URI");
    MCCInterface* destination = services_[requestURI];
    if (destination!=0)
      return destination->process(request, response);
    else
      // Requested service not found!
      // TODO: Adapt to error representation when it has been decided.
      return MCC_Status(404);
  }
  
}
