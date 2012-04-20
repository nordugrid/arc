#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include <arc/Logger.h>
#include <arc/IString.h>
#include <arc/StringConv.h>
#include <arc/IniConfig.h>
#include <arc/ArcRegex.h>
#include <arc/credential/Credential.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/VOMSAttribute.h>

#define AC_POLICY_PARAM_NAME "ac_policy"

static Arc::Logger logger(Arc::Logger::rootLogger, "arc-vomsac-check");

static void usage(char *pname) {
	logger.msg(Arc::ERROR,"Usage: %s [-N] -P <user proxy> -L <job status file> [-c <configfile>] [-d <loglevel>]",pname);
}

/* ARC classes use '/VO=voname/Group=groupname/subgroupname/Role=role' notation
 * VOMS classes use '/voname/groupname/subgroupname/Role=role/Capability=NULL' notation
 * For configuration compatibility reasons normalization to common format used
 * either for values provided in config or retreived from proxy certificate
 */
std::string normalize_fqan(std::string fqan) {
	// first trim possible spaces and quotes
	std::string nfqan = Arc::trim(fqan," \"");
	// remove 'VO=' and 'Group=' if any
	std::size_t pos = nfqan.find("VO=");
      	if(pos != std::string::npos) nfqan.erase(pos, 3);
	pos = nfqan.find("Group=");
      	if(pos != std::string::npos) nfqan.erase(pos, 6);
	// remove NULL values
	pos = nfqan.find("/Role=NULL");
	if(pos != std::string::npos) nfqan.erase(pos, 10);
	pos = nfqan.find("/Capability=NULL");
	if(pos != std::string::npos) nfqan.erase(pos, 16);
	// return normalized fqan
	return nfqan;	
}

