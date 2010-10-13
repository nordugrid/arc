#include <string>
#include <cstdlib>

#include "Policy.h"

Policy::Policy(const std::string &pstr) : port(-1) {
  std::string::size_type colon = pstr.find(':');
  std::string::size_type slash = pstr.find('/');
  if(slash == std::string::npos) {
    if(colon == std::string::npos) {
      host = pstr;
    }
    else {
      host = pstr.substr(0, colon);
      port = atoi(pstr.substr(colon + 1).c_str());
    }
  }
  else {
    if(colon == std::string::npos || colon > slash) {
      host = pstr.substr(0, slash);
      suffix = pstr.substr(slash + 1);
    }
    else {
      host = pstr.substr(0, colon);
      port = atoi(pstr.substr(colon + 1, slash - colon - 1).c_str());
      suffix = pstr.substr(slash + 1);
    }
  }
  RegComp();
}

Policy::Policy(const Policy& p) : host(p.host),
				  port(p.port),
				  suffix(p.suffix) {
  RegComp();
}

Policy& Policy::operator=(const Policy& p){
  if(!host.empty())
    regfree(&host_rx);
  if(!suffix.empty())
    regfree(&suffix_rx);
  host = p.host;
  port = p.port;
  suffix = p.suffix;
  RegComp();
  return *this;
}

Policy::~Policy() {
  if(!host.empty())
    regfree(&host_rx);
  if(!suffix.empty())
    regfree(&suffix_rx);
}

bool Policy::Check(const std::string &h, int p, const std::string &d) const {
  return ((host.empty() || regexec(&host_rx, h.c_str(), 0, NULL, 0) == 0) &&
	  (port == -1 || port == p) &&
	  (suffix.empty() || regexec(&suffix_rx, d.c_str(), 0, NULL, 0) == 0));
}

void Policy::RegComp() {
  if(!host.empty()) {
    std::string reg = '^' + host + '$';
    std::string::size_type pos = 0;
    while((pos = reg.find('.', pos)) != std::string::npos) {
      reg.insert(pos, "\\");
      pos += 2;
    }
    pos = 0;
    while ((pos = reg.find('*', pos)) != std::string::npos) {
      reg.insert(pos, ".");
      pos += 2;
    }
    pos = 0;
    while ((pos = reg.find('?', pos)) != std::string::npos) {
      reg.replace(pos, 1, ".");
      pos++;
    }
    regcomp(&host_rx, reg.c_str(), REG_ICASE);
  }
  if(!suffix.empty()) {
    std::string reg = '^' + suffix + '$';
    std::string::size_type pos = 0;
    while((pos = reg.find('.', pos)) != std::string::npos) {
      reg.insert(pos, "\\");
      pos += 2;
    }
    pos = 0;
    while ((pos = reg.find('*', pos)) != std::string::npos) {
      reg.insert(pos, ".");
      pos += 2;
    }
    pos = 0;
    while ((pos = reg.find('?', pos)) != std::string::npos) {
      reg.replace(pos, 1, ".");
      pos++;
    }
    pos = 0;
    while ((pos = reg.find(',', pos)) != std::string::npos) {
      std::string::size_type pos1 = reg.find_last_not_of(' ', pos - 1);
      std::string::size_type pos2 = reg.find_first_not_of(' ', pos + 1);
      reg.replace(pos1 + 1, pos2 - pos1 - 1, " *, *");
      pos = pos1 + 6;
    }
    regcomp(&suffix_rx, reg.c_str(), REG_ICASE);
  }
}
