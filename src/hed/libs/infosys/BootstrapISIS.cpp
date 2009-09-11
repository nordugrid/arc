#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <unistd.h>
#include <algorithm>

#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include <arc/StringConv.h>
#include "InfoRegister.h"
#ifdef WIN32
#include <arc/win32.h>
#endif

namespace Arc {

static Logger logger_(Logger::rootLogger, "InfoSys");

struct Registrar_data {
   ISIS_description isis;
   InfoRegistrar *registrar;
};

    void InfoRegistrar::initISIS(XMLNode cfg) {
        logger_.msg(DEBUG, "Initialize ISIS handler");
        // Process configuration
        defaultBootstrapISIS.url   = (std::string)cfg["URL"];
        if(defaultBootstrapISIS.url.empty()) {
            logger_.msg(ERROR, "Can't recognize URL: %s",(std::string)cfg["URL"]);
        } else {
            //logger_.msg(DEBUG, "InfoRegistrar created for URL: %s",(std::string)cfg["URL"]);
        }
        defaultBootstrapISIS.key = (std::string)cfg["KeyPath"];
        defaultBootstrapISIS.cert = (std::string)cfg["CertificatePath"];
        defaultBootstrapISIS.proxy = (std::string)cfg["ProxyPath"];
        defaultBootstrapISIS.cadir = (std::string)cfg["CACertificatesDir"];
        defaultBootstrapISIS.cafile = (std::string)cfg["CACertificatePath"];
               
        // Set up default values
        myISIS = defaultBootstrapISIS;
        originalISISCount = 1;
        myISISList.push_back(myISIS);

        getISISList(myISIS);
        logger_.msg(DEBUG, "Initialize ISIS handler successed");
    }

    void InfoRegistrar::removeISIS(ISIS_description isis) {
        logger_.msg(DEBUG, "Remove ISIS (%s) from list", isis.url);
        // Remove isis from myISISList
        for (std::vector<ISIS_description>::iterator it = myISISList.begin();
             it < myISISList.end() && ((*it).url != myISIS.url || myISISList.erase(it) == it); it++);

        // If the 'isis' is the currently used (myISIS) isis
        if ( isis.url == myISIS.url && myISISList.size() != 0 ) {
            // Select a new random isis from the list
            std::srand(time(NULL));
            ISIS_description rndISIS = myISISList[std::rand() % myISISList.size()];

            // Add the neighbors of the newly selected ISIS to the list and set myISIS to one of them
            getISISList(rndISIS);
        }

        // Check if there is enough ISIS's left
        getISIS();
    }

    void InfoRegistrar::getISISList(ISIS_description isis) {
        logger_.msg(DEBUG, "getISISList from %s", isis.url);
        logger_.msg(DEBUG, "Key %s, Cert: %s, CA: %s", isis.key, isis.cert, isis.cadir);
        // Try to get ISISList from the actual ISIS
        // Compose getISISList request
        NS query_ns;
        query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";

        // Try to get ISIS.getISISList()
        PayloadSOAP request(query_ns);
        request.NewChild("GetISISList");

        // Send message
        PayloadSOAP *response;
        MCCConfig mcc_cfg;
        mcc_cfg.AddPrivateKey(isis.key);
        mcc_cfg.AddCertificate(isis.cert);
        mcc_cfg.AddProxy(isis.proxy);
        if (!isis.cadir.empty()) {
            mcc_cfg.AddCADir(isis.cadir);
        }
        if (!isis.cafile.empty()) {
            mcc_cfg.AddCAFile(isis.cafile);
        }

        int retry_ = retry;
        int reconnection = 0;
        while ( retry_ >= 1 ) {
            ClientSOAP cli(mcc_cfg,isis.url,60);
            MCC_Status status = cli.process(&request, &response);
            retry_--;
            reconnection++;
            // If the given ISIS wasn't available try reconnect
            if (!status.isOk() || !response || !bool((*response)["GetISISListResponse"])) {
                logger_.msg(DEBUG, "ISIS (%s) is not avaliable or not valid response. (%d. reconnection)", isis.url, reconnection);
            } else {
                logger_.msg(DEBUG, "Connection to the ISIS (%s) is success and get the list of ISIS.", isis.url);
                break;
            }
        }

        // If the given ISIS wasn't available remove it and return
        if ( retry_ == 0 ) {
            removeISIS(isis);
            return;
        }

        // Merge result with the orignal list of known ISIS's
        int i = 0;
        while((bool)(*response)["GetISISListResponse"]["EPR"][i]) {
            bool ISIS_found = false;
            for (std::vector<ISIS_description>::iterator it = myISISList.begin();
                it < myISISList.end() && ((*it).url != (std::string) (*response)["GetISISListResponse"]["EPR"][i]
                || (ISIS_found = true)); it++);
            if ( !ISIS_found ) {
                ISIS_description new_ISIS;
                new_ISIS.url = (std::string)(*response)["GetISISListResponse"]["EPR"][i];
                new_ISIS.key = defaultBootstrapISIS.key;
                new_ISIS.cert = defaultBootstrapISIS.cert;
                new_ISIS.proxy = defaultBootstrapISIS.proxy;
                new_ISIS.cadir = defaultBootstrapISIS.cadir;
                new_ISIS.cafile = defaultBootstrapISIS.cafile;
                myISISList.push_back(new_ISIS);
                logger_.msg(DEBUG, "GetISISList add this (%s) ISIS into the list.", new_ISIS.url);
            }
            i++;
        }

        // Update the original number of ISIS's variable
        originalISISCount = myISISList.size();

        // Select a new random isis from the list
        std::srand(time(NULL));
        ISIS_description rndISIS = myISISList[std::rand() % myISISList.size()];

        logger_.msg(DEBUG, "Chosen ISIS for communication: %s", rndISIS.url);
        myISIS = rndISIS;

        if (response) delete response;
    }

    ISIS_description InfoRegistrar::getISIS(void) {
        logger_.msg(DEBUG, "Get ISIS from list of ISIS handler");
        if (myISISList.size() == 0) {
            if ( myISIS.url == defaultBootstrapISIS.url ) {
                logger_.msg(WARNING, "There is no more ISIS available. The list of ISIS's is already empty.");

                // Set up default values for the further tries
                myISIS = defaultBootstrapISIS;
                originalISISCount = 1;
                myISISList.push_back(myISIS);

                // If there is no available, return an empty ISIS
                ISIS_description temporary_ISIS;
                temporary_ISIS.url = "";
                return temporary_ISIS;
            } else {
                // Try to receive the "original" bootsrap informations, if the BootstrapISIS is already available.
                getISISList(defaultBootstrapISIS);
                return getISIS();
            }
        }
        if (myISISList.size() == 1) {
            // If there is only one known ISIS than force the check of availability of new cloud members.
            getISISList(myISIS);
            return myISIS;
        }
        if (myISISList.size() <= originalISISCount / 2) {
            // Select a new random isis from the list
            std::srand(time(NULL));
            ISIS_description rndISIS = myISISList[std::rand() % myISISList.size()];

            // Add the neighbors of the newly selected ISIS to the list and set myISIS to one of them
            getISISList(rndISIS);
        }
        //And finally...
        return myISIS;
    }

}

