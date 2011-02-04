#ifndef DTRLIST_H_
#define DTRLIST_H_

#include <arc/Thread.h>

#include "DTR.h"

namespace DataStaging {
  
  class DTRList {
  	
  	private:
  	  
  	  std::list<DTR*> DTRs;
  
      Arc::SimpleCondition Lock;
  	  
  	public:
          	
  	  //Default constructor and destructor
  	  DTRList(){};
  	  ~DTRList(){};
  	  
  	  // Put a new DTR into the list
  	  bool add_dtr(const DTR& DTRToAdd);
  	  
  	  // Remove a DTR from the list
  	  bool delete_dtr(DTR* DTRToDelete);
  	  
  	  // Filter the queue to select DTRs owned by a
  	  // specified process
  	  // The list is passed by reference to be populated.
  	  bool filter_dtrs_by_owner(StagingProcesses OwnerToFilter, std::list<DTR*>& FilteredList);
  	  // Complementary to above -- return just the number of DTRs
  	  // owned by a particular process
  	  int number_of_dtrs_by_owner(StagingProcesses OwnerToFilter);

      // Filter the queue to select DTRs with particulare status(es).
      // If we have only one common queue, this method is necessary
      // to make virtual queues for the jobs about to go into the
      // pre-, post-processor or delivery
      bool filter_dtrs_by_status(DTRStatus StatusToFilter, std::list<DTR*>& FilteredList);
      
      // Select DTRs that are about to go to pre-, post-processor
      // or delivery. This selection is actually a virtual queue 
      // for pre-, post-processor and delivery.
      bool filter_dtrs_by_next_receiver(StagingProcesses NextReceiver, std::list<DTR*>& FilteredList);
      
      // Select DTRs that have just arrived from pre-, post-processor,
      // delivery or generator and thus need some reaction from
      // the scheduler. This selection is actually a virtual queue 
      // of DTRs that need to be processed -- a replacement of Events
      // list we had before in the implementation.
      bool filter_pending_dtrs(std::list<DTR*>& FilteredList);

      // Get the list of DTRs corresponding to the given job description
      bool filter_dtrs_by_job(const std::string& jobid, std::list<DTR*>& FilteredList);
      
      // Get the list of all DTRs
      std::list<DTR*> all_dtrs();

      // Get the list of all job IDs
      std::list<std::string> all_jobs();

      // Dump state of all current DTRs to a destination, eg file, database, url...
      void dumpState(const std::string& path);

  };
	
} // namespace DataStaging

#endif /*DTRLIST_H_*/
