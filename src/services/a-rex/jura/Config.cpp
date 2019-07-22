#include <arc/ArcConfigIni.h>
#include <arc/StringConv.h>

#include "Config.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"JURA.Config");

namespace ArcJura
{

  char const * const Config::default_logfile = "/var/log/arc/jura.log";
  Arc::LogLevel const Config::default_loglevel = Arc::INFO;
  char const * const Config::default_archivedir = "/var/log/arc/jura/archive";
  unsigned int const Config::default_archivettl = 0; // forever
  unsigned int const Config::default_urdelivery_keepfailed = 30;
  unsigned int const Config::default_urdelivery_frequency = 3600;
  unsigned int const Config::SGAS::default_urbatchsize = 50;
  unsigned int const Config::APEL::default_urbatchsize = 1000;

  Config::Config(char const * configFile): processed(false),
        logfile(default_logfile), loglevel(default_loglevel),
        archiving(false), archivedir(default_archivedir), archivettl(default_archivettl),
        urdelivery_keepfailed(default_urdelivery_keepfailed),
        urdelivery_frequency(default_urdelivery_frequency)
  {
    if(!configFile) return; // only defaults
    std::string name;
    SGAS sgas;
    APEL apel;
    Arc::ConfigIni file(configFile);
    if(!file) return;
    file.SetSectionIndicator(".");
    const int common_secnum = 0;
    file.AddSection("common");
    const int sgas_secnum = 1;
    file.AddSection("arex/jura/sgas");
    const int apel_secnum = 2;
    file.AddSection("arex/jura/apel");
    const int archiving_secnum = 3;
    file.AddSection("arex/jura/archiving");
    const int jura_secnum = 4;
    file.AddSection("arex/jura");
    const int arex_secnum = 5;
    file.AddSection("arex");
    while(true) {
      std::string command;
      std::string rest;
      if(!file.ReadNext(command,rest)) return;
      if(command.empty()) break;
      if (file.SectionNum() == common_secnum) {
        if (file.SubSection()[0] == '\0') {
          if (command == "x509_host_key") {
            host_key = rest;
            Arc::SetEnv("X509_USER_KEY", host_key);
          } else if (command == "x509_host_cert") {
            host_cert = rest;
            Arc::SetEnv("X509_USER_CERT", host_cert);
          } else if (command == "x509_cert_dir") {
            ca_cert_dir = rest;
            Arc::SetEnv("X509_CERT_DIR", host_cert);
          }
        }
      } else if (file.SectionNum() == arex_secnum) {
        if (file.SubSection()[0] == '\0') {
          if (command == "controldir") {
            controldir = rest;
          }
        }
      } else if (file.SectionNum() == jura_secnum) {
        if (file.SubSection()[0] == '\0') {
          if (command == "logfile") {
            logfile = rest;
          } else if (command == "loglevel") {
            unsigned int loglevel_num;
            if (!Arc::stringto(rest, loglevel_num)) {
              logger.msg(Arc::ERROR, "Wrong loglevel (%s) config value given!", rest);
              return;
            }
            loglevel = Arc::old_level_to_level(loglevel_num);
          } else if (command == "vomsless_vo") {
            vomsless_vo = Arc::ConfigIni::NextArg(rest,'#');
            vomsless_issuer = rest;
          } else if (command == "vo_group") {
            vo_group = rest;
          } else if (command == "urdelivery_keepfailed") {
            if (!Arc::stringto(rest, urdelivery_keepfailed)) {
              logger.msg(Arc::ERROR, "Wrong urdelivery_keepfailed (%s) config value given!", rest);
              return;
            }
          } else if (command == "urdelivery_frequency") {
            if (!Arc::stringto(rest, urdelivery_frequency)) {
              logger.msg(Arc::ERROR, "Wrong urdelivery_frequency (%s) config value given!", rest);
              return;
            }
          } else if (command == "x509_host_key") {
            host_key = rest;
            Arc::SetEnv("X509_USER_KEY", host_key, true);
          } else if (command == "x509_host_cert") {
            host_cert = rest;
            Arc::SetEnv("X509_USER_CERT", host_cert, true);
          } else if (command == "x509_cert_dir") {
            ca_cert_dir = rest;
            Arc::SetEnv("X509_CERT_DIR", host_cert, true);
          }
        }
      } else if (file.SectionNum() == archiving_secnum) {
        if (file.SubSection()[0] == '\0') {
          archiving = true;
          if (command == "archivedir") {
            archivedir = rest;
          }
        }
      } else if (file.SectionNum() == sgas_secnum) {
        if (file.SectionNew()) {
          name = file.SectionIdentifier();
          if(name.empty()) {
            logger.msg(Arc::ERROR, "Name part is missing by SGAS section!");
            return;
          }
          sgas_entries.push_back(SGAS());
          sgas_entries.back().name = name;
        } else {
          if (command == "targeturl") {
            sgas_entries.back().targeturl = rest;
            if(!sgas_entries.back().targeturl) {
              logger.msg(Arc::ERROR, "Targeturl config value is missing by SGAS!");
              return;
            }
          } else if (command == "localid_prefix") {
            sgas_entries.back().localid_prefix = rest;
          } else if (command == "legacy_fallback") {
            if ( rest != "yes" ) {
                logger.msg(Arc::INFO, "Block %s is not marked for legacy fallback processing. Skipping.", sgas_entries.back().name);
                sgas_entries.pop_back();
            }
          } else if (command == "vofilter") {
            sgas_entries.back().vofilters.push_back(rest);
          } else if (command == "urbatchsize") {
            if(!Arc::stringto(rest, sgas_entries.back().urbatchsize) ||
                sgas_entries.back().urbatchsize == 4294967295 ) {  // it is -1
              logger.msg(Arc::ERROR, "Wrong urbatchsize (%s) config value given by SGAS!", rest);
              return;
            }
          }
        }
      } else if (file.SectionNum() == apel_secnum) {
        if (file.SectionNew()) {
          name = file.SectionIdentifier();
          if(name.empty()) {
            logger.msg(Arc::ERROR, "Name part is missing by APEL section!");
            return;
          }
          apel_entries.push_back(APEL());
          apel_entries.back().name = name;
        } else {
          if (command == "targeturl") {
            apel_entries.back().targeturl = rest;
            if(!apel_entries.back().targeturl) {
              logger.msg(Arc::ERROR, "Targeturl config value is missing by APEL!");
              return;
            }
          } else if (command == "legacy_fallback") {
            if ( rest != "yes" ) {
                logger.msg(Arc::INFO, "Block %s is not marked for legacy fallback processing. Skipping.", apel_entries.back().name);
                apel_entries.pop_back();
            }
          } else if (command == "topic") {
            apel_entries.back().topic = rest;
          } else if (command == "gocdb_name") {
            apel_entries.back().gocdb_name = rest;
          } else if (command == "benchmark_type") {
            apel_entries.back().benchmark_type = rest;
          } else if (command == "benchmark_value") {
            if(!Arc::stringto(rest, apel_entries.back().benchmark_value)) {
              logger.msg(Arc::ERROR, "Wrong benchmark_value (%s) config value given by APEL!", rest);
              return;
            }
          } else if (command == "benchmark_description") {
            apel_entries.back().benchmark_description = rest;
          } else if (command == "use_ssl") {
            rest = Arc::lower(rest);
            if (rest == "yes") {
              apel_entries.back().use_ssl = true;
            } else if (rest == "no") {
              apel_entries.back().use_ssl = false;
            } else {
              return;
            }
          } else if (command == "vofilter") {
            apel_entries.back().vofilters.push_back(rest);
          } else if (command == "urbatchsize") {
            if(!Arc::stringto(rest, apel_entries.back().urbatchsize) ||
                apel_entries.back().urbatchsize == 4294967295 ) {  // it is -1) {
              logger.msg(Arc::ERROR, "Wrong urbatchsize (%s) config value given by APEL!", rest);
              return;
            }
          }
        }
      }
    }
    processed = true;
  }
  
  Config::SGAS::SGAS(const SGAS &_conf) {
    name=_conf.name;
    targeturl=_conf.targeturl;
    localid_prefix=_conf.localid_prefix;
    vofilters=_conf.vofilters;
    urbatchsize=_conf.urbatchsize;
  }
  
  Config::APEL::APEL(const APEL &_conf) {
    name=_conf.name;
    targeturl=_conf.targeturl;
    topic=_conf.topic;
    gocdb_name=_conf.gocdb_name;
    benchmark_type=_conf.benchmark_type;
    benchmark_value=_conf.benchmark_value;
    benchmark_description=_conf.benchmark_description; 
    use_ssl=_conf.use_ssl;
    vofilters=_conf.vofilters;
    urbatchsize=_conf.urbatchsize;
  }
}

