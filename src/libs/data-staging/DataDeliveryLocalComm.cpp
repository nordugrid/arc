#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcLocation.h>
#include <arc/FileAccess.h>
#include <arc/FileUtils.h>

#include "DataDeliveryLocalComm.h"

namespace DataStaging {

  // Check if needed and create copy of proxy with suitable ownership
  static std::string prepare_proxy(const std::string& proxy_path, int child_uid, int child_gid) {
    if (proxy_path.empty()) return ""; // No credentials
    int my_uid = (int)::getuid();
    if (my_uid != 0) return ""; // Can't switch user id
    if (child_uid == 0) return ""; // Not switching
    if (child_uid == my_uid) return ""; // Not switching
    // Check ownership of credentials.
    struct ::stat st;
    if(!Arc::FileStat(proxy_path,&st,true)) return ""; // Can't stat - won't read
    if(st.st_uid == child_uid) return ""; // Owned by child
    // Ownership may prevent reading of file.
    std::string proxy_content;
    if(!Arc::FileRead(proxy_path, proxy_content)) return "";
    // Creating temporary file
    // Probably not most effective solution. But makes sure
    // access permissions are set properly.
    std::string proxy_new_path;
    if(!Arc::TmpFileCreate(proxy_new_path, proxy_content, child_uid, child_gid, S_IRUSR|S_IWUSR)) {
      if (!proxy_new_path.empty()) Arc::FileDelete(proxy_new_path);
      return "";
    }
    return proxy_new_path;
  }

  DataDeliveryLocalComm::DataDeliveryLocalComm(DTR_ptr dtr, const TransferParameters& params)
    : DataDeliveryComm(dtr, params),child_(NULL),last_comm(Arc::Time()) {
    // Initial empty status
    memset(&status_,0,sizeof(status_));
    status_.commstatus = CommInit;
    status_pos_ = 0;
    if(!dtr->get_source()) {
      logger_->msg(Arc::ERROR, "No source defined");
      return;
    }
    if(!dtr->get_destination()) {
      logger_->msg(Arc::ERROR, "No destination defined");
      return;
    }
    {
      Glib::Mutex::Lock lock(lock_);
      // Generate options for child
      std::list<std::string> args;
      std::string execpath = Arc::ArcLocation::GetLibDir()+G_DIR_SEPARATOR_S+"DataStagingDelivery";
      args.push_back(execpath);

      // check for alternative source or destination eg cache, mapped URL, TURL
      std::string surl;
      if (!dtr->get_mapped_source().empty()) {
        surl = dtr->get_mapped_source();
      }
      else if (!dtr->get_source()->TransferLocations().empty()) {
        surl = dtr->get_source()->TransferLocations()[0].fullstr();
      }
      else {
        logger_->msg(Arc::ERROR, "No locations defined for %s", dtr->get_source()->str());
        return;
      }

      if (dtr->get_destination()->TransferLocations().empty()) {
        logger_->msg(Arc::ERROR, "No locations defined for %s", dtr->get_destination()->str());
        return;
      }
      std::string durl = dtr->get_destination()->TransferLocations()[0].fullstr();
      bool caching = false;
      if ((dtr->get_cache_state() == CACHEABLE) && !dtr->get_cache_file().empty()) {
        durl = dtr->get_cache_file();
        caching = true;
      }
      else if (!dtr->get_mapped_destination().empty()) {
        durl = dtr->get_mapped_destination();
      }
      int child_uid = 0;
      int child_gid = 0;
      if(!caching) {
        child_uid = dtr->get_local_user().get_uid();
        child_gid = dtr->get_local_user().get_gid();
      }
      args.push_back("--surl");
      args.push_back(surl);
      args.push_back("--durl");
      args.push_back(durl);
      // Check if credentials are needed for source/dest
      Arc::DataHandle surl_h(surl, dtr->get_usercfg());
      Arc::DataHandle durl_h(durl, dtr->get_usercfg());

      bool needCredentialsInFile = (surl_h && surl_h->RequiresCredentialsInFile()) ||
                                   (durl_h && durl_h->RequiresCredentialsInFile());
      if (!needCredentialsInFile) {
        // If file-based credentials are not required then send through stdin
        if (!dtr->get_usercfg().OToken().empty()) {
          stdin_ = "token ";
	  stdin_ += dtr->get_usercfg().OToken();
	} else {
          stdin_ = "x509 ";
          stdin_ += dtr->get_usercfg().CredentialString();
        }
      } else {
        // If child is going to be run under different user ID
        // we must ensure it will be able to read credentials.
        tmp_proxy_ = prepare_proxy(dtr->get_usercfg().ProxyPath(), child_uid, child_gid);
        if (!tmp_proxy_.empty()) {
          args.push_back("--sopt");
          args.push_back(std::string("credential=")+tmp_proxy_);
          args.push_back("--dopt");
          args.push_back(std::string("credential=")+tmp_proxy_);
        } else if(!dtr->get_usercfg().ProxyPath().empty()) {
          args.push_back("--sopt");
          args.push_back(std::string("credential=")+dtr->get_usercfg().ProxyPath());
          args.push_back("--dopt");
          args.push_back(std::string("credential=")+dtr->get_usercfg().ProxyPath());
        }
      }
      if (!dtr->get_usercfg().CACertificatesDirectory().empty()) {
        args.push_back("--sopt");
        args.push_back(std::string("ca=")+dtr->get_usercfg().CACertificatesDirectory());
        args.push_back("--dopt");
        args.push_back(std::string("ca=")+dtr->get_usercfg().CACertificatesDirectory());
      }
      args.push_back("--sopt");
      args.push_back(std::string("casystem=")+Arc::tostring((int)dtr->get_usercfg().CAUseSystem()));
      args.push_back("--dopt");
      args.push_back(std::string("casystem=")+Arc::tostring((int)dtr->get_usercfg().CAUseSystem()));
      args.push_back("--topt");
      args.push_back(std::string("minspeed=")+Arc::tostring(transfer_params.min_current_bandwidth));
      args.push_back("--topt");
      args.push_back(std::string("minspeedtime=")+Arc::tostring(transfer_params.averaging_time));
      args.push_back("--topt");
      args.push_back(std::string("minavgspeed=")+Arc::tostring(transfer_params.min_average_bandwidth));
      args.push_back("--topt");
      args.push_back(std::string("maxinacttime=")+Arc::tostring(transfer_params.max_inactivity_time));

      if (dtr->get_source()->CheckSize()) {
        args.push_back("--size");
        args.push_back(Arc::tostring(dtr->get_source()->GetSize()));
      }
      if (dtr->get_source()->CheckCheckSum()) {
        std::string csum(dtr->get_source()->GetCheckSum());
        std::string::size_type pos(csum.find(':'));
        if (pos == std::string::npos || pos == csum.length()-1) {
          logger_->msg(Arc::WARNING, "Bad checksum format %s", csum);
        } else {
          args.push_back("--cstype");
          args.push_back(csum.substr(0, pos));
          args.push_back("--csvalue");
          args.push_back(csum.substr(pos+1));
        }
      } else if (!dtr->get_destination()->GetURL().MetaDataOption("checksumtype").empty()) {
        args.push_back("--cstype");
        args.push_back(dtr->get_destination()->GetURL().MetaDataOption("checksumtype"));
        if (!dtr->get_destination()->GetURL().MetaDataOption("checksumvalue").empty()) {
          args.push_back("--csvalue");
          args.push_back(dtr->get_destination()->GetURL().MetaDataOption("checksumvalue"));
        }
      } else if (!dtr->get_destination()->GetURL().Option("checksum").empty()) {
        args.push_back("--cstype");
        args.push_back(dtr->get_destination()->GetURL().Option("checksum"));
      } else if (dtr->get_destination()->AcceptsMeta() || dtr->get_destination()->ProvidesMeta()) {
        args.push_back("--cstype");
        args.push_back(dtr->get_destination()->DefaultCheckSum());
      }
      child_ = new Arc::Run(args);
      // Set up pipes
      child_->KeepStdout(false);
      child_->KeepStderr(false);
      child_->KeepStdin(false);
      child_->AssignUserId(child_uid);
      child_->AssignGroupId(child_gid);
      child_->AssignStdin(stdin_);
      // Start child
      std::string cmd;
      for(std::list<std::string>::iterator arg = args.begin();arg!=args.end();++arg) {
        cmd += *arg;
        cmd += " ";
      }
      logger_->msg(Arc::DEBUG, "Running command: %s", cmd);
      if(!child_->Start()) {
        delete child_;
        child_=NULL;
        logger_->msg(Arc::ERROR, "Failed to run command: %s", cmd);
        return;
      }
    }
    handler_->Add(this);
  }

