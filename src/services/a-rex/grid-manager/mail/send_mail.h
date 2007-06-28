/*
  Starts external process smtp-send.sh to send mail to user
  about changes in job's status.
*/
bool send_mail(const JobDescription &desc,JobUser &user);
