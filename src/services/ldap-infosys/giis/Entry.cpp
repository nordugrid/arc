#include "Entry.h"

#include <time.h>

#include <sstream>
#include <cstdlib>

Entry::Entry(const std::list<std::string>& query) : port(-1),
						    sizelimit(0),
						    timelimit(30),
						    cachetime(0) {
  for(std::list<std::string>::const_iterator it = query.begin();
      it != query.end(); it++) {
    if(it->substr(0, 4) == "dn: ")
      dn = it->substr(4);
    if(it->substr(0, 16) == "Mds-Service-hn: ")
      hn = it->substr(16);
    if(it->substr(0, 18) == "Mds-Service-port: ")
      port = atoi(it->substr(18).c_str());
    if(it->substr(0, 25) == "Mds-Service-Ldap-suffix: ")
      suffix = it->substr(25);
    if(it->substr(0, 27) == "Mds-Service-Ldap-cachettl: ")
      cachetime = atoi(it->substr(27).c_str());
    if(it->substr(0, 26) == "Mds-Service-Ldap-timeout: ")
      timelimit = atoi(it->substr(26).c_str());
    if(it->substr(0, 28) == "Mds-Service-Ldap-sizelimit: ")
      sizelimit = atoi(it->substr(28).c_str());
    if(it->substr(0, 15) == "Mds-validfrom: ")
      validfrom = it->substr(15);
    if(it->substr(0, 13) == "Mds-validto: ")
      validto = it->substr(13);
    if(it->substr(0, 12) == "Mds-keepto: ")
      keepto = it->substr(12);
  }
  std::string::size_type spos = suffix.find(',');
  std::string::size_type dpos = dn.find(',');
  if(spos != std::string::npos && dpos != std::string::npos)
    dn = suffix.substr(0, spos) + dn.substr(dpos);
  else
    dn.clear();
}

Entry::~Entry() {}

const std::string& Entry::Host() const {
  return hn;
}

int Entry::Port() const {
  return port;
}

const std::string& Entry::Suffix() const {
  return suffix;
}

std::string Entry::SearchEntry() const {
  std::ostringstream ss;
  ss << "dn: " << dn << std::endl;
  ss << "objectClass: Mds" << std::endl;
  ss << "objectClass: MdsVoOp" << std::endl;
  ss << "objectClass: MdsService" << std::endl;
  ss << "objectClass: MdsServiceLdap" << std::endl;
  ss << "Mds-Service-type: ldap" << std::endl;
  ss << "Mds-Service-hn: " << hn << std::endl;
  ss << "Mds-Service-port: " << port << std::endl;
  ss << "Mds-Service-Ldap-suffix: " << suffix << std::endl;
  ss << "Mds-Service-Ldap-sizelimit: " << sizelimit << std::endl;
  ss << "Mds-Service-Ldap-timeout: " << timelimit << std::endl;
  ss << "Mds-Service-Ldap-cachettl: " << cachetime << std::endl;
  ss << "Mds-Bind-Method-servers: ANONYM-ONLY" << std::endl;
  time_t t1 = time(NULL);
  struct tm t2;
  char buf[16];
  strftime (buf, 16, "%Y%m%d%H%M%SZ", gmtime_r(&t1, &t2));
  if(keepto < buf)
    ss << "Mds-Reg-status: PURGED" << std::endl;
  else if(validto < buf)
    ss << "Mds-Reg-status: INVALID" << std::endl;
  else
    ss << "Mds-Reg-status: VALID" << std::endl;
  ss << std::endl;
  return ss.str();
}

bool Entry::operator!() const {
  return (dn.empty() || hn.empty() || (port == -1) || suffix.empty());
}