  DataDeliveryLocalComm::~DataDeliveryLocalComm(void) {
    {
      Glib::Mutex::Lock lock(lock_);
      if(child_) {
        child_->Kill(10); // Give it a chance
        delete child_; child_=NULL;  // And then kill for sure
      }
    }
    if(!tmp_proxy_.empty()) Arc::FileDelete(tmp_proxy_);
    if(handler_) handler_->Remove(this);
  }

  void DataDeliveryLocalComm::PullStatus(void) {
    Glib::Mutex::Lock lock(lock_);
    if(!child_) return;
    for(;;) {
      if(status_pos_ < sizeof(status_buf_)) {
        int l;
        // TODO: direct redirect
        for(;;) {
          char buf[1024+1];
          l = child_->ReadStderr(0,buf,sizeof(buf)-1);
          if(l <= 0) break;
          buf[l] = 0;
          char* start = buf;
          for(;*start;) {
            char* end = strchr(start,'\n');
            if(end) *end = 0;
            logger_->msg(Arc::INFO, "DataDelivery: %s", start);
            if(!end) break;
            start = end + 1;
          }
        }
        l = child_->ReadStdout(0,((char*)&status_buf_)+status_pos_,sizeof(status_buf_)-status_pos_);
        if(l == -1) { // child error or closed comm
          if(child_->Running()) {
            status_.commstatus = CommClosed;
          } else {
            status_.commstatus = CommExited;
            if(child_->Result() != 0) {
              logger_->msg(Arc::ERROR, "DataStagingDelivery exited with code %i", child_->Result());
              status_.commstatus = CommFailed;
            }
          }
          delete child_; child_=NULL; return;
        }
        if(l == 0) break;
        status_pos_+=l;
        last_comm = Arc::Time();
      }
      if(status_pos_ >= sizeof(status_buf_)) {
        status_buf_.error_desc[sizeof(status_buf_.error_desc)-1] = 0;
        status_=status_buf_;
        status_pos_-=sizeof(status_buf_);
      }
    }
    // check for stuck child process (no report through comm channel)
    Arc::Period t = Arc::Time() - last_comm;
    if (transfer_params.max_inactivity_time > 0 && t >= transfer_params.max_inactivity_time*2) {
      logger_->msg(Arc::ERROR, "Transfer killed after %i seconds without communication", t.GetPeriod());
      child_->Kill(1);
      delete child_;
      child_ = NULL;
    }
  }

  bool DataDeliveryLocalComm::CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg) {
    allowed_dirs.push_back("/");
    double avg[3];
    if (getloadavg(avg, 3) != 3) {
      load_avg = "-1";
    } else {
      load_avg = Arc::tostring(avg[1]);
    }

    return true;
  }


} // namespace DataStaging
