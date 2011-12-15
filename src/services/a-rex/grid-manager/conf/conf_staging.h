#ifndef GM_CONF_STAGING_H_
#define GM_CONF_STAGING_H_

#include <vector>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>

#include "environment.h"

class DTRGenerator;

/// Represents configuration of DTR data staging
class StagingConfig {
  friend class DTRGenerator;
public:
  /// Load config from configuration file. Information from JobsListConfig is
  /// used first, then it is overwritten by parameters in [data-staging] (for
  /// ini style) or new staging parameters in <dataTransfer> (for xml style).
  StagingConfig(const GMEnvironment& env);
  StagingConfig() : valid(false) {};

  operator bool() const { return valid; };
  bool operator!() const { return !valid; };

private:
  /// Max transfers in delivery
  int max_delivery;
  /// Max number of pre- and post-processor slots per state
  int max_processor;
  /// Max number of emergency slots
  int max_emergency;
  /// Number of files per share to keep prepared
  int max_prepared;

  // TODO: the next 8 members are already defined in <dataTransfer> in xml.
  // Need to move them to <DTR> instead. For now they are only processed for
  // ini-style config with [data-staging]

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
  /// Whether or not to use passive transfer (off by default)
  bool passive;
  /// Whether or not to use secure transfer (off by default)
  bool secure;
  /// Pattern for choosing preferred replicas
  std::string preferred_pattern;

  /// Endpoints of delivery services
  std::vector<Arc::URL> delivery_services;
  /// Criterion on which to split transfers into shares
  std::string share_type;
  /// The list of shares with defined priorities
  std::map<std::string, int> defined_shares;
  /// Whether to use the host certificate for remote delivery
  bool use_host_cert_for_remote_delivery;
  /// where to log DTR state information
  std::string dtr_log;

  /// Validity of configuration
  bool valid;

  /// Logger object
  static Arc::Logger logger;

  /// Fill parameters from info in JobsListConfig object
  void fillFromJobsListConfig(const JobsListConfig& jcfg);
  /// Read in params from XML config
  bool readStagingConf(const Arc::XMLNode& cfg);
  /// Read in params from ini config
  bool readStagingConf(std::ifstream& cfile);
  /// Convert parameter to integer with mimimum value of -1
  bool paramToInt(const std::string& param, int& value);

};


#endif /* GM_CONF_STAGING_H_ */
