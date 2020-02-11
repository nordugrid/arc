#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <pwd.h>

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/JobPerfLog.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/ArcConfigIni.h>
#include "../jobs/ContinuationPlugins.h"
#include "../run/RunPlugin.h"
#include "../log/JobLog.h"
#include "../log/JobsMetrics.h"
#include "../log/HeartBeatMetrics.h"
#include "../log/SpaceMetrics.h"
#include "../jobs/JobsList.h"

#include "CacheConfig.h"
#include "GMConfig.h"

#include "CoreConfig.h"

namespace ARex {

Arc::Logger CoreConfig::logger(Arc::Logger::getRootLogger(), "CoreConfig");

void CoreConfig::CheckLRMSBackends(const std::string& default_lrms) {
  std::string tool_path;
  tool_path=Arc::ArcLocation::GetDataDir()+"/cancel-"+default_lrms+"-job";
  if(!Glib::file_test(tool_path,Glib::FILE_TEST_IS_REGULAR)) {
    logger.msg(Arc::WARNING,"Missing cancel-%s-job - job cancellation may not work",default_lrms);
  }
  tool_path=Arc::ArcLocation::GetDataDir()+"/submit-"+default_lrms+"-job";
  if(!Glib::file_test(tool_path,Glib::FILE_TEST_IS_REGULAR)) {
    logger.msg(Arc::WARNING,"Missing submit-%s-job - job submission to LRMS may not work",default_lrms);
  }
  tool_path=Arc::ArcLocation::GetDataDir()+"/scan-"+default_lrms+"-job";
  if(!Glib::file_test(tool_path,Glib::FILE_TEST_IS_REGULAR)) {
    logger.msg(Arc::WARNING,"Missing scan-%s-job - may miss when job finished executing",default_lrms);
  }
}


bool CoreConfig::CheckYesNoCommand(bool& config_param, const std::string& name, std::string& rest) {
  std::string s = Arc::ConfigIni::NextArg(rest);
  if (s == "yes") {
    config_param = true;
  }
  else if(s == "no") {
    config_param = false;
  }
  else {
    logger.msg(Arc::ERROR, "Wrong option in %s", name);
    return false;
  }
  return true;
}

bool CoreConfig::ParseConf(GMConfig& config) {

  if (!config.conffile.empty()) {
    Arc::ConfigFile cfile;
    if (!cfile.open(config.conffile)) {
      logger.msg(Arc::ERROR, "Can't read configuration file at %s", config.conffile);
      return false;
    }
    // detect type of file
    Arc::ConfigFile::file_type type = cfile.detect();
    if (type == Arc::ConfigFile::file_INI) {
      bool result = ParseConfINI(config, cfile);
      cfile.close();
      return result;
    }
    logger.msg(Arc::ERROR, "Can't recognize type of configuration file at %s", config.conffile);
    return false;
  }
  logger.msg(Arc::ERROR, "Could not determine configuration type or configuration is empty");
  return false;
}

bool CoreConfig::ParseConfINI(GMConfig& config, Arc::ConfigFile& cfile) {

  // List of helper commands that will be substituted after all configuration is read
  std::list<std::string> helpers;
  std::string jobreport_publisher;
  bool helper_log_is_set = false;
  bool job_log_log_is_set = false;
  Arc::ConfigIni cf(cfile);
  cf.SetSectionIndicator(".");
  static const int common_secnum = 0;
  cf.AddSection("common");
  static const int ganglia_secnum = 1;
  cf.AddSection("arex/ganglia");
  static const int emies_secnum = 2;
  cf.AddSection("arex/ws/jobs");
  static const int ws_secnum = 3;
  cf.AddSection("arex/ws");
    static const int jura_secnum = 4;
  cf.AddSection("arex/jura");
  static const int gm_secnum = 5;
  cf.AddSection("arex");
  static const int infosys_secnum = 6;
  cf.AddSection("infosys"); 
  static const int queue_secnum = 7;
  cf.AddSection("queue");
  static const int ssh_secnum = 8;
  cf.AddSection("lrms/ssh");
  static const int lrms_secnum = 9;
  cf.AddSection("lrms");
  static const int cluster_secnum = 10;
  cf.AddSection("infosys/cluster");
  static const int perflog_secnum = 11;
  cf.AddSection("common/perflog");
  if (config.job_perf_log) {
    config.job_perf_log->SetEnabled(false);
    config.job_perf_log->SetOutput("/var/log/arc/perfdata/arex.perflog");
  }
  // process configuration information here
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command, rest);

    if (command.empty()) { // EOF
      break;
    }