int main(int argc, char *argv[]) {
	int opt;
	int no_ac_success = 0;
	const char *user_proxy_f = NULL;
	const char *job_local_f = NULL;
	const char *config_f = NULL;

	// log
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::getRootLogger().setThreshold(Arc::ERROR);

	// parse options 
	while ((opt = getopt(argc, argv, "NP:L:c:d:")) != -1) {
		switch (opt) {
			case 'N':
				no_ac_success = 1;
				break;
			case 'P':
				user_proxy_f = optarg;
				break;
			case 'L':
				job_local_f = optarg;
				break;
			case 'c':
				config_f = optarg;
				break;
			case 'd':
				Arc::Logger::getRootLogger().setThreshold(
					Arc::old_level_to_level(atoi(optarg))
				);
				break;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}
	

	// check required
	if ( !user_proxy_f ) {
		logger.msg(Arc::ERROR,"User proxy file is required but is not specified");
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if ( !job_local_f ) {
		logger.msg(Arc::ERROR,"Local job status file is required");
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if ( !config_f ) {
		config_f = "/etc/arc.conf";
	}

	// read information about the job used by the A-REX
	// and determine selected queue
	std::string queue;
 	std::ifstream job_local;
	job_local.open(job_local_f, std::ios::in);
	if ( job_local.is_open() ) {
		std::string line;
		while ( ! job_local.eof() ){
			getline(job_local,line);
			if ( ! line.compare(0,6,"queue=") ) {
				queue = line.substr(6);
				logger.msg(Arc::INFO,"Making the decision for the queue %s",queue);
				break;
			}
		}
	} else {
		logger.msg(Arc::ERROR,"Can not read information from the local job status file");
		return EXIT_FAILURE;
	}
	job_local.close();

	// Parse INI configuraion file
	Arc::IniConfig cfg(config_f);
	if ( ! cfg ) {
		logger.msg(Arc::ERROR,"Can not parse the configuration file %s",config_f);
		return EXIT_FAILURE;
	}
	
	// get queue ac_policy 
	// first try [queue/name] block directly, then search for 'id' or 'name' field in [queue] blocks
	std::string qqueue = '"' + queue + '"';
	Arc::XMLNode qparams = cfg["queue/" + queue];
	if ( ! (bool)qparams ) {
		for(Arc::XMLNode qnode = cfg["queue"];(bool)qnode;++qnode) {
			if ( (std::string)qnode["id"] == qqueue || (std::string)qnode["name"] == qqueue ) {
				qparams = qnode;
				break;
			}
		}
	}
	if ( ! (bool)qparams) {
		logger.msg(Arc::ERROR,"Can not find queue '%s' in the configuration file",queue);
		return EXIT_FAILURE;
	}

	// create match regexes from ac_policy provided
	std::vector< std::pair<Arc::RegularExpression,bool> > access_policies;
	for ( Arc::XMLNode pnode = qparams[AC_POLICY_PARAM_NAME];(bool)pnode;++pnode) {
		std::string acp = (std::string)pnode;
		std::size_t pos = acp.find("VOMS:");
		if ( pos != std::string::npos ) {
			// determine positive/negative match
			bool pmatch = true;
			if ( pos ) { 
				char pnflag = acp[pos-1];
				if ( pnflag == '-' || pnflag == '!' ) pmatch = false;
			}
			// normalize rest part of the string 
			std::string regex = ".*" + normalize_fqan(acp.substr(pos + 5)) + ".*";
			// save (regex,pmatch) pairs to access_policies vector
			std::pair <Arc::RegularExpression,bool> match_regex(Arc::RegularExpression(regex),pmatch);
			access_policies.push_back(match_regex);
		}
	}
	if ( access_policies.empty() ) {
		logger.msg(Arc::INFO,"No access policy to check, returning success");
		return EXIT_SUCCESS;
	}

	struct stat statFInfo;

	// CA Cert directory required to work with proxy
	std::string ca_dir = (std::string)cfg["common"]["x509_cert_dir"];
	if (ca_dir.empty()) {
		ca_dir = "/etc/grid-security/certificates";
	} else {
		ca_dir = Arc::trim(ca_dir,"\"");
	}
	if ( stat(ca_dir.c_str(),&statFInfo) ) {
		logger.msg(Arc::ERROR,"CA certificates directory %s does not exist", ca_dir);
		return EXIT_FAILURE;
	}

	// VOMS directory required to verify VOMS ACs
	std::string voms_dir = (std::string)cfg["common"]["x509_voms_dir"];
	if (voms_dir.empty()) {
		voms_dir = "/etc/grid-security/vomsdir";
	} else {
		voms_dir = Arc::trim(voms_dir,"\"");
	}
        // Or maybe not _required_
	//if ( stat(voms_dir.c_str(),&statFInfo) ) {
	//	logger.msg(Arc::ERROR,"VOMS directory %s does not exist", voms_dir);
	//	return EXIT_FAILURE;
	//}

	// construct ARC credentials object
	Arc::Credential holder(user_proxy_f, "", ca_dir, "");
	if (! holder.GetVerification()) {
		logger.msg(Arc::ERROR,"User proxy certificate is not valid");
		return EXIT_FAILURE;
	}

	// get VOMS AC from proxy certificate
	logger.msg(Arc::DEBUG,"Getting VOMS AC for: %s", holder.GetDN());
	std::vector<Arc::VOMSACInfo> voms_attributes;
	Arc::VOMSTrustList vomscert_trust_dn;
	vomscert_trust_dn.AddRegex(".*");

	if ( ! Arc::parseVOMSAC(holder, ca_dir, "", voms_dir, vomscert_trust_dn, voms_attributes, true, true) ) {
		// logger.msg(Arc::WARNING,"Error parsing VOMS AC");
		if ( no_ac_success ) return EXIT_SUCCESS;
		return EXIT_FAILURE;
	}

	// loop over access_policies
	for (std::vector<std::pair<Arc::RegularExpression,bool> >::iterator iP = access_policies.begin();
		iP != access_policies.end(); iP++) {
		logger.msg(Arc::VERBOSE,"Checking a match for '%s'",(iP->first).getPattern());
		// for every VOMS AC provided
		for (std::vector<Arc::VOMSACInfo>::iterator iAC = voms_attributes.begin(); iAC != voms_attributes.end(); iAC++) {
			// check every attribure specified to match specified policy
			for (int acnt = 0; acnt < iAC->attributes.size(); acnt++ ) {
				std::string fqan = normalize_fqan(iAC->attributes[acnt]);
				if ( (iP->first).match(fqan) ) {
					logger.msg(Arc::DEBUG,"FQAN '%s' IS a match to '%s'",fqan,(iP->first).getPattern());
					// if positive match return success
					if ( iP->second ) return EXIT_SUCCESS;
					// prohibit execution on negative match
					logger.msg(Arc::ERROR,"Queue '%s' usage is prohibited to FQAN '%s' by the site access policy",
						queue, fqan);
					return EXIT_FAILURE;
				} else {
					logger.msg(Arc::DEBUG,"FQAN '%s' IS NOT a match to '%s'",fqan,(iP->first).getPattern());
				}
			}
		}
	}

	logger.msg(Arc::ERROR,"Queue '%s' usage with provided FQANs is prohibited by the site access policy",queue);
	return EXIT_FAILURE;
}

