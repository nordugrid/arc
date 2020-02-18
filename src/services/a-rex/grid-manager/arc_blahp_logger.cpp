#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/FileUtils.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/credential/Credential.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/VOMSAttribute.h>


static Arc::Logger logger(Arc::Logger::rootLogger, "arc-blahp-logger");

static void usage(char *pname) {
    std::cerr << "Usage: " << pname << " -I <jobID> -U <user> -P <proxy file> -L <job status file> [-c <ceid prefix>] [-p <log prefix> ] [-d <log level>] [ -i ]\n";
    std::cerr << "\n  Where\n   -i should be set to ignore failed jobs. Default is to publish them.\n";
}

int main(int argc, char *argv[]) {
    int opt;
    const char *user_proxy_f = NULL;
    const char *job_local_f = NULL;
    const char *jobid_s = NULL;
    const char *user_s = NULL;
    const char *ceid_s = NULL;
    std::string logprefix = "/var/log/arc/accounting/blahp.log";
    bool ignore_failed = false;

    // log
    Arc::LogLevel debuglevel = Arc::ERROR;
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(debuglevel);

    // Parse command line options
    while ((opt = getopt(argc, argv, "iI:U:P:L:c:p:d:")) != -1) {
        switch (opt) {
            case 'i':
                ignore_failed = true;
                break;
            case 'I':
                jobid_s = optarg;
                break;
            case 'U':
                user_s = optarg;
                break;
            case 'P':
                user_proxy_f = optarg;
                break;
            case 'L':
                job_local_f = optarg;
                break;
            case 'c':
                ceid_s = optarg;
                break;
            case 'p':
                logprefix = std::string(optarg);
                break;
            case 'd':
                debuglevel = Arc::old_level_to_level(atoi(optarg));
                Arc::Logger::getRootLogger().setThreshold(debuglevel);
                break;
            default:
                logger.msg(Arc::ERROR,"Unknown option %s", opt);
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if ( !jobid_s ) {
        logger.msg(Arc::ERROR,"Job ID argument is required.");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if ( !user_proxy_f ) {
        logger.msg(Arc::ERROR,"Path to user's proxy file should be specified.");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if ( !user_s ) {
        logger.msg(Arc::ERROR,"User name should be specified.");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if ( !job_local_f ) {
        logger.msg(Arc::ERROR,"Path to .local job status file is required.");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Get or generate ceID prefix
    std::string ceid;
    if ( !ceid_s ) {
        logger.msg(Arc::DEBUG,"Generating ceID prefix from hostname automatically");
        char host[256];
            if (gethostname(host, sizeof(host)) != 0) {
                logger.msg(Arc::ERROR, "Cannot determine hostname from gethostname() to generate ceID automatically.");
            return EXIT_FAILURE;
        } else {
            host[sizeof(host)-1] = 0;
            ceid = std::string(host) + ":2811/nordugrid-torque";
        }
    } else {
        ceid = std::string(ceid_s);
    }
    logger.msg(Arc::DEBUG,"ceID prefix is set to %s", ceid);

    // Get the current timestamp for log and logsuffix
    Arc::SetEnv("TZ","UTC");
    tzset();
    Arc::Time exectime;
    std::string timestamp = exectime.str(Arc::UserTime);
    std::string logsuffix = exectime.str(Arc::MDSTime).substr(0,8);
    logger.msg(Arc::DEBUG,"Getting currect timestamp for BLAH parser log: %s", timestamp);

    // Parse .local file to get required information
    std::string globalid;
    std::string localid;
    std::string queue;
    std::string subject;
    std::string interface;
    std::string headnode;
    logger.msg(Arc::DEBUG,"Parsing .local file to obtain job-specific identifiers and info");

    std::ifstream job_local;
    job_local.open(job_local_f, std::ios::in);
    if ( job_local.is_open() ) {
        std::string line;
        while ( job_local.good() ) {
            getline(job_local,line);
            if ( ! line.compare(0,9,"globalid=") ) {
                globalid = line.substr(9);
                logger.msg(Arc::DEBUG,"globalid is set to %s", globalid);
            } else if ( ! line.compare(0,9,"headnode=") ) {
                headnode = line.substr(9);
                logger.msg(Arc::DEBUG,"headnode is set to %s", headnode);
            } else if ( ! line.compare(0,10,"interface=") ) {
                interface = line.substr(10);
                logger.msg(Arc::DEBUG,"interface is set to %s", interface);
            } else if ( ! line.compare(0,8,"localid=") ) {
                localid = line.substr(8);
                if ( localid.empty() ) {
                    logger.msg(Arc::ERROR,"There is no local LRMS ID. Message will not be written to BLAH log.");
                    return EXIT_FAILURE;
                }
                logger.msg(Arc::DEBUG,"localid is set to %s", localid);
            } else if ( ! line.compare(0,6,"queue=") ) {
                queue = line.substr(6);
                logger.msg(Arc::DEBUG,"queue name is set to %s", queue);
            } else if ( ! line.compare(0,8,"subject=") ) {
                subject = line.substr(8);
                logger.msg(Arc::DEBUG,"owner subject is set to %s", subject);
            } else if ( (! line.compare(0,12,"failedstate=")) && ignore_failed ) {
                logger.msg(Arc::ERROR,"Job did not finished successfully. Message will not be written to BLAH log.");
                return EXIT_FAILURE;
            } else if ( ! line.compare(0,10,"starttime=") ) {
                //need to convert timestamp into a blah compatible format
                //blah / apel use the timestamp to determine job eligibility to accounting, as job IDs can (?) loop
                //it is more deterministic to use the job start date as the timestamp than "now()" which will cause issues in case of processing delays
                Arc::Time job_timestamp(line.substr(10)) ;
                timestamp = job_timestamp.str(Arc::UserTime);
                logger.msg(Arc::DEBUG,"Job timestamp successfully parsed as %s", timestamp);
            }
        }
    } else {
        logger.msg(Arc::ERROR,"Can not read information from the local job status file");
        return EXIT_FAILURE;
    }

    // Just in case subject escape
    subject = Arc::escape_chars(subject, "\"\\", '\\', false);

    // Construct clientID depend on submission interface
    std::string clientid;
    if ( interface == "org.nordugrid.gridftpjob" ) {
        clientid = globalid;
    } else if ( interface == "org.ogf.glue.emies.activitycreation" ) {
        clientid = headnode + "/" + globalid;
    } else if ( interface == "org.nordugrid.arcrest" ) {
        clientid = headnode + "/" + globalid;
    } else {
        logger.msg(Arc::ERROR,"Unsupported submission interface %s. Seems arc-blahp-logger need to be updated accordingly. Please submit the bug to bugzilla.");
        return EXIT_FAILURE;
    }

    // Get FQANs information from user's proxy
    // P.S. validity check is not enforced, proxy can be even expired long time before job finished
    Arc::Credential holder(user_proxy_f, "", "", "");
    std::vector<Arc::VOMSACInfo> voms_attributes;
    Arc::VOMSTrustList vomscert_trust_dn;

    logger.msg(Arc::DEBUG, "Parsing VOMS AC to get FQANs information");
    // suppress expired 'ERROR' from Arc.Credential output
    if ( debuglevel == Arc::ERROR ) Arc::Logger::getRootLogger().setThreshold(Arc::FATAL);
    Arc::parseVOMSAC(holder, "", "", "", vomscert_trust_dn, voms_attributes, false, true);
    Arc::Logger::getRootLogger().setThreshold(debuglevel);

    std::string fqans_logentry;
    std::string fqan;
    std::size_t pos;
    if(voms_attributes.size() > 0) {
        for (std::vector<Arc::VOMSACInfo>::iterator iAC = voms_attributes.begin(); iAC != voms_attributes.end(); iAC++) {
            for (unsigned int acnt = 0; acnt < iAC->attributes.size(); acnt++ ) {
                fqan = iAC->attributes[acnt];
                logger.msg(Arc::DEBUG, "Found VOMS AC attribute: %s", fqan);

                std::list<std::string> elements;
                Arc::tokenize(fqan, elements, "/");
                if ( elements.size() == 0 ) {
                    logger.msg(Arc::DEBUG, "Malformed VOMS AC attribute %s", fqan);
                    continue;
                }

                if (elements.front().rfind("voname=", 0) == 0) {
                    elements.pop_front(); // crop voname=
                    if ( ! elements.empty() ) elements.pop_front(); // crop hostname=
                    if ( ! elements.empty() ) {
                        logger.msg(Arc::DEBUG, "VOMS AC attribute is a tag");
                        fqan = "";
                        while (! elements.empty () ) {
                            fqan.append("/").append(elements.front());
                            elements.pop_front();
                        }
                    } else {
                        logger.msg(Arc::DEBUG, "Skipping policyAuthority VOMS AC attribute");
                        continue;
                    }
                } else {
                    logger.msg(Arc::DEBUG, "VOMS AC attribute is the FQAN");
                    pos = fqan.find("/Role=");
                    if ( pos == std::string::npos ) fqan = fqan + "/Role=NULL";
                }
                fqans_logentry += "\"userFQAN=" + Arc::trim(Arc::escape_chars(fqan, "\"\\", '\\', false)) + "\" ";
            }
        }
    } else {
        logger.msg(Arc::DEBUG, "No FQAN found. Using None as userFQAN value");
        fqans_logentry = "\"userFQAN=/None/Role=NULL\" ";
    }

    // Assemble BLAH logentry
    std::string logentry = "\"timestamp=" + timestamp +
                           "\" \"userDN=" + Arc::trim(subject) +
                           "\" " + fqans_logentry +
                           "\"ceID=" + ceid + "-" + queue +
                           "\" \"jobID=" + std::string(jobid_s) +
                           "\" \"lrmsID=" + Arc::trim(localid) +
                           "\" \"localUser=" + std::string(user_s) +
                           "\" \"clientID=" + clientid +
                           "\"";
    logger.msg(Arc::DEBUG, "Assembling BLAH parser log entry: %s", logentry);

    // Write entry to BLAH log with locking to exclude possible simultaneous writes when several jobs are finished
    std::string fname = logprefix + "-" + logsuffix;
    Arc::FileLock lock(fname);
    logger.msg(Arc::DEBUG,"Writing the info the the BLAH parser log: %s", fname);
        for (int i = 10; !lock.acquire() && i >= 0; --i) {
        if (i == 0) return false;
        sleep(1);
    }
    std::ofstream logfile;
    logfile.open(fname.c_str(),std::ofstream::app);
    if(!logfile.is_open()) {
        logger.msg(Arc::ERROR,"Cannot open BLAH log file '%s'", fname);
        lock.release();
        return EXIT_FAILURE;
    }
    logfile << logentry << std::endl;
    logfile.close();
    lock.release();

    return EXIT_SUCCESS;
}
