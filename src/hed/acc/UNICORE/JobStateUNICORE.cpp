#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobStateUNICORE.h"

namespace Arc {

JobState::StateType JobStateUNICORE::StateMap(const std::string& state)
{
  if (state == "ACCEPTED") return JobState::ACCEPTED;
  else if (state == "QUEUED") return JobState::QUEUING;
  else if (state == "RUNNING") return JobState::RUNNING;
  else if (state == "SUCCESSFUL") return JobState::FINISHED;
  else if (state == "FAILED") return JobState::FAILED;
  else return JobState::UNDEFINED;
};

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
*/
