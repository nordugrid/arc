// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/client/Job.h>

namespace Arc {

  Logger Job::logger(Logger::getRootLogger(), "Job");

  Job::Job()
    : ExitCode(-1),
      WaitingPosition(-1),
      RequestedTotalWallTime(-1),
      RequestedTotalCPUTime(-1),
      RequestedMainMemory(-1),
      RequestedSlots(-1),
      UsedTotalWallTime(-1),
      UsedTotalCPUTime(-1),
      UsedMainMemory(-1),
      UsedSlots(-1),
      SubmissionTime(-1),
      ComputingManagerSubmissionTime(-1),
      StartTime(-1),
      ComputingManagerEndTime(-1),
      EndTime(-1),
      WorkingAreaEraseTime(-1),
      ProxyExpirationTime(-1),
      CreationTime(-1),
      Validity(-1),
      VirtualMachine(false) {}

  Job::~Job() {}

  void Job::Print(bool longlist) const {
    logger.msg(WARNING, "The Job::Print method is DEPRECATED, use the Job::SaveToStream method instead.");
    SaveToStream(std::cout, longlist);
  }

  void Job::SaveToStream(std::ostream& out, bool longlist) const {

    out << IString("Job: %s", JobID.str()) << std::endl;
    if (!Name.empty())
      out << IString(" Name: %s", Name) << std::endl;
    if (!State().empty())
      out << IString(" State: %s (%s)", State.GetGeneralState(), State())
                << std::endl;
    if (ExitCode != -1)
      out << IString(" Exit Code: %d", ExitCode) << std::endl;
    if (!Error.empty()) {
      std::list<std::string>::const_iterator iter;
      for (iter = Error.begin(); iter != Error.end(); iter++)
        out << IString(" Error: %s", *iter) << std::endl;
    }
    if (longlist) {
      if (!Owner.empty())
        out << IString(" Owner: %s", Owner) << std::endl;
      if (!OtherMessages.empty())
        for (std::list<std::string>::const_iterator iter = OtherMessages.begin();
             iter != OtherMessages.end(); iter++)
          out << IString(" Other Messages: %s", *iter)
                    << std::endl;
      if (!ExecutionCE.empty())
        out << IString(" ExecutionCE: %s", ExecutionCE)
                  << std::endl;
      if (!Queue.empty())
        out << IString(" Queue: %s", Queue) << std::endl;
      if (UsedSlots != -1)
        out << IString(" Used Slots: %d", UsedSlots) << std::endl;
      if (WaitingPosition != -1)
        out << IString(" Waiting Position: %d", WaitingPosition)
                  << std::endl;
      if (!StdIn.empty())
        out << IString(" Stdin: %s", StdIn) << std::endl;
      if (!StdOut.empty())
        out << IString(" Stdout: %s", StdOut) << std::endl;
      if (!StdErr.empty())
        out << IString(" Stderr: %s", StdErr) << std::endl;
      if (!LogDir.empty())
        out << IString(" Grid Manager Log Directory: %s", LogDir)
                  << std::endl;
      if (SubmissionTime != -1)
        out << IString(" Submitted: %s",
                             (std::string)SubmissionTime) << std::endl;
      if (EndTime != -1)
        out << IString(" End Time: %s", (std::string)EndTime)
                  << std::endl;
      if (!SubmissionHost.empty())
        out << IString(" Submitted from: %s", SubmissionHost)
                  << std::endl;
      if (!SubmissionClientName.empty())
        out << IString(" Submitting client: %s",
                             SubmissionClientName) << std::endl;
      if (RequestedTotalCPUTime != -1)
        out << IString(" Requested CPU Time: %s",
                             RequestedTotalCPUTime.istr())
                  << std::endl;
      if (UsedTotalCPUTime != -1)
        out << IString(" Used CPU Time: %s",
                             UsedTotalCPUTime.istr()) << std::endl;
      if (UsedTotalWallTime != -1)
        out << IString(" Used Wall Time: %s",
                             UsedTotalWallTime.istr()) << std::endl;
      if (UsedMainMemory != -1)
        out << IString(" Used Memory: %d", UsedMainMemory)
                  << std::endl;
      if (WorkingAreaEraseTime != -1)
        out << IString((State == JobState::DELETED) ?
                             istring(" Results were deleted: %s") :
                             istring(" Results must be retrieved before: %s"),
                             (std::string)WorkingAreaEraseTime)
                  << std::endl;
      if (ProxyExpirationTime != -1)
        out << IString(" Proxy valid until: %s",
                             (std::string)ProxyExpirationTime)
                  << std::endl;
      if (CreationTime != -1)
        out << IString(" Entry valid from: %s",
                             (std::string)CreationTime) << std::endl;
      if (Validity != -1)
        out << IString(" Entry valid for: %s",
                             Validity.istr()) << std::endl;
    }

    out << std::endl;

  } // end Print

  bool Job::CompareJobID(const Job* a, const Job* b) {
    return a->JobID.fullstr().compare(b->JobID.fullstr()) < 0;
  }

  bool Job::CompareSubmissionTime(const Job* a, const Job* b) {
    return a->SubmissionTime < b->SubmissionTime;
  }

  bool Job::CompareJobName(const Job* a, const Job* b) {
    return a->Name.compare(b->Name) < 0;
  }
} // namespace Arc
