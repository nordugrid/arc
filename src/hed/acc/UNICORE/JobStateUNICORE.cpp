#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include "JobStateUNICORE.h"


namespace Arc {

  JobState::StateType JobStateUNICORE::StateMap(const std::string& state) {
    if (Arc::lower(state) == "accepted")
      return JobState::ACCEPTED;
    else if (Arc::lower(state) == "queued")
      return JobState::QUEUING;
    else if (Arc::lower(state) == "running")
      return JobState::RUNNING;
    else if (Arc::lower(state) == "finished")
      return JobState::FINISHED;
    else if (Arc::lower(state) == "failed")
      return JobState::FAILED;
    else if (state == "")
      return JobState::UNDEFINED;
    else
      return JobState::OTHER;
  }

}

/*
   113      <xsd:enumeration value="UNDEFINED"/>
   114      <xsd:enumeration value="READY"/>
   115      <xsd:enumeration value="QUEUED"/>
   116      <xsd:enumeration value="RUNNING"/>
   117      <xsd:enumeration value="SUCCESSFUL"/>
   118      <xsd:enumeration value="FAILED"/>
   119      <xsd:enumeration value="STAGINGIN"/>
   120      <xsd:enumeration value="STAGINGOUT"/>

     UNICORE shows the following job states:

 * STAGINGIN - the server is staging in data from remote sites into the job directory
 * READY - job is ready to be started
 * QUEUED - job is waiting in the batch queue
 * RUNNING - job is running
 * STAGINGOUT - execution has finished, and the server is staging out data to remote sites
 * SUCCESSFUL - all finished, no errors occured
 * FAILED - errors occured in the execution and/or data staging phases
 * UNDEFINED - this state formally exists, but is not seen on clients
 */
