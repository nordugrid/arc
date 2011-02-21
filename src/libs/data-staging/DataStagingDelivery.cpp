#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string.h>
#include <unistd.h>

#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBuffer.h>

#include "DataDeliveryComm.h"

using namespace Arc;


static void ReportStatus(DataStaging::DTRStatus::DTRStatusType st,
                         DataStaging::DTRErrorStatus::DTRErrorStatusType err,
                         const std::string& err_desc,
                         unsigned long long int transfered,
                         unsigned long long int size) {
  static DataStaging::DataDeliveryComm::Status status;
  static DataStaging::DataDeliveryComm::Status status_;
  static unsigned int status_pos = 0;
  static bool status_changed = true;

  // Filling
  status.commstatus = DataStaging::DataDeliveryComm::CommNoError;
  status.timestamp = ::time(NULL);
  status.status = st;
  status.error = err;
  strncpy(status.error_desc,err_desc.c_str(),sizeof(status.error_desc));
  status.streams = 0;
  status.transfered = transfered;
  status.size = size;
  status.offset = 0;
  status.speed = 0;
  if(status_pos == 0) {
    status_=status;
    status_changed=true;
  };
  if(status_changed) {
    for(;;) {
      ssize_t l = ::write(STDOUT_FILENO,((char*)&status)+status_pos,sizeof(status)-status_pos);
      if(l == -1) { // error, parent exited?
        break;
      } else if(l == 0) { // will happen if stdout is non-blocking
        break;
      } else {
        status_pos+=l;
      };
      if(status_pos >= sizeof(status)) {
        status_pos=0;
        status_changed=false;
        break;
      };
    };
  };
}

static unsigned long long int GetFileSize(const DataPoint& source, const DataPoint& dest) {
  if(source.CheckSize()) return source.GetSize();
  if(dest.CheckSize()) return dest.GetSize();
  return 0;
}

