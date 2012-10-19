#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

#include <arc/ArcLocation.h>
#include <arc/FileUtils.h>
#include <arc/XMLNode.h>

#include "conf_file.h"
#include "../run/run_parallel.h"

#include "GMConfig.h"

// Defaults
// default job ttl after finished - 1 week
#define DEFAULT_KEEP_FINISHED (7*24*60*60)
// default job ttr after deleted - 1 month
#define DEFAULT_KEEP_DELETED (30*24*60*60)
// default maximum number of jobs in download/upload
#define DEFAULT_MAX_LOAD (10)
// default maximal allowed amount of reruns
#define DEFAULT_JOB_RERUNS (5)
// default maximum down/upload retries
#define DEFAULT_MAX_RETRIES (10)
// default wake up period for main job loop
#define DEFAULT_WAKE_UP (120)


Arc::Logger GMConfig::logger(Arc::Logger::getRootLogger(), "GMConfig");
static std::string empty_string("");

GMConfig::GMConfig(const std::string& conffile):
  valid(false) {
  SetDefaults();
}

GMConfig::GMConfig(const Arc::XMLNode& node):
  valid(false) {
  SetDefaults();
}

void GMConfig::SetDefaults() {
  conffile_is_temp = false;
  job_log = NULL;
  cont_plugins = NULL;
  cred_plugin = NULL;
  delegations = NULL;
  share_uid = 0;
  keep_finished = DEFAULT_KEEP_FINISHED;
  keep_deleted = DEFAULT_KEEP_DELETED;
  strict_session = false;
  fixdir = fixdir_always;
  reruns = DEFAULT_JOB_RERUNS;
  wakeup_period = DEFAULT_WAKE_UP;
  jobs = NULL;

  use_secure_transfer = false;
  use_passive_transfer = false;
  use_local_transfer = false;
  use_new_data_staging = false;

  max_jobs_running = -1;
  max_jobs_total = -1;
  max_jobs = -1;
  max_jobs_per_dn = -1;

  min_speed = 0;
  min_speed_time = 300;
  min_average_speed = 0;
  max_inactivity_time = 300;

  max_jobs_staging = DEFAULT_MAX_LOAD;
  max_jobs_staging_emergency = 1;
  max_downloads = -1;
  max_staging_share = 0;
  max_retries = DEFAULT_MAX_RETRIES;

  enable_arc_interface = false;
  enable_emies_interface = false;
}

bool GMConfig::Load() {
  // Call CoreConfig (conf_file.h) to fill values in this object
  if (!CoreConfig::ParseConf(*this)) {
    valid = false;
    return false;
  }
  return true;
}

void GMConfig::Print() {
  for(std::vector<std::string>::const_iterator i = session_roots.begin(); i != session_roots.end(); ++i)
    logger.msg(Arc::INFO, "\tSession root dir : %s", *i);
  logger.msg(Arc::INFO, "\tControl dir      : %s", control_dir);
  logger.msg(Arc::INFO, "\tdefault LRMS     : %s", default_lrms);
  logger.msg(Arc::INFO, "\tdefault queue    : %s", default_queue);
  logger.msg(Arc::INFO, "\tdefault ttl      : %u", keep_finished);

  std::vector<std::string> conf_caches = cache_params.getCacheDirs();
  std::vector<std::string> remote_conf_caches = cache_params.getRemoteCacheDirs();
  if(conf_caches.empty()) {
    logger.msg(Arc::INFO,"No valid caches found in configuration, caching is disabled");
    return;
  }
  // list each cache
  for (std::vector<std::string>::iterator i = conf_caches.begin(); i != conf_caches.end(); i++) {
    logger.msg(Arc::INFO, "\tCache            : %s", (*i).substr(0, (*i).find(" ")));
    if ((*i).find(" ") != std::string::npos)
      logger.msg(Arc::INFO, "\tCache link dir   : %s", (*i).substr((*i).find_last_of(" ")+1, (*i).length()-(*i).find_last_of(" ")+1));
  }
  // list each remote cache
  for (std::vector<std::string>::iterator i = remote_conf_caches.begin(); i != remote_conf_caches.end(); i++) {
    logger.msg(Arc::INFO, "\tRemote cache     : %s", (*i).substr(0, (*i).find(" ")));
    if ((*i).find(" ") != std::string::npos)
      logger.msg(Arc::INFO, "\tRemote cache link: %s", (*i).substr((*i).find_last_of(" ")+1, (*i).length()-(*i).find_last_of(" ")+1));
  }
  if (cache_params.cleanCache()) logger.msg(Arc::INFO, "\tCache cleaning enabled");
  else logger.msg(Arc::INFO, "\tCache cleaning disabled");
}

