// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_WSCOMMONPLUGIN_H__
#define __ARC_WSCOMMONPLUGIN_H__

#include <string>

#include <arc/compute/Endpoint.h>
#include <arc/URL.h>

namespace Arc {

/// A general wrapping class that adds common functions for all ARC WS-interface plugins
/**
 * A class implementing EntityRetrieverPlugin for WS-based ARC interface 
 * (that inherits TargetInformationRetrieverPlugin and JobListRetrieverPlugin in particular)
 * and making use of the common WS methods implementation needs to inherit WSCommonPlugin 
 * instead with the particular EntityRetrieverPlugin specified as a template argument
 *
 * \ingroup compute
 * \headerfile WSCommonPlugin.h arc/compute/WSCommonPlugin.h
 */
template <class T> class WSCommonPlugin : public T {
public:
  WSCommonPlugin(PluginArgument* parg) : T(parg) {}
  /// Check that endpoint is WS endpoint
  /**
   * This method implements endpoint filtering to be accepted by WS plugin. 
   * To be accepted the Endpoint URL:
   * \li with protocol prefix must match http(s) protocol
   * \li without any protocol prefix URLs are allowed for hostname-based 
   * submission to WS services
   */
  virtual bool isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }
    return false;
  }

  /// Create WS endpoint URL from the provided service endpoint string
  /**
   * This method return endpoint URL using the following logic:
   * \li if provided name is a string with protocol prefix - check that 
   * protocol matches http(s) and return it or empty endpoint otherwise
   * \li if provided name is a string without protocol prefix treat it 
   * as a hostname and construct WS URL assuming default port and /arex 
   * endpoint path
   */
  static URL CreateURL(std::string service) {
    std::string::size_type pos = service.find("://");
    if (pos == std::string::npos) {
      service = "https://" + service + "/arex";
    } else {
      const std::string proto = lower(service.substr(0,pos));
      if((proto != "http") && (proto != "https")) return URL();
    }

    return service;
  }
};

} // namespace Arc

#endif // __ARC_WSCOMMONPLUGIN_H__