int main(int argc,char* argv[]) {
  // Collecting parameters
  // --surl: source URL 
  // --durl: destination URL
  // --sopt: any URL option, credential - path to file storing credentials
  // --dopt: any URL option, credential - path to file storing credentials
  // --topt: minspeed, minspeedtime, minavgspeed, maxinacttime, avgtime
  // surl and durl may be given only once
  // sopt, dopt, topt may be given multiple times
  // type of credentials is detected automatically, so far only 
  // X.509 proxies or key+certificate are accepted
  std::string source_str;
  std::string dest_str;
  std::list<std::string> source_opts;
  std::list<std::string> dest_opts;
  std::list<std::string> transfer_opts;
  std::string source_cred_path;
  std::string dest_cred_path;
  std::string source_ca_path;
  std::string dest_ca_path;
  OptionParser opt;
  opt.AddOption(0,"surl","","source URL",source_str);
  opt.AddOption(0,"durl","","destination URL",dest_str);
  opt.AddOption(0,"sopt","","source options",source_opts);
  opt.AddOption(0,"dopt","","destination options",dest_opts);
  opt.AddOption(0,"topt","","transfer options",transfer_opts);
  if(opt.Parse(argc,argv).size() != 0) {
    std::cerr<<"Unexpected arguments"<<std::endl; return -1;
  };
  if(source_str.empty()) {
    std::cerr<<"Source URL missing"<<std::endl; return -1;
  };
  if(dest_str.empty()) {
    std::cerr<<"Destination URL missing"<<std::endl; return -1;
  };
  URL source_url(source_str);
  if(!source_url) {
    std::cerr<<"Source URL not valid: "<<source_str<<std::endl; return -1;
  };
  URL dest_url(dest_str);
  if(!dest_url) {
    std::cerr<<"Destination URL not valid: "<<dest_str<<std::endl; return -1;
  };
  for(std::list<std::string>::iterator o = source_opts.begin();
                           o != source_opts.end();++o) {
    std::string::size_type p = o->find('=');
    if(p == std::string::npos) {
      source_url.AddOption(*o,"");
    } else {
      std::string name = o->substr(0,p);
      if(name == "credential") {
        source_cred_path = o->substr(p+1);
      } else if(name == "ca") {
        source_ca_path = o->substr(p+1);
      } else {
        source_url.AddOption(name,o->substr(p+1));
      };
    };
  };
  for(std::list<std::string>::iterator o = dest_opts.begin();
                           o != dest_opts.end();++o) {
    std::string::size_type p = o->find('=');
    if(p == std::string::npos) {
      dest_url.AddOption(*o,"");
    } else {
      std::string name = o->substr(0,p);
      if(name == "credential") {
        dest_cred_path = o->substr(p+1);
      } else if(name == "ca") {
        dest_ca_path = o->substr(p+1);
      } else {
        dest_url.AddOption(name,o->substr(p+1));
      };
    };
  };

  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE); //TODO: configurable
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::EmptyFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);

  DataBuffer buffer;
  buffer.speed.verbose(true);
  unsigned long long int minspeed = 0;
  time_t minspeedtime = 0;
  for(std::list<std::string>::iterator o = transfer_opts.begin();
                           o != transfer_opts.end();++o) {
    std::string::size_type p = o->find('=');
    std::string value;
    if(p != std::string::npos) {
      std::string name = o->substr(0,p);
      unsigned long long int value;
      if(stringto(o->substr(p+1),value)) {
        if(name == "minspeed") {
          minspeed=value;
          buffer.speed.set_min_speed(minspeed,minspeedtime);
        } else if(name == "minspeedtime") {
          minspeedtime=value;
          buffer.speed.set_min_speed(minspeed,minspeedtime);
        } else if(name == "minavgspeed") {
          buffer.speed.set_min_average_speed(value);
        } else if(name == "maxinacttime") {
          buffer.speed.set_max_inactivity_time(value);
        } else if(name == "avgtime") {
          buffer.speed.set_base(value);
        };
      };
    };
  };

  UserConfig source_cfg(initializeCredentialsType(initializeCredentialsType::SkipCredentials));
  if(!source_cred_path.empty()) source_cfg.ProxyPath(source_cred_path);
  if(!source_ca_path.empty()) source_cfg.CACertificatesDirectory(source_ca_path);
  //source_cfg.UtilsDirPath(...); - probably not needed
  DataHandle source(source_url,source_cfg);
  if(!source) {
    std::cerr<<"Source URL not supported: "<<source_url<<std::endl;
    _exit(-1);
    //return -1;
  };
  source->SetSecure(false);
  source->Passive(true);
  UserConfig dest_cfg(initializeCredentialsType(initializeCredentialsType::SkipCredentials));
  if(!dest_cred_path.empty()) dest_cfg.ProxyPath(dest_cred_path);
  if(!dest_ca_path.empty()) dest_cfg.CACertificatesDirectory(dest_ca_path);
  //dest_cfg.UtilsDirPath(...); - probably not needed
  DataHandle dest(dest_url,dest_cfg);
  if(!dest) {
    std::cerr<<"Destination URL not supported: "<<dest_url<<std::endl;
    _exit(-1);
    //return -1;
  };
  dest->SetSecure(false);
  dest->Passive(true);

  // Filling initial report buffer
  ReportStatus(DataStaging::DTRStatus::NULL_STATE,DataStaging::DTRErrorStatus::NONE_ERROR,"",0,0);
  // Initiating transfer
  DataStatus source_st = source->StartReading(buffer);
  if(!source_st) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 (source_url.Protocol()!="file")?DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR:DataStaging::DTRErrorStatus::INTERNAL_ERROR,
                 std::string("Failed reading from source: ")+source->CurrentLocation().str()+" : "+source->GetFailureReason().GetDesc()+": "+source_st.GetDesc(),0,0);
    _exit(-1);
    //return -1;
  };
  DataStatus dest_st = dest->StartWriting(buffer);
  if(!dest_st) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 (dest_url.Protocol() != "file")?DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR:DataStaging::DTRErrorStatus::INTERNAL_ERROR,
                 std::string("Failed writing to destination: ")+dest->CurrentLocation().str()+(" : ")+dest->GetFailureReason().GetDesc()+": "+dest_st.GetDesc(),0,0);
    _exit(-1);
    //return -1;
  };
  // While transfer is running in another threads
  // here we periodically report status to parent
  bool eof_reached = false;
  for(;!buffer.error();) {
    if(buffer.eof_read() && buffer.eof_write()) {
      eof_reached = true; break;
    };
    ReportStatus(DataStaging::DTRStatus::TRANSFERRING,DataStaging::DTRErrorStatus::NONE_ERROR,"",buffer.speed.transferred_size(),GetFileSize(*source,*dest));
    buffer.wait_any();
  };
  ReportStatus(DataStaging::DTRStatus::TRANSFERRING,DataStaging::DTRErrorStatus::NONE_ERROR,"",buffer.speed.transferred_size(),GetFileSize(*source,*dest));
  bool source_failed = buffer.error_read();
  bool dest_failed = buffer.error_write();
  dest_st = dest->StopWriting();
  source_st = source->StopReading();
  bool reported = false;
  if(!eof_reached) {
    // Handle error
    if(source_failed || !source_st) {
      ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                   (source_url.Protocol() != "file")?DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR:DataStaging::DTRErrorStatus::INTERNAL_ERROR,
                   std::string("Failed reading from source: ")+source->CurrentLocation().str()+" : "+source->GetFailureReason().GetDesc()+": "+source_st.GetDesc(),0,0);
      reported = true;
    };
    if(dest_failed || !dest_st) {
      ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                   (dest_url.Protocol() != "file")?DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR:DataStaging::DTRErrorStatus::INTERNAL_ERROR,
                   std::string("Failed writing to destination: ")+dest->CurrentLocation().str()+" : "+dest->GetFailureReason().GetDesc()+": "+dest_st.GetDesc(),0,0);
      reported = true;
    };
    if((!dest_failed) && (!source_failed)) {
      ReportStatus(DataStaging::DTRStatus::TRANSFERRED,DataStaging::DTRErrorStatus::TRANSFER_SPEED_ERROR,"Failed transfering data",0,0);
      reported = true;
    };
  };
  if(!reported) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,DataStaging::DTRErrorStatus::NONE_ERROR,"",buffer.speed.transferred_size(),GetFileSize(*source,*dest));
  };
  _exit(eof_reached?0:1);
  //return eof_reached?0:1;
}

