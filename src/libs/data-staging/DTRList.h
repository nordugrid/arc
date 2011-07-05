#ifndef DTRLIST_H_
#define DTRLIST_H_

#include <arc/Thread.h>

#include "DTR.h"

namespace DataStaging {
  
  /// Global list of all active DTRs in the system.
  /** This class contains several methods for filtering the list by owner, state etc */
  class DTRList {
  	
  	private:
  	  
      /// Internal list of DTRs
  	  std::list<DTR*> DTRs;
  
  	  /// Lock to protect list during modification
      Arc::SimpleCondition Lock;
  	  
  	public:
          	
  	  /// Put a new DTR into the list.
      /** A (pointer to a) copy of the DTR is added to the list, and so
       * DTRToAdd can be deleted after this method is called. */
  	  bool add_dtr(const DTR& DTRToAdd);
  	  
  	  /// Remove a DTR from the list.
  	  /** The DTRToDelete object is destroyed, and hence should not be
  	   * used after calling this method. */
  	  bool delete_dtr(DTR* DTRToDelete);
  	  
  	  /// Filter the queue to select DTRs owned by a specified process.
  	  /// @param FilteredList This list is filled with filtered DTRs
  	  bool filter_dtrs_by_owner(StagingProcesses OwnerToFilter, std::list<DTR*>& FilteredList);

  	  /// Returns the number of DTRs owned by a particular process
  	  int number_of_dtrs_by_owner(StagingProcesses OwnerToFilter);

      /// Filter the queue to select DTRs with particular status.
      /** If we have only one common queue for all DTRs, this method is
       * necessary to make virtual queues for the DTRs about to go into the
       * pre-, post-processor or delivery stages.
       * @param FilteredList This list is filled with filtered DTRs
       */
      bool filter_dtrs_by_status(DTRStatus StatusToFilter, std::list<DTR*>& FilteredList);
      
      /// Select DTRs that are about to go to the specified process.
      /** This selection is actually a virtual queue for pre-, post-processor
       * and delivery.
       * @param FilteredList This list is filled with filtered DTRs
       */
      bool filter_dtrs_by_next_receiver(StagingProcesses NextReceiver, std::list<DTR*>& FilteredList);
      
      /// Select DTRs that have just arrived from pre-, post-processor, delivery or generator.
      /** These DTRs need some reaction from the scheduler. This selection is
       * actually a virtual queue of DTRs that need to be processed.
       * @param FilteredList This list is filled with filtered DTRs
       */
      bool filter_pending_dtrs(std::list<DTR*>& FilteredList);

      /// Get the list of DTRs corresponding to the given job ID.
      /// @param FilteredList This list is filled with filtered DTRs
      bool filter_dtrs_by_job(const std::string& jobid, std::list<DTR*>& FilteredList);
      
      /// Get the list of all DTRs
      std::list<DTR*> all_dtrs();

      /// Get the list of all job IDs
      std::list<std::string> all_jobs();

      /// Dump state of all current DTRs to a destination, eg file, database, url...
      /** Currently only file is supported.
       * @param path Path to the file in which to dump state.
       */
      void dumpState(const std::string& path);

  };
	
} // namespace DataStaging

#endif /*DTRLIST_H_*/
