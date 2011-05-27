#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>
#include <arc/StringConv.h>

#include "DTRList.h"

namespace DataStaging {
  
  bool DTRList::add_dtr(const DTR& DTRToAdd) {
  	DTR* dtr = new DTR(DTRToAdd);
  	Lock.lock();
  	DTRs.push_back(dtr);
  	Lock.unlock();
  	
  	// Added successfully
  	return true;
  }
  
  bool DTRList::delete_dtr(DTR* DTRToDelete) {
  	
  	Lock.lock();
  	DTRs.remove(DTRToDelete);
  	delete DTRToDelete;
  	Lock.unlock();
  	
  	// Deleted successfully
  	return true;
  }
  
  bool DTRList::filter_dtrs_by_owner(StagingProcesses OwnerToFilter, std::list<DTR*>& FilteredList){
  	std::list<DTR*>::iterator it;
     
    Lock.lock();
  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	  if((*it)->get_owner() == OwnerToFilter)
  	    FilteredList.push_back(*it);
    Lock.unlock();

  	// Filtered successfully
  	return true;
  }
  
  int DTRList::number_of_dtrs_by_owner(StagingProcesses OwnerToFilter){
  	std::list<DTR*>::iterator it;
    int counter = 0;
    
    Lock.lock();
  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	  if((*it)->get_owner() == OwnerToFilter)
  	    counter++;
    Lock.unlock();

  	// Filtered successfully
  	return counter;
  }
  
  bool DTRList::filter_dtrs_by_status(DTRStatus StatusToFilter, std::list<DTR*>& FilteredList){
  	std::list<DTR*>::iterator it;
  	
  	Lock.lock();
  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	  if((*it)->get_status() == StatusToFilter)
  	    FilteredList.push_back(*it);
  	Lock.unlock();
  	    
  	// Filtered successfully
  	return true;
  }
  
  bool DTRList::filter_dtrs_by_next_receiver(StagingProcesses NextReceiver, std::list<DTR*>& FilteredList) {
  	std::list<DTR*>::iterator it;
  	
  	switch(NextReceiver){
  	  case PRE_PROCESSOR: {
        Lock.lock();
        for(it = DTRs.begin();it != DTRs.end(); ++it)
  	      if((*it)->is_destined_for_pre_processor())
  	        FilteredList.push_back(*it);
  	    Lock.unlock();
  	    return true;  	  	
  	  }
  	  case POST_PROCESSOR: {
  	  	Lock.lock();
  	  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	      if((*it)->is_destined_for_post_processor())
  	        FilteredList.push_back(*it);
  	    Lock.unlock();
  	    return true;
  	  }
  	  case DELIVERY: {
  	  	Lock.lock();
  	  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	      if((*it)->is_destined_for_delivery())
  	        FilteredList.push_back(*it);
  	    Lock.unlock();
  	    return true;
  	  }
  	  default: // A strange receiver requested
  	    return false;
  	}
  }
  
  bool DTRList::filter_pending_dtrs(std::list<DTR*>& FilteredList){
  	std::list<DTR*>::iterator it;
  	Arc::Time now;
  	
  	Lock.lock(); 	
  	for(it = DTRs.begin();it != DTRs.end(); ++it){
  	  if( ((*it)->came_from_pre_processor() || (*it)->came_from_post_processor() ||
  	       (*it)->came_from_delivery() || (*it)->came_from_generator()) &&
  	      ((*it)->get_process_time() <= now) )
  	    FilteredList.push_back(*it);
  	}  	    
  	Lock.unlock();
  	
  	// Filtered successfully
  	return true;
  }
  
  bool DTRList::filter_dtrs_by_job(const std::string& jobid, std::list<DTR*>& FilteredList) {
    std::list<DTR*>::iterator it;

    Lock.lock();
    for(it = DTRs.begin();it != DTRs.end(); ++it)
      if((*it)->get_parent_job_id() == jobid)
        FilteredList.push_back(*it);
    Lock.unlock();

    // Filtered successfully
    return true;
  }

  std::list<DTR*> DTRList::all_dtrs() {
    std::list<DTR*> allDTRs;
    Lock.lock();
    allDTRs = DTRs;
    Lock.unlock();
    return allDTRs;
  }

  std::list<std::string> DTRList::all_jobs() {
    std::list<std::string> alljobs;
    std::list<DTR*>::iterator it;

    Lock.lock();
    for(it = DTRs.begin();it != DTRs.end(); ++it) {
      std::list<std::string>::iterator i = alljobs.begin();
      for (; i != alljobs.end(); ++i) {
        if (*i == (*it)->get_parent_job_id())
          break;
      }
      if (i == alljobs.end())
        alljobs.push_back((*it)->get_parent_job_id());
    }
    Lock.unlock();

    return alljobs;
  }

  void DTRList::dumpState(const std::string& path) {
    // only files supported for now - simply overwrite path
    std::string data;
    Lock.lock();
    for(std::list<DTR*>::iterator it = DTRs.begin();it != DTRs.end(); ++it) {
      data += (*it)->get_id() + " " +
              (*it)->get_status().str() + " " +
              Arc::tostring((*it)->get_priority()) + " " +
              (*it)->get_transfer_share() + "\n";
    }
    Lock.unlock();

    Arc::FileDelete(path);
    Arc::FileCreate(path, data);
  }

} // namespace DataStaging
