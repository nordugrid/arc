#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include "../files/info_types.h"
#include "../files/info_files.h"
#include "../run/run_parallel.h"
#include "../conf/GMConfig.h"
#include "send_mail.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

/* check if have to send mail and initiate sending */
bool send_mail(const GMJob &job,const GMConfig& config) {
  char flag = states_all[job.get_state()].mail_flag;
  if(flag == ' ') return true;
  std::string notify = "";
  std::string jobname = "";
  JobLocalDescription *job_desc = job.get_local();
  if(job_desc == NULL) {
    job_desc = new JobLocalDescription;
    if(!job_local_read_file(job.get_id(),config,*job_desc)) {
      logger.msg(Arc::ERROR,"Failed reading local information");
      delete job_desc; job_desc=NULL;
    };
  };
  if(job_desc != NULL) {
    jobname=job_desc->jobname;
    notify=job_desc->notify;
    if(job.get_local() == NULL) { delete job_desc; };
  };
//  job_local_read_notify(job.get_id(),user,notify);
  if(notify.length() == 0) return true; /* save some time */
  Arc::Run* child = NULL;
  std::string failure_reason=job.GetFailure(config);
  if(job_failed_mark_check(job.get_id(),config)) {
    if(failure_reason.length() == 0) failure_reason="<unknown>";
  };
  for(std::string::size_type n=0;;) {
    n=failure_reason.find('\n',n);
    if(n == std::string::npos) break;
    failure_reason[n]='.';
  };
  std::string cmd(Arc::ArcLocation::GetToolsDir()+"/smtp-send.sh");
  std::string from_addr = config.SupportMailAddress();
  char* args[11] ={ /* max 3 mail addresses */
       (char*)cmd.c_str(),
       (char*)states_all[job.get_state()].name,
       (char*)job.get_id().c_str(),
       (char*)config.ControlDir().c_str(),
       (char*)from_addr.c_str(),
       (char*)jobname.c_str(),
       (char*)failure_reason.c_str(),
       NULL,
       NULL,
       NULL,
       NULL
  };
  /* go through mail addresses and flags */
  std::string::size_type pos=0;
  std::string::size_type pos_s=0;
  std::string mails[3];
  int mail_n=0;
  bool right_flag = false;
  /* by default mail is sent when job enters states PREPARING and FINISHED */
  if((flag == 'b') || (flag == 'e')) right_flag=true;
  for(;;) {
    if(pos_s >= notify.length()) break;
    if((pos = notify.find(' ',pos_s)) == std::string::npos) pos=notify.length();
    if(pos==pos_s) { pos++; pos_s++; continue; };
    std::string word(notify.substr(pos_s,pos-pos_s));
    if(word.find('@') == std::string::npos) { /* flags */
      if(word.find(flag) == std::string::npos) { right_flag=false; }
      else { right_flag=true; };
      pos_s=pos+1;
      continue;
    };
    if(right_flag) { mails[mail_n]=word; mail_n++; };
    if(mail_n >= 3) break;
    pos_s=pos+1;
  };
  if(mail_n == 0) return true; /* not sending to anyone */
  for(mail_n--;mail_n>=0;mail_n--) {
    args[7+mail_n]=(char*)(mails[mail_n].c_str());
  };
  if(!RunParallel::run(config,job,args,&child)) {
    logger.msg(Arc::ERROR,"Failed running mailer");
    return false;
  };
  child->Abandon();
  delete child;
  return true;
}

} // namespace ARex
