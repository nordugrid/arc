#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>
#include <arc/StringConv.h>

#include "DTRList.h"

namespace DataStaging {
  
  bool DTRList::add_dtr(DTR_ptr DTRToAdd) {
  	Lock.lock();
  	DTRs.push_back(DTRToAdd);
  	Lock.unlock();
  	
  	// Added successfully
  	return true;
  }
  
  bool DTRList::delete_dtr(DTR_ptr DTRToDelete) {
  	
  	Lock.lock();
  	DTRs.remove(DTRToDelete);
  	Lock.unlock();
  	
  	// Deleted successfully
  	return true;
  }
  
  bool DTRList::filter_dtrs_by_owner(StagingProcesses OwnerToFilter, std::list<DTR_ptr>& FilteredList){
  	std::list<DTR_ptr>::iterator it;
     
    Lock.lock();
  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	  if((*it)->get_owner() == OwnerToFilter)
  	    FilteredList.push_back(*it);
    Lock.unlock();

  	// Filtered successfully
  	return true;
  }
  
  int DTRList::number_of_dtrs_by_owner(StagingProcesses OwnerToFilter){
  	std::list<DTR_ptr>::iterator it;
    int counter = 0;
    
    Lock.lock();
  	for(it = DTRs.begin();it != DTRs.end(); ++it)
  	  if((*it)->get_owner() == OwnerToFilter)
  	    counter++;
    Lock.unlock();

  	// Filtered successfully
  	return counter;
  }
  
  bool DTRList::filter_dtrs_by_status(DTRStatus::DTRStatusType StatusToFilter, std::list<DTR_ptr>& FilteredList){
    std::vector<DTRStatus::DTRStatusType> StatusesToFilter(1, StatusToFilter);
    return filter_dtrs_by_statuses(StatusesToFilter, FilteredList);
  }

  bool DTRList::filter_dtrs_by_statuses(const std::vector<DTRStatus::DTRStatusType>& StatusesToFilter,
                                        std::list<DTR_ptr>& FilteredList){
    std::list<DTR_ptr>::iterator it;

    Lock.lock();
    for(it = DTRs.begin();it != DTRs.end(); ++it) {
      for (std::vector<DTRStatus::DTRStatusType>::const_iterator i = StatusesToFilter.begin(); i != StatusesToFilter.end(); ++i) {
        if((*it)->get_status().GetStatus() == *i) {
          FilteredList.push_back(*it);
          break;
        }
      }
    }
    Lock.unlock();

    // Filtered successfully
    return true;
  }

  bool DTRList::filter_dtrs_by_statuses(const std::vector<DTRStatus::DTRStatusType>& StatusesToFilter,
                                        std::map<DTRStatus::DTRStatusType, std::list<DTR_ptr> >& FilteredList) {
    std::list<DTR_ptr>::iterator it;

    Lock.lock();
    for(it = DTRs.begin();it != DTRs.end(); ++it) {
      for (std::vector<DTRStatus::DTRStatusType>::const_iterator i = StatusesToFilter.begin(); i != StatusesToFilter.end(); ++i) {
        if((*it)->get_status().GetStatus() == *i) {
          FilteredList[*i].push_back(*it);
          break;
        }
      }
    }
    Lock.unlock();

    // Filtered successfully
    return true;
  }

  bool DTRList::filter_dtrs_by_next_receiver(StagingProcesses NextReceiver, std::list<DTR_ptr>& FilteredList) {
  	std::list<DTR_ptr>::iterator it;
  	
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
  
  bool DTRList::filter_pending_dtrs(std::list<DTR_ptr>& FilteredList){
  	std::list<DTR_ptr>::iterator it;
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
  
  bool DTRList::filter_dtrs_by_job(const std::string& jobid, std::list<DTR_ptr>& FilteredList) {
    std::list<DTR_ptr>::iterator it;

    Lock.lock();
    for(it = DTRs.begin();it != DTRs.end(); ++it)
      if((*it)->get_parent_job_id() == jobid)
        FilteredList.push_back(*it);
    Lock.unlock();

    // Filtered successfully
    return true;
  }

  void DTRList::caching_started(DTR_ptr request) {
    CachingLock.lock();
    CachingSources[request->get_source_str()] = request->get_priority();
    CachingLock.unlock();
  }

  void DTRList::caching_finished(DTR_ptr request) {
    CachingLock.lock();
    CachingSources.erase(request->get_source_str());
    CachingLock.unlock();
  }

  bool DTRList::is_being_cached(DTR_ptr DTRToCheck) {

    CachingLock.lock();
    std::map<std::string, int>::iterator i = CachingSources.find(DTRToCheck->get_source_str());
    bool caching = (i != CachingSources.end());
    // If already caching, find the DTR and increase its priority if necessary
    if (caching && i->second < DTRToCheck->get_priority()) {
      Lock.lock();
      for(std::list<DTR_ptr>::iterator it = DTRs.begin();it != DTRs.end(); ++it) {
        if ((*it)->get_source_str() == DTRToCheck->get_source_str() &&
            (*it)->is_destined_for_delivery()) {
          (*it)->get_logger()->msg(Arc::INFO, "Boosting priority from %i to %i due to incoming higher priority DTR",
                                   (*it)->get_priority(), DTRToCheck->get_priority());
          (*it)->set_priority(DTRToCheck->get_priority());
          CachingSources[DTRToCheck->get_source_str()] = DTRToCheck->get_priority();
        }
      }
      Lock.unlock();
    }
    CachingLock.unlock();
    return caching;
  }

  bool DTRList::empty() {
    Lock.lock();
    bool empty = DTRs.empty();
    Lock.unlock();
    return empty;
  }

  unsigned int DTRList::size() {
    Lock.lock();
    unsigned int size = DTRs.size();
    Lock.unlock();
    return size;
  }

  std::list<std::string> DTRList::all_jobs() {
    std::list<std::string> alljobs;
    std::list<DTR_ptr>::iterator it;

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
    for(std::list<DTR_ptr>::iterator it = DTRs.begin();it != DTRs.end(); ++it) {
      data += (*it)->get_id() + " " +
              (*it)->get_status().str() + " " +
              Arc::tostring((*it)->get_priority()) + " " +
              (*it)->get_transfer_share();
      // add destination for recovery after crash
      if ((*it)->get_status() == DTRStatus::TRANSFERRING || (*it)->get_status() == DTRStatus::TRANSFER) {
        data += " " + (*it)->get_destination()->CurrentLocation().fullstr();
        data += " " + (*it)->get_delivery_endpoint().Host();
      }
      data += "\n";
    }
    Lock.unlock();

    Arc::FileCreate(path, data);
  }

} // namespace DataStaging
