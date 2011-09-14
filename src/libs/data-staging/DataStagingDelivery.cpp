#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBuffer.h>

#include "DataDeliveryComm.h"

using namespace Arc;

static Arc::Logger logger(Arc::Logger::getRootLogger(), "DataDelivery");
static bool shutdown = false;

static void sig_shutdown(int)
{
    if(shutdown) _exit(0);
    shutdown = true;
}

static void ReportStatus(DataStaging::DTRStatus::DTRStatusType st,
                         DataStaging::DTRErrorStatus::DTRErrorStatusType err,
                         DataStaging::DTRErrorStatus::DTRErrorLocation err_loc,
                         const std::string& err_desc,
                         unsigned long long int transferred,
                         unsigned long long int size,
                         const std::string& checksum = "") {
  static DataStaging::DataDeliveryComm::Status status;
  static DataStaging::DataDeliveryComm::Status status_;
  static unsigned int status_pos = 0;
  static bool status_changed = true;

  // Filling
  status.commstatus = DataStaging::DataDeliveryComm::CommNoError;
  status.timestamp = ::time(NULL);
  status.status = st;
  status.error = err;
  status.error_location = err_loc;
  strncpy(status.error_desc,err_desc.c_str(),sizeof(status.error_desc));
  status.streams = 0;
  status.transferred = transferred;
  status.size = size;
  status.offset = 0;
  status.speed = 0;
  strncpy(status.checksum, checksum.c_str(), sizeof(status.checksum));
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

  // log to stderr
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE); //TODO: configurable
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::EmptyFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);

  // Collecting parameters
  // --surl: source URL 
  // --durl: destination URL
  // --sopt: any URL option, credential - path to file storing credentials
  // --dopt: any URL option, credential - path to file storing credentials
  // --topt: minspeed, minspeedtime, minavgspeed, maxinacttime, avgtime
  // --cstype: checksum type to calculate
  // --csvalue: checksum value of source file to validate against
  // surl, durl, cstype and csvalue may be given only once
  // sopt, dopt, topt may be given multiple times
  // type of credentials is detected automatically, so far only 
  // X.509 proxies or key+certificate are accepted
  std::string source_str;
  std::string dest_str;
  std::list<std::string> source_opts;
  std::list<std::string> dest_opts;
  std::list<std::string> transfer_opts;
  std::string checksum_type;
  std::string checksum_value;
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
  opt.AddOption(0,"cstype","","checksum type",checksum_type);
  opt.AddOption(0,"csvalue","","checksum value",checksum_value);
  if(opt.Parse(argc,argv).size() != 0) {
    logger.msg(ERROR, "Unexpected arguments"); return -1;
  };
  if(source_str.empty()) {
    logger.msg(ERROR, "Source URL missing"); return -1;
  };
  if(dest_str.empty()) {
    logger.msg(ERROR, "Destination URL missing"); return -1;
  };
  URL source_url(source_str);
  if(!source_url) {
    logger.msg(ERROR, "Source URL not valid: %s", source_str); return -1;
  };
  URL dest_url(dest_str);
  if(!dest_url) {
    logger.msg(ERROR, "Destination URL not valid: %s", dest_str); return -1;
  };
  for(std::list<std::string>::iterator o = source_opts.begin();
                           o != source_opts.end();++o) {
    std::string::size_type p = o->find('=');
    if(p == std::string::npos) {
      source_url.AddOption(*o);
    } else {
      std::string name = o->substr(0,p);
      if(name == "credential") {
        source_cred_path = o->substr(p+1);
      } else if(name == "ca") {
        source_ca_path = o->substr(p+1);
      } else {
        source_url.AddOption(*o);
      };
    };
  };
  for(std::list<std::string>::iterator o = dest_opts.begin();
                           o != dest_opts.end();++o) {
    std::string::size_type p = o->find('=');
    if(p == std::string::npos) {
      dest_url.AddOption(*o);
    } else {
      std::string name = o->substr(0,p);
      if(name == "credential") {
        dest_cred_path = o->substr(p+1);
      } else if(name == "ca") {
        dest_ca_path = o->substr(p+1);
      } else {
        dest_url.AddOption(*o);
      };
    };
  };

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
        } else if(name == "minspeedtime") {
          minspeedtime=value;
        } else if(name == "minavgspeed") {
          buffer.speed.set_min_average_speed(value);
        } else if(name == "maxinacttime") {
          buffer.speed.set_max_inactivity_time(value);
        } else if(name == "avgtime") {
          buffer.speed.set_base(value);
        } else {
          logger.msg(ERROR, "Unknown transfer option: %s", name);
          _exit(-1);
        }
      };
    };
  }
  buffer.speed.set_min_speed(minspeed,minspeedtime);

  initializeCredentialsType source_cred(initializeCredentialsType::SkipCredentials);
  UserConfig source_cfg(source_cred);
  if(!source_cred_path.empty()) source_cfg.ProxyPath(source_cred_path);
  if(!source_ca_path.empty()) source_cfg.CACertificatesDirectory(source_ca_path);
  //source_cfg.UtilsDirPath(...); - probably not needed
  DataHandle source(source_url,source_cfg);
  if(!source) {
    logger.msg(ERROR, "Source URL not supported: %s", source_url.str());
    _exit(-1);
    //return -1;
  };
  source->SetSecure(false);
  source->Passive(true);
  initializeCredentialsType dest_cred(initializeCredentialsType::SkipCredentials);
  UserConfig dest_cfg(dest_cred);
  if(!dest_cred_path.empty()) dest_cfg.ProxyPath(dest_cred_path);
  if(!dest_ca_path.empty()) dest_cfg.CACertificatesDirectory(dest_ca_path);
  //dest_cfg.UtilsDirPath(...); - probably not needed
  DataHandle dest(dest_url,dest_cfg);
  if(!dest) {
    logger.msg(ERROR, "Destination URL not supported: %s", dest_url.str());
    _exit(-1);
    //return -1;
  };
  dest->SetSecure(false);
  dest->Passive(true);

  // set signal handlers
  signal(SIGTERM, sig_shutdown);
  signal(SIGINT, sig_shutdown);

  // Filling initial report buffer
  ReportStatus(DataStaging::DTRStatus::NULL_STATE,
               DataStaging::DTRErrorStatus::NONE_ERROR,
               DataStaging::DTRErrorStatus::NO_ERROR_LOCATION,
               "",0,0,"");

  // if checksum type is supplied, use that type, otherwise use default for the
  // destination (if checksum is supported by the destination protocol)
  CheckSumAny crc;
  CheckSumAny crc_source;
  CheckSumAny crc_dest;
  std::string crc_type("");

  if (!checksum_type.empty()) {
    crc_type = checksum_type;
    if (!checksum_value.empty())
      source->SetCheckSum(checksum_type+':'+checksum_value);
  }
  else if (dest->AcceptsMeta() || dest->ProvidesMeta()) {
    crc_type = dest->DefaultCheckSum();
  }

  if (!crc_type.empty()) {
    crc = crc_type.c_str();
    crc_source = crc_type.c_str();
    crc_dest = crc_type.c_str();
    if (crc.Type() != CheckSumAny::none) logger.msg(INFO, "Will calculate %s checksum", crc_type);
    source->AddCheckSumObject(&crc_source);
    dest->AddCheckSumObject(&crc_dest);
  }
  buffer.set(&crc);

  // Initiating transfer
  DataStatus source_st = source->StartReading(buffer);
  if(!source_st) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 (source_url.Protocol()!="file") ?
                  (source_st.Retryable() ? DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR :
                                           DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR) :
                  DataStaging::DTRErrorStatus::LOCAL_FILE_ERROR,
                 DataStaging::DTRErrorStatus::ERROR_SOURCE,
                 std::string("Failed reading from source: ")+source->CurrentLocation().str()+
                  " : "+source->GetFailureReason().GetDesc()+": "+source_st.GetDesc(),
                 0,0);
    _exit(-1);
    //return -1;
  };
  DataStatus dest_st = dest->StartWriting(buffer);
  if(!dest_st) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 (dest_url.Protocol() != "file") ?
                  (dest_st.Retryable() ? DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR :
                                         DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR) :
                  DataStaging::DTRErrorStatus::LOCAL_FILE_ERROR,
                 DataStaging::DTRErrorStatus::ERROR_DESTINATION,
                 std::string("Failed writing to destination: ")+dest->CurrentLocation().str()+
                  " : "+dest->GetFailureReason().GetDesc()+": "+dest_st.GetDesc(),
                 0,0);
    _exit(-1);
    //return -1;
  };
  // While transfer is running in another threads
  // here we periodically report status to parent
  bool eof_reached = false;
  for(;!buffer.error() && !shutdown;) {
    if(buffer.eof_read() && buffer.eof_write()) {
      eof_reached = true; break;
    };
    ReportStatus(DataStaging::DTRStatus::TRANSFERRING,
                 DataStaging::DTRErrorStatus::NONE_ERROR,
                 DataStaging::DTRErrorStatus::NO_ERROR_LOCATION,
                 "",
                 buffer.speed.transferred_size(),
                 GetFileSize(*source,*dest));
    buffer.wait_any();
  };
  if (shutdown) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 DataStaging::DTRErrorStatus::INTERNAL_PROCESS_ERROR,
                 DataStaging::DTRErrorStatus::ERROR_TRANSFER,
                 "DataStagingProcess process killed",
                 buffer.speed.transferred_size(),
                 GetFileSize(*source,*dest));
    buffer.error_write(true); // to trigger cleanup of destination
    dest->StopWriting();
    _exit(-1);
  }
  ReportStatus(DataStaging::DTRStatus::TRANSFERRING,
               DataStaging::DTRErrorStatus::NONE_ERROR,
               DataStaging::DTRErrorStatus::NO_ERROR_LOCATION,
               "",
               buffer.speed.transferred_size(),
               GetFileSize(*source,*dest));

  bool source_failed = buffer.error_read();
  bool dest_failed = buffer.error_write();
  dest_st = dest->StopWriting();
  source_st = source->StopReading();
  bool reported = false;

  // Error at source or destination
  if(source_failed || !source_st) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 (source_url.Protocol() != "file") ?
                  (!source_st && source_st.Retryable() ?
                      DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR :
                      DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR) :
                  DataStaging::DTRErrorStatus::LOCAL_FILE_ERROR,
                 DataStaging::DTRErrorStatus::ERROR_SOURCE,
                 std::string("Failed reading from source: ")+source->CurrentLocation().str()+
                  " : "+source->GetFailureReason().GetDesc()+": "+source_st.GetDesc(),
                 buffer.speed.transferred_size(),
                 GetFileSize(*source,*dest));
    reported = true;
  };
  if(dest_failed || !dest_st) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 (dest_url.Protocol() != "file") ?
                  (!dest_st && dest_st.Retryable() ?
                         DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR :
                         DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR) :
                  DataStaging::DTRErrorStatus::LOCAL_FILE_ERROR,
                 DataStaging::DTRErrorStatus::ERROR_DESTINATION,
                 std::string("Failed writing to destination: ")+dest->CurrentLocation().str()+
                  " : "+dest->GetFailureReason().GetDesc()+": "+dest_st.GetDesc(),
                 buffer.speed.transferred_size(),
                 GetFileSize(*source,*dest));
    reported = true;
  };

  // Transfer error, usually timeout
  if(!eof_reached) {
    if((!dest_failed) && (!source_failed)) {
      ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                   DataStaging::DTRErrorStatus::TRANSFER_SPEED_ERROR,
                   DataStaging::DTRErrorStatus::ERROR_UNKNOWN,
                   "Transfer timed out",
                   buffer.speed.transferred_size(),
                   GetFileSize(*source,*dest));
      reported = true;
    };
  };

  // checksum validation against supplied value
  std::string calc_csum;
  if (crc && buffer.checksum_valid()) {
    char buf[100];
    crc.print(buf,100);
    calc_csum = buf;
  } else if(crc_source) {
    char buf[100];
    crc_source.print(buf,100);
    calc_csum = buf;
  } else if(crc_dest) {
    char buf[100];
    crc_dest.print(buf,100);
    calc_csum = buf;
  }
  if (!calc_csum.empty() && crc.Type() != CheckSumAny::none) {

    // compare calculated to any checksum given as an option
    if (source->CheckCheckSum()) {
      if (calc_csum.substr(0, calc_csum.find(":")) != checksum_type) {
        logger.msg(INFO, "Checksum type of source and calculated checksum differ, cannot compare");
      } else if (calc_csum.substr(calc_csum.find(":")+1) != checksum_value) {
        logger.msg(ERROR, "Checksum mismatch between calcuated checksum %s and source checksum %s", calc_csum, source->GetCheckSum());
        ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                     DataStaging::DTRErrorStatus::TRANSFER_SPEED_ERROR,
                     DataStaging::DTRErrorStatus::ERROR_UNKNOWN,
                     "Checksum mismatch",
                     0,0);
        reported = true;
        eof_reached = false; // TODO general error flag is better than this
      }
      else
        logger.msg(INFO, "Calculated transfer checksum %s matches source checksum", calc_csum);
    }
  } else {
    logger.msg(VERBOSE, "Checksum not computed");
  }
  if(!reported) {
    ReportStatus(DataStaging::DTRStatus::TRANSFERRED,
                 DataStaging::DTRErrorStatus::NONE_ERROR,
                 DataStaging::DTRErrorStatus::NO_ERROR_LOCATION,
                 "",
                 buffer.speed.transferred_size(),
                 GetFileSize(*source,*dest),
                 calc_csum);
  };
  _exit(eof_reached?0:1);
  //return eof_reached?0:1;
}

