// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_WSCOMMONPLUGIN_H__
#define __ARC_WSCOMMONPLUGIN_H__

#include <string>

#include <arc/compute/Endpoint.h>
#include <arc/URL.h>

namespace Arc {

  template <class T> class WSCommonPlugin : public T {
  public:
    WSCommonPlugin(PluginArgument* parg) : T(parg) {}

    virtual bool isEndpointNotSupported(const Endpoint& endpoint) const {
      const std::string::size_type pos = endpoint.URLString.find("://");
      if (pos != std::string::npos) {
        const std::string proto = lower(endpoint.URLString.substr(0, pos));
        return ((proto != "http") && (proto != "https"));
      }
      return false;
    }

    static URL CreateURL(std::string service) {
      std::string::size_type pos = service.find("://");
      if (pos == std::string::npos) {
        service = "https://" + service + "/arex";
      } else {
        const std::string proto = lower(service.substr(0,pos));
        if((proto != "http") && (proto != "https")) return URL();
      }
      // Default port other than 443?
      // Default path?

      return service;
    }
  };

} // namespace Arc

#endif // __ARC_WSCOMMONPLUGIN_H__
