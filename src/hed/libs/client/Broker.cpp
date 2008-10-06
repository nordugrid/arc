#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <arc/client/Broker.h>
#include "Broker.h"
namespace Arc {
  Broker::Broker() {}
  Broker::Broker( Arc::JobDescription jd) {
  
              //Pre filtering
      
             // end of Pre filtering
             //Broker::sort_Targets();
  }
  
  Broker::~Broker() {}
  
  ExecutionTarget Broker::get_Target() {}
  
  

} // namespace Arc