    if (cf.SectionNum() == common_secnum) { // common
      if (cf.SubSection()[0] == '\0') {
        if(command == "x509_voms_dir") {
          config.voms_dir = rest;
        };
      };
      continue;
    };

    if (cf.SectionNum() == perflog_secnum) { // common/perflog
      if (cf.SubSection()[0] == '\0') {
        if(config.job_perf_log) config.job_perf_log->SetEnabled(true);
        if (command == "perflogdir") { // 
          if (!config.job_perf_log) continue;
          std::string fname = rest;  // empty is allowed too
          if(!fname.empty()) fname += "/arex.perflog";
          config.job_perf_log->SetOutput(fname.c_str());
        }
      };
      continue;
    };

    if (cf.SectionNum() == lrms_secnum) { // lrms
      if (cf.SubSection()[0] == '\0') {
        if (command == "lrms") { // default lrms type and queue (optional)
          std::string default_lrms = Arc::ConfigIni::NextArg(rest);
          if (default_lrms.empty()) {
            logger.msg(Arc::ERROR, "lrms is empty"); return false;
          }
          if (default_lrms == "slurm") { // allow lower case slurm in config
            default_lrms = "SLURM";
          }
          config.default_lrms = default_lrms;
          std::string default_queue = Arc::ConfigIni::NextArg(rest);
          if (!default_queue.empty()) {
            config.default_queue = default_queue;
          }
          CheckLRMSBackends(default_lrms);
        };
      };
      continue;
    };

