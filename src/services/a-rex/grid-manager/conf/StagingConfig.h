#ifndef GM_CONF_STAGING_H_
#define GM_CONF_STAGING_H_

#include <vector>

#include <arc/JobPerfLog.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/ArcConfigFile.h>

#include "GMConfig.h"

namespace ARex {

class DTRGenerator;

/// Represents configuration of DTR data staging
class StagingConfig {
  friend class DTRGenerator;
public:
  /// Load config from [arex/data-staging] section of configuration file
  StagingConfig(const GMConfig& config);

  operator bool() const { return valid; };
  bool operator!() const { return !valid; };

  int get_max_delivery() const { return max_delivery; };
  int get_max_processor() const { return max_processor; };
  int get_max_emergency() const { return max_emergency; };
  int get_max_prepared() const { return max_prepared; };
  unsigned long long int get_min_speed() const { return min_speed; };
  time_t get_min_speed_time() const { return min_speed_time; };
  unsigned long long int get_min_average_speed() const { return min_average_speed; };
  time_t get_max_inactivity_time() const { return max_inactivity_time; };
  int get_max_retries() const { return max_retries; };
  bool get_passive() const { return passive; };
  bool get_httpgetpartial() const { return httpgetpartial; };
  std::string get_preferred_pattern() const { return preferred_pattern; };
  std::vector<Arc::URL> get_delivery_services() const { return delivery_services; };
  unsigned long long int get_remote_size_limit() const { return remote_size_limit; };
  std::string get_share_type() const { return share_type; };
  std::map<std::string, int> get_defined_shares() const { return defined_shares; };
  bool get_use_host_cert_for_remote_delivery() const { return use_host_cert_for_remote_delivery; };
  Arc::LogLevel get_log_level() const { return log_level; };
  std::string get_dtr_log() const { return dtr_log; };
  std::string get_dtr_central_log() const { return dtr_central_log; };
  std::string get_acix_endpoint() const { return acix_endpoint; };

private:
  /// Max transfers in delivery
  int max_delivery;
  /// Max number of pre- and post-processor slots per state
  int max_processor;
  /// Max number of emergency slots
  int max_emergency;
  /// Number of files per share to keep prepared
  int max_prepared;

  /// Minimum speed for transfer over min_speed_time seconds
  unsigned long long int min_speed;
  /// Time over which to calculate min_speed
  time_t min_speed_time;
  /// Minimum average speed for entire transfer
  unsigned long long int min_average_speed;
  /// Maximum time with no transfer activity
  time_t max_inactivity_time;
  /// Max retries for failed transfers that can be retried
  int max_retries;
  /// Whether or not to use passive transfer
  bool passive;
  /// Whether to use partial HTTP GET transfers
  bool httpgetpartial;
  /// Pattern for choosing preferred replicas
  std::string preferred_pattern;

  /// Endpoints of delivery services
  std::vector<Arc::URL> delivery_services;
  /// File size limit (in bytes) below which local transfer should be used
  unsigned long long int remote_size_limit;
  /// Criterion on which to split transfers into shares
  std::string share_type;
  /// The list of shares with defined priorities
  std::map<std::string, int> defined_shares;
  /// Whether to use the host certificate for remote delivery
  bool use_host_cert_for_remote_delivery;
  /// Log level for DTR transfer log in job.id.errors file
  Arc::LogLevel log_level;
  /// where to log DTR state information
  std::string dtr_log;
  /// Log for performance metrics
  Arc::JobPerfLog perf_log;
  /// Central log file for all DTR messages
  std::string dtr_central_log;

  /// ACIX endpoint from which to find locations of cached files
  std::string acix_endpoint;

  /// Validity of configuration
  bool valid;

  /// Logger object
  static Arc::Logger logger;

  /// Read in params from ini config
  bool readStagingConf(Arc::ConfigFile& cfile);
  /// Convert parameter to integer with mimimum value of -1
  bool paramToInt(const std::string& param, int& value);

  StagingConfig();
};

} // namespace ARex

#endif /* GM_CONF_STAGING_H_ */
