// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMISSIONSTATUS_H__
#define __ARC_SUBMISSIONSTATUS_H__

namespace Arc {
  
  class SubmissionStatus {
  public:
    enum SubmissionStatusType {
      NONE = 0,
      NO_SERVICES = 1 << 0,
      ENDPOINT_NOT_QUERIED = 1 << 1,
      BROKER_PLUGIN_NOT_LOADED = 1 << 2,
      DESCRIPTION_NOT_SUBMITTED = 1 << 3,
      SUBMITTER_PLUGIN_NOT_LOADED = 1 << 4, 
      ERROR_FROM_ENDPOINT = 1 << 5
    };

    SubmissionStatus() : status(NONE) {}
    SubmissionStatus(const SubmissionStatus& s) : status(s.status) {}
    SubmissionStatus(SubmissionStatusType s) : status(s) {}
    SubmissionStatus(unsigned int s) : status(s & maxValue) {}
  
    SubmissionStatus& operator|=(SubmissionStatusType s) { status |= s; return *this; }
    SubmissionStatus& operator|=(const SubmissionStatus& s) { status |= s.status; return *this; }
    SubmissionStatus& operator|=(unsigned int s) { status |= (s & maxValue); return *this; }

    SubmissionStatus operator|(SubmissionStatusType s) const { return (status | s); }
    SubmissionStatus operator|(const SubmissionStatus& s) const { return (status | s.status); }
    SubmissionStatus operator|(unsigned int s) const { return (status | (s & maxValue)); }
    
    SubmissionStatus& operator&=(SubmissionStatusType s) { status &= s; return *this; }
    SubmissionStatus& operator&=(const SubmissionStatus& s) { status &= s.status; return *this; }
    SubmissionStatus& operator&=(unsigned int s) { status &= s; return *this; }

    SubmissionStatus operator&(SubmissionStatusType s) const { return (status & s); }
    SubmissionStatus operator&(const SubmissionStatus& s) const { return (status & s.status); }
    SubmissionStatus operator&(unsigned int s) const { return (status & s); }

    SubmissionStatus& operator=(SubmissionStatusType s) { status = s; return *this; }
    SubmissionStatus& operator=(unsigned int s) { status = (s & maxValue); return *this; }

    operator bool() const { return status == NONE; }
    
    bool operator==(const SubmissionStatus& s) const { return status == s.status; }
    bool operator==(SubmissionStatusType s) const { return status == (unsigned)s; }
    bool operator==(unsigned int s) const { return status == s; }

    bool operator!=(const SubmissionStatus& s) const { return !operator==(s); }
    bool operator!=(SubmissionStatusType s) const { return !operator==(s); }
    bool operator!=(unsigned int s) const { return !operator==(s); }
    
    bool isSet(SubmissionStatusType s) const { return (s & status) == (unsigned)s; }
    
  private:
    static const unsigned int maxValue = (1 << 6) - 1;
    unsigned int status;
  };

}

#endif // __ARC_SUBMISSIONSTATUS_H__
