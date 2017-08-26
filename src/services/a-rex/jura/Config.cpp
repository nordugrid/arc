#include <arc/ArcConfigIni.h>
#include <arc/StringConv.h>

#include "Config.h"

namespace ArcJura
{

  char const * const Config::default_logfile = "/var/log/arc/jura.log";
  Arc::LogLevel const Config::default_loglevel = Arc::WARNING;
  char const * const Config::default_archiving = "";
  unsigned int const Config::default_archivettl = 0;
  unsigned int const Config::default_urbatchsize = 50;
  unsigned int const Config::default_urdelivery_keepfailed = 30;
  unsigned int const Config::default_urdelivery_frequency = 3600;
  unsigned int const Config::SGAS::default_urbatchsize = 50;
  unsigned int const Config::APEL::default_urbatchsize = 1000;



  Config::Config(char const * configFile): processed(false),
        logfile(default_logfile), loglevel(default_loglevel),
        archiving(default_archiving), archivettl(default_archivettl),
        urbatchsize(default_urbatchsize),
        urdelivery_keepfailed(default_urdelivery_keepfailed),
        urdelivery_frequency(default_urdelivery_frequency)
  {
    std::string name;
    SGAS sgas;
    APEL apel;
    Arc::ConfigIni file(configFile);
    if(!file) return;
    file.SetSectionIndicator(".");
    const int sgas_secnum = 0;
    file.AddSection("grid-manager/jura/sgas");
    const int apel_secnum = 0;
    file.AddSection("grid-manager/jura/apel");
    const int jura_secnum = 2;
    file.AddSection("grid-manager/jura");
    while(true) {
      std::string command;
      std::string rest;
      if(!file.ReadNext(command,rest)) return;
      if(command.empty()) break;
      if (file.SectionNum() == jura_secnum) {
        if (file.SubSection()[0] == '\0') {
          if (command == "logfile") {
            logfile = rest;
          } else if (command == "loglevel") {
            unsigned int loglevel_num;
            if (!Arc::stringto(rest, loglevel_num)) return;
            switch(loglevel) {
              case 0: loglevel = Arc::DEBUG; break;
              case 1: loglevel = Arc::VERBOSE; break;
              case 2: loglevel = Arc::INFO; break;
              case 3: loglevel = Arc::WARNING; break;
              case 4: loglevel = Arc::ERROR; break;
              case 5: loglevel = Arc::FATAL; break;
              default: return;
            }
          } else if (command == "enable_archiving") {
            std::string enable = Arc::lower(Arc::ConfigIni::NextArg(rest));
            if (enable == "yes") {
              archiving = rest;
            } else if (enable == "no") {
              archiving.clear();
            } else {
              return;
            }
          } else if (command == "archivettl") {
            if (!Arc::stringto(rest, archivettl)) return;
          } else if (command == "urbatchsize") {
            if (!Arc::stringto(rest, urbatchsize)) return;
          } else if (command == "vomsless_vo") {
            vomsless_vo = Arc::ConfigIni::NextArg(rest);
            vomsless_issuer = rest;
          } else if (command == "vo_group") {
            vo_group = rest;
          } else if (command == "urdelivery_keepfailed") {
            if (!Arc::stringto(rest, urdelivery_keepfailed)) return;
          } else if (command == "urdelivery_frequency") {
            if (!Arc::stringto(rest, urdelivery_frequency)) return;
          }
        }
      } else if (file.SectionNum() == sgas_secnum) {
        if (file.SectionNew()) {
          name = file.SubSection();
          if(name.empty()) return;
          sgas_entries.push_back(SGAS());
          sgas_entries.back().name = name;
        } else {
          if (command == "targeturl") {
            sgas_entries.back().targeturl = rest;
            if(!sgas_entries.back().targeturl) return;
          } else if (command == "localid_prefix") {
            sgas_entries.back().localid_prefix = rest;
          } else if (command == "vofilter") {
            sgas_entries.back().vofilters.push_back(rest);
          } else if (command == "urbatchsize") {
            if(!Arc::stringto(rest, sgas_entries.back().urbatchsize)) return;
          }
        }
      } else if (file.SectionNum() == apel_secnum) {
        if (file.SectionNew()) {
          name = file.SubSection();
          if(name.empty()) return;
          apel_entries.push_back(APEL());
          apel_entries.back().name = name;
        } else {
          if (command == "targeturl") {
            apel_entries.back().targeturl = rest;
            if(!apel_entries.back().targeturl) return;
          } else if (command == "topic") {
            apel_entries.back().topic = rest;
          } else if (command == "gocdb_name") {
            apel_entries.back().gocdb_name = rest;
          } else if (command == "benchmark_type") {
            apel_entries.back().benchmark_type = rest;
          } else if (command == "benchmark_value") {
            if(!Arc::stringto(rest, apel_entries.back().benchmark_value)) return;
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
            if(!Arc::stringto(rest, apel_entries.back().urbatchsize)) return;
          }
        }
      }
    }
    processed = true;
  }
}

