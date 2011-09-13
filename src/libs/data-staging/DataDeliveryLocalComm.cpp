#include <arc/ArcLocation.h>

#include "DataDeliveryLocalComm.h"

namespace DataStaging {

  DataDeliveryLocalComm::DataDeliveryLocalComm(const DTR& dtr, const TransferParameters& params)
    : DataDeliveryComm(dtr, params),child_(NULL),last_comm(Arc::Time()) {
    if(!dtr.get_source()) return;
    if(!dtr.get_destination()) return;
    {
      Glib::Mutex::Lock lock(lock_);
      // Initial empty status
      memset(&status_,0,sizeof(status_));
      status_.commstatus = CommInit;
      status_pos_ = 0;
      // Generate options for child
      std::list<std::string> args;
      // maybe just nordugrid_libexec_loc_?
      std::string execpath = Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBEXECSUBDIR+G_DIR_SEPARATOR_S+"DataStagingDelivery";
      args.push_back(execpath);
      // check for alternative source or destination eg cache, mapped URL, TURL
      if (dtr.get_source()->TransferLocations().empty()) {
        logger_->msg(Arc::ERROR, "DTR %s: No locations defined for %s", dtr_id, dtr.get_source()->str());
        return;
      }
      std::string surl = dtr.get_source()->TransferLocations()[0].fullstr();
      bool caching = false;
      if (!dtr.get_mapped_source().empty())
        surl = dtr.get_mapped_source();

      if (dtr.get_destination()->TransferLocations().empty()) {
        logger_->msg(Arc::ERROR, "DTR %s: No locations defined for %s", dtr_id, dtr.get_destination()->str());
        return;
      }
      std::string durl = dtr.get_destination()->TransferLocations()[0].fullstr();
      if ((dtr.get_cache_state() == CACHEABLE) && !dtr.get_cache_file().empty()) {
        durl = dtr.get_cache_file();
        caching = true;
      }
      args.push_back("--surl");
      args.push_back(surl);
      args.push_back("--durl");
      args.push_back(durl);
      if (!dtr.get_usercfg().ProxyPath().empty()) {
        args.push_back("--sopt");
        args.push_back("credential="+dtr.get_usercfg().ProxyPath());
        args.push_back("--dopt");
        args.push_back("credential="+dtr.get_usercfg().ProxyPath());
      }
      if (!dtr.get_usercfg().CACertificatesDirectory().empty()) {
        args.push_back("--sopt");
        args.push_back("ca="+dtr.get_usercfg().CACertificatesDirectory());
        args.push_back("--dopt");
        args.push_back("ca="+dtr.get_usercfg().CACertificatesDirectory());
      }
      args.push_back("--topt");
      args.push_back("minspeed="+Arc::tostring(transfer_params.min_current_bandwidth));
      args.push_back("--topt");
      args.push_back("minspeedtime="+Arc::tostring(transfer_params.averaging_time));
      args.push_back("--topt");
      args.push_back("minavgspeed="+Arc::tostring(transfer_params.min_average_bandwidth));
      args.push_back("--topt");
      args.push_back("maxinacttime="+Arc::tostring(transfer_params.max_inactivity_time));

      if (dtr.get_source()->CheckCheckSum()) {
        std::string csum(dtr.get_source()->GetCheckSum());
        std::string::size_type pos(csum.find(':'));
        if (pos == std::string::npos || pos == csum.length()-1) {
          logger_->msg(Arc::WARNING, "DTR %s: Bad checksum format %s", dtr_id, csum);
        } else {
          args.push_back("--cstype");
          args.push_back(csum.substr(0, pos));
          args.push_back("--csvalue");
          args.push_back(csum.substr(pos+1));
        }
      } else if (!dtr.get_destination()->GetURL().Option("checksum").empty()) {
        args.push_back("--cstype");
        args.push_back(dtr.get_destination()->GetURL().Option("checksum"));
      } else if (dtr.get_destination()->AcceptsMeta() || dtr.get_destination()->ProvidesMeta()) {
        args.push_back("--cstype");
        args.push_back(dtr.get_destination()->DefaultCheckSum());
      }
      child_ = new Arc::Run(args);
      // Set up pipes
      child_->KeepStdout(false);
      child_->KeepStderr(false);
      child_->KeepStdin(false);
      if(!caching) {
        child_->AssignUserId(dtr.get_local_user().get_uid());
        child_->AssignGroupId(dtr.get_local_user().get_gid());
      }
      // Start child
      std::string cmd;
      for(std::list<std::string>::iterator arg = args.begin();arg!=args.end();++arg) {
        cmd += *arg;
        cmd += " ";
      }
      logger_->msg(Arc::DEBUG, "DTR %s: Running command: %s", dtr_id, cmd);
      if(!child_->Start()) {
        delete child_;
        child_=NULL;
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
            logger_->msg(Arc::INFO, "DTR %s: DataDelivery: %s", dtr_id, start);
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
              logger_->msg(Arc::ERROR, "DTR %s: DataStagingDelivery exited with code %i", dtr_id, child_->Result());
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
    if (transfer_params.max_inactivity_time > 0 && t >= transfer_params.max_inactivity_time) {
      logger_->msg(Arc::ERROR, "Transfer killed after %i seconds without communication", t.GetPeriod());
      child_->Kill(1);
      delete child_;
      child_ = NULL;
    }
  }

} // namespace DataStaging