    if (cf.SectionNum() == gm_secnum) { // arex
      if (cf.SubSection()[0] == '\0') {
        if (command == "user") {
          config.SetShareID(Arc::User(rest));
        }
        else if (command == "runtimedir") {
          config.rte_dir = rest;
        }
        else if (command == "maxjobs") { // maximum number of the jobs to support
          std::string max_jobs_s = Arc::ConfigIni::NextArg(rest);
          if (max_jobs_s.empty()) {
            logger.msg(Arc::ERROR, "Missing number in maxjobs"); return false;
          }
          if (!Arc::stringto(max_jobs_s, config.max_jobs)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobs: %s", max_jobs_s); return false;
          }
          if (config.max_jobs < 0) config.max_jobs = -1;

          max_jobs_s = Arc::ConfigIni::NextArg(rest);
          if (max_jobs_s.empty()) {
            logger.msg(Arc::ERROR, "Missing number in maxjobs"); return false;
          }
          if (!Arc::stringto(max_jobs_s, config.max_jobs_running)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobs: %s", max_jobs_s); return false;
          }
          if (config.max_jobs_running < 0) config.max_jobs_running = -1;

          max_jobs_s = Arc::ConfigIni::NextArg(rest);
          if (max_jobs_s.empty()) {
            logger.msg(Arc::ERROR, "Missing number in maxjobs"); return false;
          }
          if (!Arc::stringto(max_jobs_s, config.max_jobs_per_dn)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobs: %s", max_jobs_s); return false;
          }
          if (config.max_jobs_per_dn < 0) config.max_jobs_per_dn = -1;

          max_jobs_s = Arc::ConfigIni::NextArg(rest);
          if (max_jobs_s.empty()) {
            logger.msg(Arc::ERROR, "Missing number in maxjobs"); return false;
          }
          if (!Arc::stringto(max_jobs_s, config.max_jobs_total)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobs: %s", max_jobs_s); return false;
          }
          if (config.max_jobs_total < 0) config.max_jobs_total = -1;

          max_jobs_s = Arc::ConfigIni::NextArg(rest);
          if (max_jobs_s.empty()) {
            logger.msg(Arc::ERROR, "Missing number in maxjobs"); return false;
          }
          if (!Arc::stringto(max_jobs_s, config.max_scripts)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobs: %s", max_jobs_s); return false;
          }
          if (config.max_scripts < 0) config.max_scripts = -1;
        }
        else if(command == "norootpower") {
          if (!CheckYesNoCommand(config.strict_session, command, rest)) return false;
        }
        else if (command == "wakeupperiod") {
          std::string wakeup_s = Arc::ConfigIni::NextArg(rest);
          if (!Arc::stringto(wakeup_s, config.wakeup_period)) {
            logger.msg(Arc::ERROR,"Wrong number in wakeupperiod: %s",wakeup_s); return false;
          }
        }
        else if (command == "mail") { // internal address from which to send mail
          config.support_email_address = rest;
          if (config.support_email_address.empty()) {
            logger.msg(Arc::ERROR, "mail parameter is empty"); return false;
          }
        }
        else if (command == "defaultttl") { // time to keep job after finished
          std::string default_ttl_s = Arc::ConfigIni::NextArg(rest);
          if (!Arc::stringto(default_ttl_s, config.keep_finished)) {
            logger.msg(Arc::ERROR, "Wrong number in defaultttl command"); return false;
          }
          default_ttl_s = Arc::ConfigIni::NextArg(rest);
          if (!default_ttl_s.empty() && !Arc::stringto(default_ttl_s, config.keep_deleted)) {
            logger.msg(Arc::ERROR, "Wrong number in defaultttl command"); return false;
          }
        }
        else if (command == "maxrerun") { // number of retries allowed
          std::string default_reruns_s = Arc::ConfigIni::NextArg(rest);
          if (!Arc::stringto(default_reruns_s, config.reruns)) {
            logger.msg(Arc::ERROR, "Wrong number in maxrerun command"); return false;
          }
        }
        else if (command == "statecallout") { // set plugin to be called on state changes
          if (!config.cont_plugins) continue;
          std::string state_name = Arc::ConfigIni::NextArg(rest);
          if (state_name.empty()) {
            logger.msg(Arc::ERROR, "State name for plugin is missing"); return false;
          }
          std::string options_s = Arc::ConfigIni::NextArg(rest);
          if (options_s.empty()) {
            logger.msg(Arc::ERROR, "Options for plugin are missing"); return false;
          }
          if (!config.cont_plugins->add(state_name.c_str(), options_s.c_str(), rest.c_str())) {
            logger.msg(Arc::ERROR, "Failed to register plugin for state %s", state_name); return false;
          }
        }
        else if (command == "sessiondir") {  // set session root directory
          std::string session_root = Arc::ConfigIni::NextArg(rest);
          if (session_root.empty()) {
            logger.msg(Arc::ERROR, "Session root directory is missing"); return false;
          }
          if (rest.length() != 0 && rest != "drain") {
            logger.msg(Arc::ERROR, "Junk in sessiondir command"); return false;
          }
          if (session_root == "*") {
            // special value which uses each user's home area
            session_root = "%H/.jobs";
          }
          config.session_roots.push_back(session_root);
          if (rest != "drain") config.session_roots_non_draining.push_back(session_root);
        }
        else if (command == "controldir") {
          std::string control_dir = rest;
          if (control_dir.empty()) {
            logger.msg(Arc::ERROR, "Missing directory in control command"); return false;
          }
          config.control_dir = control_dir;
        }
        else if (command == "control") {
          logger.msg(Arc::WARNING, "'control' configuration option is no longer supported, please use 'controldir' instead");
        }
        else if (command == "helper") {
          std::string helper_user = Arc::ConfigIni::NextArg(rest);
          if (helper_user.empty()) {
            logger.msg(Arc::ERROR, "User for helper program is missing"); return false;
          }
          if (helper_user != ".") {
            logger.msg(Arc::ERROR, "Only user '.' for helper program is supported"); return false;
          }
          if (rest.empty()) {
            logger.msg(Arc::ERROR, "Helper program is missing"); return false;
          }
          helpers.push_back(rest);
        }
        else if (command == "helperlog") {
          config.helper_log = rest;
          // empty is allowed
          helper_log_is_set = true;
        }
        else if (command == "fixdirectories") {
          std::string s = Arc::ConfigIni::NextArg(rest);
          if (s == "yes") {
            config.fixdir = GMConfig::fixdir_always;
          }
          else if (s == "missing") {
            config.fixdir = GMConfig::fixdir_missing;
          }
          else if (s == "no") {
            config.fixdir = GMConfig::fixdir_never;
          }
          else {
            logger.msg(Arc::ERROR, "Wrong option in fixdirectories"); return false;
          }
        }
        else if (command == "scratchdir") {
          std::string scratch = rest;
          // don't set if already set by shared_scratch
          if (config.scratch_dir.empty()) config.scratch_dir = scratch;
        }
        else if (command == "shared_scratch") {
          std::string scratch = rest;
          config.scratch_dir = scratch;
        }
        else if (command == "joblog") { // where to write job information
          if (config.job_log) {
            std::string fname = rest;  // empty is allowed too
            config.job_log->SetOutput(fname.c_str());
          }
        }
        else if (command == "delegationdb") {
          std::string s = Arc::ConfigIni::NextArg(rest);
          if (s == "bdb") {
            config.deleg_db = GMConfig::deleg_db_bdb;
          }
          else if (s == "sqlite") {
            config.deleg_db = GMConfig::deleg_db_sqlite;
          }
          else {
            logger.msg(Arc::ERROR, "Wrong option in delegationdb"); return false;
          };
        }
        else if (command == "forcedefaultvoms") {
          std::string str = rest;
          if (str.empty()) {
            logger.msg(Arc::ERROR, "forcedefaultvoms parameter is empty"); return false;
          }
          config.forced_voms[""] = str;
        }
        /*
          #infoproviders_timelimit
          Currently information provider timeout is not implemented,
          hence no need to read this option.
        */
      };
      continue;
    };


