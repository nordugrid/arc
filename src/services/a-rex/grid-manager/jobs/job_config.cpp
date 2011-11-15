#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_config.h"

JobsListConfig::JobsListConfig(void) {
  for(int n = 0;n<JOB_STATE_NUM;n++) jobs_num[n]=0;
  jobs_pending = 0;
  max_jobs_processing=DEFAULT_MAX_LOAD;
  max_jobs_processing_emergency=1;
  max_jobs_running=-1;
  max_jobs_total=-1;
  max_jobs=-1;
  max_jobs_per_dn=-1;
  max_downloads=-1;
  max_processing_share = 0;
  //limited_share;
  share_type = "";
  min_speed=0;
  min_speed_time=300;
  min_average_speed=0;
  max_inactivity_time=300;
  max_retries=DEFAULT_MAX_RETRIES;
  use_secure_transfer=false; /* secure data transfer is OFF by default !!! */
  use_passive_transfer=false;
  use_local_transfer=false;
  use_new_data_staging=false;
  wakeup_period = 120; // default wakeup every 2 mins
}

