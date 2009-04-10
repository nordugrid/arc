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

static Arc::Logger logger_(Arc::Logger::rootLogger, "InfoSys");

namespace Arc {

    void InfoRegistrar::initISIS(XMLNode cfg) {
        // Process configuration
        defaultBootstrapISIS.key   = (std::string)cfg["KeyPath"];
        defaultBootstrapISIS.cert  = (std::string)cfg["CertificatePath"];
        defaultBootstrapISIS.proxy = (std::string)cfg["ProxyPath"];
        defaultBootstrapISIS.cadir = (std::string)cfg["CACertificatesDir"];
        defaultBootstrapISIS.url   = (std::string)cfg["URL"];
        if(defaultBootstrapISIS.url.empty()) {
            logger_.msg(ERROR, "Can't recognize URL: %s",(std::string)cfg["URL"]);
        } else {
            //logger_.msg(DEBUG, "InfoRegistrar created for URL: %s",(std::string)cfg["URL"]);
        }

        // Set up default values
        myISIS = defaultBootstrapISIS;
        originalISISCount = 1;
        myISISList.push_back(myISIS);

        // Fill the myISISList with the neighbors list and draw myISIS
        getISISList(myISIS);
    }

    void InfoRegistrar::removeISIS(ISIS_description isis) {
        // Remove isis from myISISList
        for (std::vector<ISIS_description>::iterator it = myISISList.begin();
             it <= myISISList.end() && ((*it).url != myISIS.url || myISISList.erase(it) == it); it++);

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
        // Try to get ISISList from the actual ISIS
        // Compose getISISList request
        Arc::NS query_ns;
        query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";

        PayloadSOAP request(query_ns);
        request.NewChild("GetISISList");

        // Send message
        PayloadSOAP *response;
        MCCConfig mcc_cfg;
        mcc_cfg.AddPrivateKey(isis.key);
        mcc_cfg.AddCertificate(isis.cert);
        mcc_cfg.AddProxy(isis.proxy);
        mcc_cfg.AddCADir(isis.cadir);

        ClientSOAP cli(mcc_cfg,isis.url);
        MCC_Status status = cli.process(&request, &response);

        // If the given ISIS wasn't available remove it and return
        if (!status.isOk() || !response || !bool((*response)["GetISISListResponse"])) {
            removeISIS(isis);
            return;
        }

        // Merge result with the orignal list of known ISIS's
        int i = 0;
        while((bool)(*response)["GetISISListResponse"]["EPR"][i]) {
            bool ISIS_found = false;
            for (std::vector<ISIS_description>::iterator it = myISISList.begin();
                it <= myISISList.end() && ((*it).url != (std::string) (*response)["GetISISListResponse"]["EPR"][i]
                || (ISIS_found = true)); it++);
            if ( !ISIS_found ) {
                ISIS_description new_ISIS;
                new_ISIS.url = (std::string)(*response)["GetISISListResponse"]["EPR"][i];
                myISISList.push_back(new_ISIS);
            }
            i++;
        }

        // Update the original number of ISIS's variable
        originalISISCount = myISISList.size();

        // Select a new random isis from the list
        std::srand(time(NULL));
        ISIS_description rndISIS = myISISList[std::rand() % myISISList.size()];

        myISIS = rndISIS;

    }

    ISIS_description InfoRegistrar::getISIS(void) {
        if (myISISList.size() == 0) {
            if ( myISIS.url == defaultBootstrapISIS.url ) {
                // If there is no available 
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