    if (cf.SectionNum() == ganglia_secnum) { // arex/ganglia
      if (cf.SubSection()[0] == '\0') {
        if (!config.jobs_metrics) continue;
        if (command == "gmetric_bin_path") {
          std::string fname = rest;  // empty is not allowed, if not filled in arc.conf  default value is used
          config.jobs_metrics->SetGmetricPath(fname.c_str());
          config.heartbeat_metrics->SetGmetricPath(fname.c_str());
          config.space_metrics->SetGmetricPath(fname.c_str());
        }
        else if (command == "metrics") {
          std::list<std::string> metrics;
          Arc::tokenize(rest, metrics, ",");
          for(std::list<std::string>::iterator m = metrics.begin(); m != metrics.end(); ++m) {
            std::string metric = Arc::trim(*m);
            if((metric == "jobstates") || 
               (metric == "all")) {
              config.jobs_metrics->SetEnabled(true);
            };
	    if((metric == "heartbeat") ||
	       (metric == "all")){
		 config.heartbeat_metrics->SetEnabled(true);
	       };
	    if((metric == "cache") || 
	       (metric == "all")){
	      config.space_metrics->SetEnabled(true);
	    };
          };
        };
      };
      continue;
    };
    
    if (cf.SectionNum() == ws_secnum) { // arex/ws
     
      if (cf.SubSection()[0] == '\0') {
        if(command == "wsurl") {
           config.arex_endpoint = rest;
        };
      };
      continue;
    };

    if (cf.SectionNum() == emies_secnum) { // arex/ws/jobs
      if (cf.SubSection()[0] == '\0') {
        config.enable_emies_interface = true;
        config.enable_arc_interface =  true; // so far
        if (command == "allownew") {
          bool enable = false;
          if (!CheckYesNoCommand(enable, command, rest)) return false;
          config.allow_new = enable;
        }
        else if (command == "allownew_override") {
          while(!rest.empty()) {
            std::string str = Arc::ConfigIni::NextArg(rest);
            if(!str.empty()) {
              config.allow_submit.push_back(str);
            };
          };
        }
        else if (command == "maxjobdesc") {
          std::string maxjobdesc_s = Arc::ConfigIni::NextArg(rest);
          if (!Arc::stringto(maxjobdesc_s, config.maxjobdesc)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobdesc command"); return false;
          }
        } else if (command == "allowaccess") {
          while(!rest.empty()) {
            std::string str = Arc::ConfigIni::NextArg(rest);
            if(!str.empty()) {
              config.matching_groups[""].push_back(std::pair<bool,std::string>(true,str));
            };
          };
        } else if (command == "denyaccess") {
          while(!rest.empty()) {
            std::string str = Arc::ConfigIni::NextArg(rest);
            if(!str.empty()) {
              config.matching_groups[""].push_back(std::pair<bool,std::string>(false,str));
            };
          };
        };
      };
      continue;
    };

    if (cf.SectionNum() == jura_secnum) { // arex/jura
      if (cf.SubSection()[0] == '\0') {
        jobreport_publisher = "jura-ng";
        if (command == "logfile") {
          if (config.job_log) {
            std::string logfile = rest;
            if (logfile.empty()) {
              logger.msg(Arc::ERROR, "Missing file name in [arex/jura] logfile"); return false;
            };
            config.job_log->SetReporterLogFile(logfile.c_str());
            job_log_log_is_set = true;
          };
        }
        else if (command == "urdelivery_frequency") {
          if (config.job_log) {
            std::string period_s = Arc::ConfigIni::NextArg(rest);
            unsigned int period = 0;
            if (!Arc::stringto(period_s, period)) {
              logger.msg(Arc::ERROR, "Wrong number in urdelivery_frequency: %s", period_s); return false;
            }
            config.job_log->SetReporterPeriod(period);
          }
        }
        else if (command == "x509_host_key") {
          if (config.job_log) {
            std::string jobreport_key = rest;
            config.job_log->SetCredentials(jobreport_key, "", "");
          }
        }
        else if (command == "x509_host_cert") {
          if (config.job_log) {
            std::string jobreport_cert = rest;
            config.job_log->SetCredentials("", jobreport_cert, "");
          }
        }
        else if (command == "x509_cert_dir") {
          if (config.job_log) {
            std::string jobreport_cadir = rest;
            config.job_log->SetCredentials("", "", jobreport_cadir);
          }
        }
      };
      continue;
    };

