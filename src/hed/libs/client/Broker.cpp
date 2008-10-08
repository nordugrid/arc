#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <arc/client/Broker.h>
#include "Broker.h"

namespace Arc {
    
    Broker::Broker( Arc::JobDescription jd) {
              ExecutionTarget eTarget;
              eTarget.MaxCPUTime = 1; 
              found_Targets.push_back(eTarget);

              eTarget.MinCPUTime = 1; 
              found_Targets.push_back(eTarget);

              ExecutionTarget eTarget2;
              eTarget2.DefaultCPUTime = 1; 
              found_Targets.push_back(eTarget2);
              //Pre filtering
      
             // end of Pre filtering
             current =  found_Targets.begin();    
   }

   Broker::~Broker() {}
    

   ExecutionTarget& Broker::get_Target() {
            std::list<Arc::ExecutionTarget>::iterator ret_pointer;
            ret_pointer = current;
            current++;  
            if ( ret_pointer==  found_Targets.end()){
                    throw "No more ExecutionTarget!";  
            }
            return *ret_pointer;
  }
  

} // namespace Arc
