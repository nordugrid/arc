#ifndef _JURA_CONFIG_H
#define _JURA_CONFIG_H

#include <cmath>
#include <string>
#include <vector>
#include <arc/URL.h>
#include <arc/Logger.h>

namespace ArcJura
{
  class Config
  {
  public:
    Config(char const * configFile = NULL);
    operator bool() { return processed; }
    bool operator!() { return !processed; }

    std::string const & getControlDir() const { return controldir; }
    std::string const & getLogfile() const { return logfile; }
    Arc::LogLevel getLoglevel() const { return loglevel; }
    bool getArchiving() const { return archiving; }
    std::string const & getArchiveDir() const { return archivedir; }
    unsigned int getArchivettl() const { return archivettl; }
    std::string const & getVOMSlessVO() const { return vomsless_vo; }
    std::string const & getVOMSlessIssuer() const { return vomsless_issuer; }
    std::string const & getVOGroup() const { return vo_group; }
    unsigned int getUrdeliveryKeepfaild() const { return urdelivery_keepfailed; }
    unsigned int getUrdeliveryFrequency() const { return urdelivery_frequency; }
    std::string const & getHostKey() const { return host_key; }
    std::string const & getHostCert() const { return host_cert; }
    std::string const & getCADir() const { return ca_cert_dir; }

    class ACCOUNTING {};
    
    class SGAS : public ACCOUNTING {
    public:
      static unsigned int const default_urbatchsize;
      SGAS(): urbatchsize(default_urbatchsize) {}
      SGAS(const SGAS &_conf);
      std::string name;
      Arc::URL targeturl;
      std::string localid_prefix;
      std::vector<std::string> vofilters;
      unsigned int urbatchsize;
    };

    std::vector<SGAS> const getSGAS() const { return sgas_entries; }

    class APEL : public ACCOUNTING {
    public:
      static unsigned int const default_urbatchsize;
      APEL(): benchmark_value(NAN), use_ssl(false), urbatchsize(default_urbatchsize) {}
      APEL(const APEL &_conf);
      std::string name;
      Arc::URL targeturl;
      std::string topic;
      std::string gocdb_name;
      std::string benchmark_type;
      float  benchmark_value;
      std::string benchmark_description; 
      bool use_ssl;
      std::vector<std::string> vofilters;
      unsigned int urbatchsize;
    };

    std::vector<APEL> const getAPEL() const { return apel_entries; }

  private:
    bool processed;

    static char const * const default_logfile;
    static Arc::LogLevel const default_loglevel;
    static char const * const default_archivedir;
    static unsigned int const default_archivettl;
    static unsigned int const default_urdelivery_keepfailed;
    static unsigned int const default_urdelivery_frequency;

    std::string controldir;
    std::string logfile;
    Arc::LogLevel loglevel;
    bool archiving;
    std::string archivedir;
    unsigned int archivettl;
    std::string vomsless_vo;
    std::string vomsless_issuer;
    std::string vo_group;
    unsigned int urdelivery_keepfailed;
    unsigned int urdelivery_frequency;
    std::string host_key;
    std::string host_cert;
    std::string ca_cert_dir;
    std::vector<SGAS> sgas_entries;
    std::vector<APEL> apel_entries;

  };

}

#endif