void GMConfig::SetControlDir(const std::string &dir) {
  if (dir.empty()) control_dir = gm_user.Home() + "/.jobstatus";
  else control_dir = dir;
}

void GMConfig::SetSessionRoot(const std::string &dir) {
  session_roots.clear();
  if (dir.empty() || dir == "*") session_roots.push_back(gm_user.Home() + "/.jobs");
  else session_roots.push_back(dir);
}

void GMConfig::SetSessionRoot(const std::vector<std::string> &dirs) {
  session_roots.clear();
  if (dirs.empty()) {
    std::string dir;
    SetSessionRoot(dir);
  } else {
    for (std::vector<std::string>::const_iterator i = dirs.begin(); i != dirs.end(); i++) {
      if (*i == "*") session_roots.push_back(gm_user.Home() + "/.jobs");
      else session_roots.push_back(*i);
    }
  }
}

std::string GMConfig::SessionRoot(const std::string& job_id) const {
  if (session_roots.empty()) return empty_string;
  if (session_roots.size() == 1 || job_id.empty()) return session_roots[0];
  // search for this jobid's session dir
  struct stat st;
  for (std::vector<std::string>::const_iterator i = session_roots.begin(); i != session_roots.end(); i++) {
    std::string sessiondir(*i + '/' + job_id);
    if (stat(sessiondir.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
      return *i;
  }
  return empty_string; // not found
}



static bool fix_directory(const std::string& path, GMConfig::fixdir_t fixmode, mode_t mode, uid_t uid, gid_t gid) {
  if (fixmode == GMConfig::fixdir_never) {
    struct stat st;
    if (!Arc::FileStat(path, &st, true)) return false;
    if (!S_ISDIR(st.st_mode)) return false;
    return true;
  } else if(fixmode == GMConfig::fixdir_missing) {
    struct stat st;
    if (Arc::FileStat(path, &st, true)) {
      if (!S_ISDIR(st.st_mode)) return false;
      return true;
    }
  }
  // GMConfig::fixdir_always
  if (!Arc::DirCreate(path, mode, true)) return false;
  // Only can switch owner if running as root
  if (getuid() == 0) if (chown(path.c_str(), uid, gid) != 0) return false;
  if (chmod(path.c_str(), mode) != 0) return false;
  return true;
}

bool GMConfig::CreateDirectories() {
  bool res = true;
  if (!control_dir.empty()) {
    mode_t mode = 0;
    if (gm_user.get_uid() == 0) {
      // This control dir serves multiple users and running
      // as root (hence really can serve multiple users)
      mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    } else {
      mode = S_IRWXU;
    }
    if (!fix_directory(control_dir, fixdir, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
    // Structure inside control dir is important - *always* create it
    // Directories containing logs and job states may need access from
    // information system, etc. So allowing them to be more open.
    // Delegation is only accessed by service itself.
    if (!fix_directory(control_dir+"/logs", fixdir_always, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
    if (!fix_directory(control_dir+"/accepting", fixdir_always, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
    if (!fix_directory(control_dir+"/restarting", fixdir_always, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
    if (!fix_directory(control_dir+"/processing", fixdir_always, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
    if (!fix_directory(control_dir+"/finished", fixdir_always, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
    std::string deleg_dir = DelegationDir();
    if (!fix_directory(deleg_dir, fixdir_always, S_IRWXU, gm_user.get_uid(), gm_user.get_gid())) res = false;
  }
  for (std::vector<std::string>::iterator i = session_roots.begin(); i != session_roots.end(); ++i) {
    mode_t mode = 0;
    if (gm_user.get_uid() == 0) {
      if (strict_session) {
        // For multiple users creating immediate subdirs using own account
        // dangerous permissions, but there is no other option
        mode = S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX;
      } else {
        // For multiple users not creating immediate subdirs using own account
        mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
      }
    } else {
      // For single user
      mode = S_IRWXU;
    }
    if (!fix_directory(*i, fixdir, mode, gm_user.get_uid(), gm_user.get_gid())) res = false;
  }
  return res;
}

std::string GMConfig::DelegationDir() const {
  std::string deleg_dir = control_dir+"/delegations";
  uid_t u = gm_user.get_uid();
  if (u == 0) return deleg_dir;
  struct passwd pwbuf;
  char buf[4096];
  struct passwd* pw;
  if (::getpwuid_r(u, &pwbuf, buf, sizeof(buf), &pw) == 0) {
    if (pw && pw->pw_name) {
      deleg_dir+=".";
      deleg_dir+=pw->pw_name;
    }
  }
  return deleg_dir;
}

bool GMConfig::Substitute(std::string& param, const Arc::User& user) const {
  std::string::size_type curpos = 0;
  for (;;) {
    if (curpos >= param.length()) break;
    std::string::size_type pos = param.find('%', curpos);
    if (pos == std::string::npos) break;
    pos++; if (pos >= param.length()) break;
    if (param[pos] == '%') { curpos=pos+1; continue; };
    std::string to_put;
    switch (param[pos]) {
      case 'R': to_put = SessionRoot(""); break; // First session dir will be used if there are multiple
      case 'C': to_put = ControlDir(); break;
      case 'U': to_put = UnixName(); break;
      case 'H': to_put = user.Home(); break;
      case 'Q': to_put = DefaultQueue(); break;
      case 'L': to_put = DefaultLRMS(); break;
      case 'u': to_put = Arc::tostring(user.get_uid()); break;
      case 'g': to_put = Arc::tostring(user.get_gid()); break;
      case 'W': to_put = Arc::ArcLocation::Get(); break;
      case 'F': to_put = conffile; break;
      case 'G':
        logger.msg(Arc::ERROR, "Globus location variable substitution is not supported anymore. Please specify path directly.");
        break;
      default: to_put = param.substr(pos-1, 2); break;
    }
    curpos = pos+1+(to_put.length() - 2);
    param.replace(pos-1, 2, to_put);
  }
  return true;
}

void GMConfig::SetShareID(const Arc::User& share_user) {
  share_uid = share_user.get_uid();
  share_gids.clear();
  if (share_uid <= 0) return;
  struct passwd pwd_buf;
  struct passwd* pwd = NULL;
#ifdef _SC_GETPW_R_SIZE_MAX
  int buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (buflen <= 0) buflen = 16384;
#else
  int buflen = 16384;
#endif
  char* buf = (char*)malloc(buflen);
  if (!buf) return;
  if (getpwuid_r(share_uid, &pwd_buf, buf, buflen, &pwd) == 0) {
    if (pwd) {
#ifdef HAVE_GETGROUPLIST
#ifdef _MACOSX
      int groups[100];
#else
      gid_t groups[100];
#endif
      int ngroups = 100;
      if (getgrouplist(pwd->pw_name, pwd->pw_gid, groups, &ngroups) >= 0) {
        for (int n = 0; n<ngroups; ++n) {
          share_gids.push_back((gid_t)(groups[n]));
        }
      }
#endif
      share_gids.push_back(pwd->pw_gid);
    }
  }
  free(buf);
}

bool GMConfig::MatchShareGid(gid_t sgid) const {
  for (std::list<gid_t>::const_iterator i = share_gids.begin(); i != share_gids.end(); ++i) {
    if (sgid == *i) return true;
  }
  return false;
}

bool GMConfig::RunHelpers() {
  bool started = true;
  for (std::list<ExternalHelper>::iterator i = helpers.begin(); i != helpers.end(); ++i) {
    started &= i->run(*this);
  }
  return started;
}

void GMConfig::PrepareToDestroy() {
  // TODO: send signals to helpers and stop started threads
}

GMConfig::ExternalHelper::ExternalHelper(const std::string &cmd) {
  command = cmd;
  proc = NULL;
}

GMConfig::ExternalHelper::~ExternalHelper() {
  if(proc != NULL) {
    delete proc;
    proc=NULL;
  }
}

bool GMConfig::ExternalHelper::run(const GMConfig& config) {
  if (proc != NULL) {
    if (proc->Running()) {
      return true; // it is already/still running
    }
    delete proc;
    proc = NULL;
  }
  // start/restart
  if (command.length() == 0) return true;  // has anything to run ?
  char* args[100]; // up to 98 arguments should be enough
  std::string args_s = command;
  std::string arg_s;
  int n;
  for (n=0; n<99; n++) {
    arg_s = config_next_arg(args_s);
    if (arg_s.length() == 0) break;
    args[n] = strdup(arg_s.c_str());
  }
  args[n] = NULL;
  logger.msg(Arc::VERBOSE, "Starting helper process: %s", command);
  std::string helper_id = "helper.";
  bool started = RunParallel::run(config, Arc::User(), helper_id.c_str(), args, &proc);
  for (n=0; n<99; n++) {
    if (args[n] == NULL) break;
    free(args[n]);
  }
  if (started) return true;
  if (proc && (*proc)) return true;
  if (proc) { delete proc; proc = NULL; };
  logger.msg(Arc::ERROR, "Helper process start failed: %s", command);
  // start failed, doing nothing - maybe in the future
  return false;
}
