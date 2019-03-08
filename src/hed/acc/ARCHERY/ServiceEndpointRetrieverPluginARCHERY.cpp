#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <ldns/ldns.h>

#include "ServiceEndpointRetrieverPluginARCHERY.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginARCHERY::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.ARCHERY");

  bool ServiceEndpointRetrieverPluginARCHERY::isEndpointNotSupported(const Endpoint& endpoint) const {
    // dns:// prefixes URL is supported
    if ( endpoint.URLString.substr(0,6) == "dns://" ) {
        return false;
    }
    // as well as domain names without any URL prefixes
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos;
  }

  EndpointQueryingStatus ServiceEndpointRetrieverPluginARCHERY::Query(const UserConfig& uc,
                                                                      const Endpoint& rEndpoint,
                                                                      std::list<Endpoint>& seList,
                                                                      const EndpointQueryOptions<Endpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);

    if (isEndpointNotSupported(rEndpoint)) {
      return s;
    }
    
    ldns_rdf *domain;
    ldns_resolver *res;
    ldns_pkt *pkt;
    ldns_status dnsstatus;

    std::string domain_url;
    if ( rEndpoint.URLString.substr(0,6) == "dns://" ) {
	    // strip dns:// prefix if specified
        domain_url = rEndpoint.URLString.substr(6);
    } else {
        // or add _archery selector if domain name is provided as is
        domain_url = "_archery." + rEndpoint.URLString;
    }

    // initialize DNS query
 	domain = ldns_dname_new_frm_str(domain_url.c_str());
	if (!domain) {
        logger.msg(DEBUG,"Cannot initialize ARCHERY domain name for query");
        return EndpointQueryingStatus::FAILED;
    }

	// create a new resolver from /etc/resolv.conf
	dnsstatus = ldns_resolver_new_frm_file(&res, NULL);
	if (dnsstatus != LDNS_STATUS_OK) {
        logger.msg(DEBUG,"Cannot create resolver from /etc/resolv.conf");
        return EndpointQueryingStatus::FAILED;
    }

	// query TXT records
	pkt = ldns_resolver_query(res,domain,LDNS_RR_TYPE_TXT,LDNS_RR_CLASS_IN,LDNS_RD);
	ldns_rdf_deep_free(domain);
	if (!pkt) {
        logger.msg(DEBUG,"Cannot query service endpoint TXT records from DNS");
        return EndpointQueryingStatus::FAILED;
    }

	// parse DNS response
	ldns_rr_list *txt;
    ldns_rr *txtrr;
    ldns_rdf *txtdata;

	txt = ldns_pkt_rr_list_by_type(pkt,LDNS_RR_TYPE_TXT,LDNS_SECTION_ANSWER);
	if (!txt) {
        logger.msg(DEBUG,"Cannot parse service endpoint TXT records.");
        return EndpointQueryingStatus::FAILED;
    }

    // get list of resource records and loop over
	ldns_rr_list_sort(txt);
	txtrr = ldns_rr_list_pop_rr(txt);
	while (txtrr != NULL) {
		txtdata = ldns_rr_rdf(txtrr,0);
		// get service endpoint resource record
		std::string se_str(ldns_rdf2str(txtdata));
        // and fetch the next one for next iteration
		txtrr = ldns_rr_list_pop_rr(txt);
        // process record
        // start with trimming whitespaces and qoutes
        const char* trimchars = " \"\t\n";
        se_str.erase(se_str.find_last_not_of(trimchars) + 1);
        se_str.erase(0, se_str.find_first_not_of(trimchars));
        // skip object ID records (not valuable for endpoints retrieving)
        std::string se_str_start = se_str.substr(0,2);
        if ( se_str_start == "o=" ) { continue; }
        // define variables to hold service endpoint properties
        std::string se_url;
        std::vector<std::string> se_types;
        int se_status = 1; //missing status treated as 'active'
        // fetch key=value pairs from endpoint record
        std::size_t space_pos = 0, space_found = 0;
        while (space_found != std::string::npos) {
            space_found = se_str.find_first_of(" ", space_pos);
            std::string se_kv_str = se_str.substr(space_pos, space_found - space_pos);
            space_pos = space_found + 1;
            if (se_kv_str.empty()) {
                continue;
            }
            // check key=value part
            std::string keyeq = se_kv_str.substr(0,2);
            if ( keyeq == "u=" ) {
                se_url = se_kv_str.substr(2);
            } else if ( keyeq == "t=" ) {
                se_types.push_back(se_kv_str.substr(2));
            } else if ( keyeq == "s=" ) {
                if ( se_kv_str.substr(2) != "1" ) {
                    se_status = 0;
                }
            } else {
                logger.msg(WARNING,"Wrong service record field \"%s\" found in the \"%s\"", se_kv_str, se_str );
            }
        }
        // check parsed values
        if ( se_url.empty() ) {
            logger.msg(WARNING,"Malformed ARCHERY record found (endpoint url is not defined): %s", se_str);
            continue;
        }
        if ( se_status ) {
            if ( se_types.empty() ) {
                logger.msg(WARNING,"Malformed ARCHERY record found (endpoint type is not defined): %s", se_str);
                continue;
            } else {
                for(std::vector<std::string>::iterator it = se_types.begin(); it != se_types.end(); ++it) {
                    logger.msg(INFO,"Found service endpoint %s (type %s)", se_url, *it);
                    // register endpoint
                    Endpoint se(se_url);
                    // with corresponding capability
                    if ( *it == "org.nordugrid.archery" || 
                         *it == "archery" ||
                         *it == "archery.group" ||
                         *it == "archery.service" || 
                         *it == "org.nordugrid.ldapegiis" ) {
                        se.Capability.insert("information.discovery.registry");
                        se.InterfaceName = supportedInterfaces.empty()?std::string(""):supportedInterfaces.front();
                    } else {
                        se.Capability.insert("information.discovery.resource");	
                        se.InterfaceName = *it;
                    }
                    seList.push_back(se);
                }
            }
        } else {
            logger.msg(INFO,"Status for service endpoint \"%s\" is set to inactive in ARCHERY. Skipping.", se_url);
        }
	}
	ldns_rr_list_deep_free(txt);
	ldns_pkt_free(pkt);
    ldns_resolver_deep_free(res);

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
