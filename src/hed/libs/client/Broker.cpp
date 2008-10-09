#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Broker.h>
#include <list>

namespace Arc {
    
    Arc::Logger logger(Arc::Logger::getRootLogger(), "broker");
    Arc::LogStream logcerr(std::cerr);
    //Arc::Logger::getRootLogger().addDestination(logcerr);
    //Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
 	    
    Broker::Broker( Arc::TargetGenerator& targen,  Arc::JobDescription jobd) {
	//for get_Targets test
              ExecutionTarget eTarget;
              eTarget.MaxCPUTime = 1; 
              eTarget.RunningJobs = 1; 
              eTarget.MaxRunningJobs = 1; 
              found_Targets.push_back(eTarget);

              eTarget.MinCPUTime = 1; 
              eTarget.MaxRunningJobs = 22; 
              found_Targets.push_back(eTarget);

              ExecutionTarget eTarget2;
              eTarget2.DefaultCPUTime = 1; 
              eTarget2.RunningJobs = 3; 
              eTarget2.MaxRunningJobs = 31; 
              found_Targets.push_back(eTarget2);
	//end of "for get_Targets test"
 		      
              targen.GetTargets(0, 1);
 	
              if (targen.FoundTargets().empty()) {
 	    logger.msg(Arc::ERROR, "No Targets found!");
	    //TODO: removed this and next comments
 	    //throw "No Targets found!";
             }
             for (std::list<Arc::ExecutionTarget>::const_iterator target =
 	           targen.FoundTargets().begin();
 	         target != targen.FoundTargets().end(); target++) {  
             //Pre filtering
	
		     //if (the Target is good ) {
	                           //found_Targets.push_back( target );	     
		     //}		     
            // end of Pre filtering		     
             }	
		 

             current =  found_Targets.begin();    
   }

   Broker::~Broker() {}
    

   ExecutionTarget& Broker::get_Target() {
            //If first time call this method, then must be sort the Targets list.
            if ( current == found_Targets.begin()){
                   sort_Targets();
            }       
       
            std::vector<Arc::ExecutionTarget>::iterator ret_pointer;
            ret_pointer = current;
            current++;  
            if ( ret_pointer==  found_Targets.end()){
                    throw "No more ExecutionTarget!";  
            }
            return *ret_pointer;
  }
  

} // namespace Arc
