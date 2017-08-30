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
#include <arc/XMLNode.h>
#include <arc/ArcConfigIni.h>
#include "../jobs/ContinuationPlugins.h"
#include "../run/RunPlugin.h"
#include "../log/JobLog.h"
#include "../log/JobsMetrics.h"
#include "../jobs/JobsList.h"

#include "CacheConfig.h"
#include "GMConfig.h"

#include "CoreConfig.h"

namespace ARex {

Arc::Logger CoreConfig::logger(Arc::Logger::getRootLogger(), "CoreConfig");
#define REPORTER_PERIOD "3600";

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

  if (config.xml_cfg) {
    return ParseConfXML(config, config.xml_cfg);
  }
  if (!config.conffile.empty()) {
    Arc::ConfigFile cfile;
    if (!cfile.open(config.conffile)) {
      logger.msg(Arc::ERROR, "Can't read configuration file at %s", config.conffile);
      return false;
    }
    // detect type of file
    Arc::ConfigFile::file_type type = cfile.detect();
    if (type == Arc::ConfigFile::file_XML) {
      Arc::XMLNode xml_cfg;
      if (!xml_cfg.ReadFromStream(cfile)) {
        cfile.close();
        logger.msg(Arc::ERROR, "Can't interpret configuration file %s as XML", config.conffile);
        return false;
      }
      cfile.close();
      // Pick out the A-REX service node
      Arc::XMLNode arex;
      Arc::Config cfg(xml_cfg);
      if (!cfg) return false;

      if (cfg.Name() == "Service") {
        if (cfg.Attribute("name") == "a-rex") {
          cfg.New(arex);
          return ParseConfXML(config, arex);
        }
        return false; // not a-rex service
      }
      if (cfg.Name() == "ArcConfig") {
        // In the case of multiple A-REX services defined, we parse the first one
        for (int i=0;; i++) {
          Arc::XMLNode node = cfg["Chain"];
          node = node["Service"][i];
          if (!node) return false; // no a-rex node found
          if (node.Attribute("name") == "a-rex") {
            node.New(arex);
            break;
          }
        }
        if (!arex) return false;
        return ParseConfXML(config, arex);
      }
      // malformed xml
      return false;
    }
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
  bool ws_enabled = false;
  Arc::ConfigIni cf(cfile);
  cf.SetSectionIndicator(".");
  static const int common_secnum = 0;
  cf.AddSection("common");
  static const int emies_secnum = 1;
  cf.AddSection("arex/ws/emies");
  static const int ws_secnum = 2;
  cf.AddSection("arex/ws");
  static const int jura_secnum = 3;
  cf.AddSection("arex/jura");
  static const int gm_secnum = 4;
  cf.AddSection("arex");
  static const int infosys_secnum = 5;
  cf.AddSection("infosys"); 
  static const int queue_secnum = 6;
  cf.AddSection("queue");
  static const int ssh_secnum = 7;
  cf.AddSection("lrms/ssh");
  static const int lrms_secnum = 8;
  cf.AddSection("lrms");
  static const int cluster_secnum = 9;
  cf.AddSection("cluster");
  static const int perflog_secnum = 10;
  cf.AddSection("common/perflog");
  static const int ganglia_secnum = 11;
  cf.AddSection("monitoring/ganglia");
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
          std::string fname = Arc::ConfigIni::NextArg(rest);  // empty is allowed too
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
          config.support_email_address = Arc::ConfigIni::NextArg(rest);
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
          std::string control_dir = Arc::ConfigIni::NextArg(rest);
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
          config.helper_log = Arc::ConfigIni::NextArg(rest);
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
          std::string scratch = Arc::ConfigIni::NextArg(rest);
          // don't set if already set by shared_scratch
          if (config.scratch_dir.empty()) config.scratch_dir = scratch;
        }
        else if (command == "shared_scratch") {
          std::string scratch = Arc::ConfigIni::NextArg(rest);
          config.scratch_dir = scratch;
        }
        else if (command == "joblog") { // where to write job information
          if (!config.job_log) continue;
          std::string fname = Arc::ConfigIni::NextArg(rest);  // empty is allowed too
          config.job_log->SetOutput(fname.c_str());
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
          std::string str = Arc::ConfigIni::NextArg(rest);
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
          std::string fname = Arc::ConfigIni::NextArg(rest);  // empty is allowed too
          config.jobs_metrics->SetGmetricPath(fname.c_str());
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
          };
        };
      };
      continue;
    };

    if (cf.SectionNum() == ws_secnum) { // arex/ws
      if (cf.SubSection()[0] == '\0') {
        ws_enabled = true;
        if(command == "wsurl") {
           config.arex_endpoint = Arc::ConfigIni::NextArg(rest);
        };
      };
      continue;
    };

    if (cf.SectionNum() == emies_secnum) { // arex/ws/emies
      if (cf.SubSection()[0] == '\0') {
        ws_enabled = true;
        config.enable_emies_interface = true;
        config.enable_arc_interface =  true; // so far
        if (command == "allownew") { // Note: not available in xml
          bool enable = false;
          if (!CheckYesNoCommand(enable, command, rest)) return false;
          config.allow_new = enable;
        }
        else if (command == "allownew_override") { // Note: not available in xml
          config.allow_submit += " " + Arc::ConfigIni::NextArg(rest);
        }
        else if (command == "maxjobdesc") { // Note: not available in xml
          std::string maxjobdesc_s = Arc::ConfigIni::NextArg(rest);
          if (!Arc::stringto(maxjobdesc_s, config.maxjobdesc)) {
            logger.msg(Arc::ERROR, "Wrong number in maxjobdesc command"); return false;
          }
        } else if (command == "allowaccess") {
          while(!rest.empty()) {
            std::string str = Arc::ConfigIni::NextArg(rest);
            if(!str.empty()) {
              config.allowed_groups[""].push_back(str);
            };
          };
        };
      };
      continue;
    };

    if (cf.SectionNum() == jura_secnum) { // arex/jura
      if (cf.SubSection()[0] == '\0') {
        jobreport_publisher = "jura";
        if (command == "logfile") {
          if (config.job_log) {
            std::string logfile = Arc::ConfigIni::NextArg(rest);
            if (logfile.empty()) {
              logger.msg(Arc::ERROR, "Missing file name in jobreport_logfile"); return false;
            };
            job_log_log_is_set = true;
            config.job_log->SetLogFile(logfile.c_str());
          };
        }
        else if (command == "urdelivery_frequency") {
          if (config.job_log) {
            std::string period_s = Arc::ConfigIni::NextArg(rest);
            unsigned int period = 0;
            if (!Arc::stringto(period_s, period)) {
              logger.msg(Arc::ERROR, "Wrong number in urdelivery_frequency: %s", period_s); return false;
            }
            config.job_log->SetPeriod(period);
          }
        }
        else if (command == "x509_host_key") {
          std::string jobreport_key = Arc::ConfigIni::NextArg(rest);
          config.job_log->SetCredentials(jobreport_key, "", "");
        }
        else if (command == "x509_host_cert") {
          std::string jobreport_cert = Arc::ConfigIni::NextArg(rest);
          config.job_log->SetCredentials("", jobreport_cert, "");
        }
        else if (command == "x509_cert_dir") {
          std::string jobreport_cadir = Arc::ConfigIni::NextArg(rest);
          config.job_log->SetCredentials("", "", jobreport_cadir);
        }
      };
      continue;
    };

    if (cf.SectionNum() == infosys_secnum) { // infosys - looking for user name to get share uid
      if (cf.SubSection()[0] == '\0') {
        if (command == "user") {
          config.SetShareID(Arc::User(rest));
        };
      };
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
          std::string str = Arc::ConfigIni::NextArg(rest);
          if (str.empty()) {
            logger.msg(Arc::ERROR, "forcedefaultvoms parameter is empty"); return false;
          }
          if (!config.queues.empty()) {
            std::string queue_name = *(--config.queues.end());
            config.forced_voms[queue_name] = str;
          }
        } else if (command == "authorizedvo") {
          std::string str = Arc::ConfigIni::NextArg(rest);
          if (str.empty()) {
            logger.msg(Arc::ERROR, "authorizedvo parameter is empty"); return false;
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
                config.allowed_groups[queue_name].push_back(str);
              }
            }
          }
        }
      }
      continue;
    }
    if (cf.SectionNum() == cluster_secnum) { // cluster
      if (cf.SubSection()[0] == '\0') {
        if (command == "authorizedvo") {
          std::string str = Arc::ConfigIni::NextArg(rest);
          if (str.empty()) {
            logger.msg(Arc::ERROR, "authorizedvo parameter is empty"); return false;
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

  if (jobreport_publisher.empty()) {
    jobreport_publisher = "jura";
  }
  if(config.job_log) {
    config.job_log->SetLogger(jobreport_publisher.c_str());
    if(!job_log_log_is_set) config.job_log->SetLogFile("/var/log/arc/jura.log");
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

bool CoreConfig::ParseConfXML(GMConfig& config, const Arc::XMLNode& cfg) {

  // Currently we have everything running inside same arched.
  // So we do not need any special treatment for infosys.
  // std::string infosys_user("");

  bool helper_log_is_set = false;
  Arc::XMLNode tmp_node = cfg["endpoint"];
  if (tmp_node) config.headnode = (std::string)tmp_node;
  /*
  jobLogPath

  jobReport
    destination
    expiration
    type
    parameters
    KeyPath
    CertificatePath
    CACertificatesDir
    logfile
  */
  tmp_node = cfg["enableARCInterface"];
  if (tmp_node) {
    if (Arc::lower((std::string)tmp_node) == "yes") {
      config.enable_arc_interface = true;
    } else {
      config.enable_arc_interface = false;
    }
  }
  tmp_node = cfg["enableEMIESInterface"];
  if (tmp_node) {
    if (Arc::lower((std::string)tmp_node) == "yes") {
      config.enable_emies_interface = true;
    } else {
      config.enable_emies_interface = false;
    }
  }
  tmp_node = cfg["jobLogPath"];
  if (tmp_node && config.job_log) {
    std::string fname = tmp_node;
    config.job_log->SetOutput(fname.c_str());
  }
  /*
  loadLimits
    maxJobsTracked
    maxJobsRun
    maxJobsTotal
    maxJobsPerDN
    wakeupPeriod
  */
  tmp_node = cfg["loadLimits"];
  if (tmp_node) {
    if (!Arc::Config::elementtoint(tmp_node, "maxJobsTracked", config.max_jobs)) {
      logger.msg(Arc::ERROR, "Value for maxJobsTracked is incorrect number");
      return false;
    };
    if (!Arc::Config::elementtoint(tmp_node, "maxJobsRun", config.max_jobs_running)) {
      logger.msg(Arc::ERROR, "Value for maxJobsRun is incorrect number");
      return false;
    };
    if (!Arc::Config::elementtoint(tmp_node, "maxJobsTotal", config.max_jobs_total)) {
      logger.msg(Arc::ERROR, "Value for maxJobsTotal is incorrect number");
      return false;
    };
    if (!Arc::Config::elementtoint(tmp_node, "maxJobsPerDN", config.max_jobs_per_dn)) {
      logger.msg(Arc::ERROR, "Value for maxJobsPerDN is incorrect number");
      return false;
    };
    if (!Arc::Config::elementtoint(tmp_node, "wakeupPeriod", config.wakeup_period)) {
      logger.msg(Arc::ERROR, "Value for wakeupPeriod is incorrect number");
      return false;
    };
    if (!Arc::Config::elementtoint(tmp_node, "maxScripts", config.max_scripts)) {
      logger.msg(Arc::ERROR, "Value for maxScripts is incorrect number");
      return false;
    };
  }

  /*
  serviceMail
  */
  tmp_node = cfg["serviceMail"];
  if(tmp_node) {
    config.support_email_address = (std::string)tmp_node;
    if (config.support_email_address.empty()) {
      logger.msg(Arc::ERROR, "serviceMail is empty");
      return false;
    }
  }

  /*
  LRMS
    type
    defaultShare
  */
  tmp_node = cfg["LRMS"];
  if (tmp_node) {
    config.default_lrms = (std::string)(tmp_node["type"]);
    if(config.default_lrms.empty()) {
      logger.msg(Arc::ERROR,"Type in LRMS is missing");
      return false;
    }
    config.default_queue = (std::string)(tmp_node["defaultShare"]);
    CheckLRMSBackends(config.default_lrms);
    config.rte_dir = (std::string)(tmp_node["runtimeDir"]);
    // We only want the scratch path as seen on the front-end
    if (tmp_node["sharedScratch"]) {
      config.scratch_dir = (std::string)(tmp_node["sharedScratch"]);
    } else if (tmp_node["scratchDir"]) {
      config.scratch_dir = (std::string)(tmp_node["scratchDir"]);
    }
  } else {
    logger.msg(Arc::ERROR, "LRMS is missing");
    return false;
  }

  /*
  authPlugin (timeout,onSuccess=PASS,FAIL,LOG,onFailure=FAIL,PASS,LOG,onTimeout=FAIL,PASS,LOG)
    state
    command
  */
  tmp_node = cfg["authPlugin"];
  if (config.cont_plugins) for (; tmp_node; ++tmp_node) {
    std::string state_name = tmp_node["state"];
    if (state_name.empty()) {
      logger.msg(Arc::ERROR, "State name for authPlugin is missing");
      return false;
    }
    std::string command = tmp_node["command"];
    if (state_name.empty()) {
      logger.msg(Arc::ERROR, "Command for authPlugin is missing");
      return false;
    }
    std::string options;
    Arc::XMLNode onode;
    onode = tmp_node.Attribute("timeout");
    if (onode) options += "timeout="+(std::string)onode+',';
    onode = tmp_node.Attribute("onSuccess");
    if (onode) options += "onsuccess="+Arc::lower((std::string)onode)+',';
    onode = tmp_node.Attribute("onFailure");
    if (onode) options += "onfailure="+Arc::lower((std::string)onode)+',';
    onode = tmp_node.Attribute("onTimeout");
    if (onode) options += "ontimeout="+Arc::lower((std::string)onode)+',';
    if (!options.empty()) options = options.substr(0, options.length()-1);
    logger.msg(Arc::DEBUG, "Registering plugin for state %s; options: %s; command: %s",
        state_name, options, command);
    if (!config.cont_plugins->add(state_name.c_str(), options.c_str(), command.c_str())) {
      logger.msg(Arc::ERROR, "Failed to register plugin for state %s", state_name);
      return false;
    }
  }

  /*
  control
    username <- not used any more
    controlDir
    sessionRootDir
    ssh
      remoteHost
    cache
      location
        path
        link
      highWatermark
      lowWatermark
    defaultTTL
    defaultTTR
    maxReruns
    noRootPower
    fixDirectories
    diskSpace <- not used any more
  */

  tmp_node = cfg["control"];
  if (!tmp_node) {
    logger.msg (Arc::ERROR, "Control element must be present");
    return false;
  }
  config.control_dir = (std::string)(tmp_node["controlDir"]);
  if (config.control_dir.empty()) {
    logger.msg(Arc::ERROR, "controlDir is missing");
    return false;
  }
  Arc::XMLNode session_node = tmp_node["sessionRootDir"];
  for (;session_node; ++session_node) {
    std::string session_root = std::string(session_node);
    if (session_root.empty()) {
      logger.msg(Arc::ERROR,"sessionRootDir is missing");
      return false;
    }
    if (session_root == "*") {
      // special value which uses each user's home area
      session_root = "%H/.jobs";
    }
    config.session_roots.push_back(session_root);
    bool session_drain = false;
    if(!Arc::Config::elementtobool(session_node.Attribute("drain"), NULL, session_drain)) {
      logger.msg(Arc::ERROR, "Attribute drain for sessionRootDir is incorrect boolean");
      return false;
    };
    if(!session_drain) config.session_roots_non_draining.push_back(session_root);
  }
  GMConfig::fixdir_t fixdir = GMConfig::fixdir_always;
  const char* fixdir_opts[] = { "yes", "missing", "no", NULL };
  int n;
  if (!Arc::Config::elementtoenum(tmp_node, "fixDirectories", n=(int)fixdir, fixdir_opts)) {
    logger.msg(Arc::ERROR, "The fixDirectories element is incorrect value");
    return false;
  };
  config.fixdir = (GMConfig::fixdir_t)n;
  GMConfig::deleg_db_t deleg_db = GMConfig::deleg_db_sqlite;
  const char* deleg_db_opts[] = { "bdb", "sqlite", NULL };
  if (!Arc::Config::elementtoenum(tmp_node, "delegationDB", n=(int)deleg_db, deleg_db_opts)) {
    logger.msg(Arc::ERROR, "The delegationDB element is incorrect value");
    return false;
  };
  config.deleg_db = (GMConfig::deleg_db_t)n;
  if (!Arc::Config::elementtoint(tmp_node, "maxReruns", config.reruns)) {
    logger.msg(Arc::ERROR, "The maxReruns element is incorrect number");
    return false;
  };
  if (!Arc::Config::elementtobool(tmp_node, "noRootPower", config.strict_session)) {
    logger.msg(Arc::ERROR, "The noRootPower element is incorrect number");
    return false;
  };
  if (!Arc::Config::elementtoint(tmp_node, "defaultTTL", config.keep_finished)) {
    logger.msg(Arc::ERROR, "The defaultTTL element is incorrect number");
    return false;
  };
  if (!Arc::Config::elementtoint(tmp_node, "defaultTTR", config.keep_deleted)) {
    logger.msg(Arc::ERROR, "The defaultTTR element is incorrect number");
    return false;
  };

  // Get cache parameters
  try {
    CacheConfig cache_config(tmp_node);
    config.cache_params = cache_config;
  }
  catch (CacheConfigException& e) {
    logger.msg(Arc::ERROR, "Error with cache configuration: %s", e.what());
    return false;
  }

  // Get ssh parameters
  Arc::XMLNode to_node  = tmp_node["ssh"];
  if (to_node) {
    std::string remote_host = (std::string)to_node["remoteHost"];
    if (remote_host.empty()) {
      logger.msg(Arc::ERROR, "The 'remoteHost' element value is empty - a host name was expected");
      return false;
    }
    config.sshfs_mounts_enabled = true;
  }

  /*
  helperLog
  */
  tmp_node = cfg["helperLog"];
  if(tmp_node) {
    config.helper_log = (std::string)tmp_node;
    helper_log_is_set = true;
  };

  /*
  helperUtility
    username
    command
  */
  std::list<std::string> helpers;
  tmp_node = cfg["helperUtility"];
  for(; tmp_node; ++tmp_node) {
    std::string command = tmp_node["command"];
    if (command.empty()) {
      logger.msg(Arc::ERROR, "Command in helperUtility is missing");
      return false;
    }
    std::string username = tmp_node["username"];
    if (username.empty()) {
      logger.msg(Arc::ERROR, "Username in helperUtility is empty");
      return false;
    }
    if (username != ".") {
      logger.msg(Arc::ERROR, "Only user '.' for helper program is supported");
      return false;
    }
    helpers.push_back(command);
  }
  // End of parsing XML node

  if(!helper_log_is_set) {
    // Set default
    config.helper_log = config.control_dir + "/job.helper.errors";
  }
  // Do substitution of control dir and helpers here now we have all the
  // configuration. These are special because they do not change per-user
  config.Substitute(config.control_dir);
  for (std::list<std::string>::iterator helper = helpers.begin(); helper != helpers.end(); ++helper) {
    config.Substitute(*helper);
    config.helpers.push_back(*helper);
  }

  // Add helper to poll for finished LRMS jobs
  std::string cmd = Arc::ArcLocation::GetDataDir() + "/scan-"+config.default_lrms+"-job";
  cmd = Arc::escape_chars(cmd, " \\", '\\', false);
  if (!config.conffile.empty()) cmd += " --config " + config.conffile;
  cmd += " " + config.control_dir;
  config.helpers.push_back(cmd);

  return true;
}

} // namespace ARex