    if (cf.SectionNum() == infosys_secnum) { // infosys - looking for user name to get share uid
      /*
      if (cf.SubSection()[0] == '\0') {
        if (command == "user") {
          config.SetShareID(Arc::User(rest));
        };
      };
      */
      continue;
    }

    if (cf.SectionNum() == queue_secnum) { // queue
      if (cf.SubSection()[0] == '\0') {
        if (cf.SectionNew()) {
          std::string name = cf.SectionIdentifier();
          if (name.empty()) {
            logger.msg(Arc::ERROR, "No queue name given in queue block name"); return false;
          }
          config.queues.push_back(name);
        }
        if (command == "forcedefaultvoms") {
          std::string str = rest;
          if (str.empty()) {
            logger.msg(Arc::ERROR, "forcedefaultvoms parameter is empty"); return false;
          }
          if (!config.queues.empty()) {
            std::string queue_name = *(--config.queues.end());
            config.forced_voms[queue_name] = str;
          }
        } else if (command == "advertisedvo") {
          std::string str = rest;
          if (str.empty()) {
            logger.msg(Arc::ERROR, "advertisedvo parameter is empty"); return false;
          }
          if (!config.queues.empty()) {
            std::string queue_name = *(--config.queues.end());
            config.authorized_vos[queue_name].push_back(str);
          }
        } else if (command == "allowaccess") {
          if (!config.queues.empty()) {
            std::string queue_name = *(--config.queues.end());
            while(!rest.empty()) {
              std::string str = Arc::ConfigIni::NextArg(rest);
              if(!str.empty()) {
                config.matching_groups[queue_name].push_back(std::pair<bool,std::string>(true,str));
              }
            }
          }
        } else if (command == "denyaccess") {
          if (!config.queues.empty()) {
            std::string queue_name = *(--config.queues.end());
            while(!rest.empty()) {
              std::string str = Arc::ConfigIni::NextArg(rest);
              if(!str.empty()) {
                config.matching_groups[queue_name].push_back(std::pair<bool,std::string>(false,str));
              }
            }
          }
        }
      }
      continue;
    }
    if (cf.SectionNum() == cluster_secnum) { // cluster
      if (cf.SubSection()[0] == '\0') {
        if (command == "advertisedvo") {
          std::string str = rest;
          if (str.empty()) {
            logger.msg(Arc::ERROR, "advertisedvo parameter is empty"); return false;
          }
          config.authorized_vos[""].push_back(str);
        };
      };
      continue;
    }

    if (cf.SectionNum() == ssh_secnum) { // ssh
      if (cf.SubSection()[0] == '\0') {
        config.sshfs_mounts_enabled = true;
      }
      continue;
    }
  };
  // End of parsing conf commands

  // Define accounting reporter and database manager if configured
  if(config.job_log) {
    if(!jobreport_publisher.empty()) {
      config.job_log->SetReporter(jobreport_publisher.c_str());
      if(!job_log_log_is_set) config.job_log->SetReporterLogFile("/var/log/arc/jura.log");
    }
  }

  if(!helper_log_is_set) {
    // Assign default value
    config.helper_log = "/var/log/arc/job.helper.errors";
  }
 
  // Do substitution of control dir and helpers here now we have all the
  // configuration. These are special because they do not change per-user
  config.Substitute(config.control_dir);
  for (std::list<std::string>::iterator helper = helpers.begin(); helper != helpers.end(); ++helper) {
    config.Substitute(*helper);
    config.helpers.push_back(*helper);
  }

  // Add helper to poll for finished LRMS jobs
  if (!config.default_lrms.empty() && !config.control_dir.empty()) {
    std::string cmd = Arc::ArcLocation::GetDataDir() + "/scan-"+config.default_lrms+"-job";
    cmd = Arc::escape_chars(cmd, " \\", '\\', false);
    if (!config.conffile.empty()) cmd += " --config " + config.conffile;
    cmd += " " + config.control_dir;
    config.helpers.push_back(cmd);
  }

  // Get cache parameters
  try {
    CacheConfig cache_config = CacheConfig(config);
    config.cache_params = cache_config;
  }
  catch (CacheConfigException& e) {
    logger.msg(Arc::ERROR, "Error with cache configuration: %s", e.what());
    return false;
  }

  return true;
}

} // namespace ARex
