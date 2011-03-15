#ifndef __ARC_SUBMITTERTESTACCCONTROL_H__
#define __ARC_SUBMITTERTESTACCCONTROL_H__

#include <arc/client/Job.h>

class SubmitterTestACCControl {
  public:
    static bool submitStatus;
    static bool migrateStatus;
    static bool modifyStatus;
    static Arc::Job* submitJob;
    static Arc::Job* migrateJob;
};

bool SubmitterTestACCControl::submitStatus = false;
bool SubmitterTestACCControl::migrateStatus = false;
bool SubmitterTestACCControl::modifyStatus = false;
Arc::Job* SubmitterTestACCControl::submitJob = NULL;
Arc::Job* SubmitterTestACCControl::migrateJob = NULL;

#endif // __ARC_SUBMITTERTESTACCCONTROL_H__
