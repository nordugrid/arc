#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "AccountingDBSQLite.h"
#include "AAR.h"

int main(int argc, char **argv) {
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

    ARex::AccountingDBSQLite adb("/tmp/adb.sqlite");
    if (!adb.IsValid()) {
       std::cerr << "Database connection was not successfull" << std::endl;
       return EXIT_FAILURE;
    }

    ARex::AAR aar;
    aar.jobid = "0DULDmc8azunjwO5upha6lOqABFKDmABFKDmpjJKDmABFKDmQs7RCo";
    aar.endpoint = { "org.nordugrid.arcrest", "https://arc.univ.kiev.ua:443/arex/" };
    aar.queue = "grid";
    aar.userdn = "/DC=org/DC=ugrid/O=people/O=KNU/CN=Andrii Salnikov";
    aar.wlcgvo = "testbed.univ.kiev.ua";
    aar.status = "in-progress";
    aar.submittime = Arc::Time("20190624101218Z");
    aar.authtokenattrs.push_back(ARex::aar_authtoken_t("vomsfqan", "/testbed.univ.kiev.ua"));
    aar.authtokenattrs.push_back(ARex::aar_authtoken_t("vomsfqan", "/testbed.univ.kiev.ua/Role=VO-Admin"));
    ARex::aar_jobevent_t accepted_event("ACCEPTED", Arc::Time("20190624101218Z"));
    aar.jobevents.push_back(accepted_event);

    adb.createAAR(aar);

    ARex::aar_jobevent_t preparing_event("PREPARING", Arc::Time("20190624121218Z"));
    adb.addJobEvent(preparing_event, aar.jobid);

    aar.localid = "2805309";
    aar.endtime = Arc::Time("20190625101758Z");
    aar.stageinvolume = 100;
    aar.status = "completed";
    aar.exitcode = 0;
    aar.nodecount = 2;
    aar.cpucount = 4;
    aar.usedmemory = 584;
    aar.usedvirtmemory = 678;
    aar.usedwalltime = 1;
    aar.usedcpuusertime = 0;
    aar.usedcpukerneltime = 0;
    aar.usedscratch = 0;
    aar.stageinvolume = 0;
    aar.stageoutvolume = 0;
    aar.rtes.push_back("ENV/PROXY");
    aar.rtes.push_back("ENV/CANDYPOND");

    aar.transfers.push_back({"srm://glite01.grid.hku.hk/dpm/grid.hku.hk/home/ops/nagios-snf-3988/arcce/srm-input", 57, Arc::Time("2019-03-20T09:41:37Z"), Arc::Time("2019-03-20T09:41:41Z"), ARex::dtr_input});

    aar.extrainfo.insert(std::pair <std::string, std::string>("jobname", "test2"));
    aar.extrainfo.insert(std::pair <std::string, std::string>("lrms", "pbs"));
    aar.extrainfo.insert(std::pair <std::string, std::string>("nodename", "s2"));
    aar.extrainfo.insert(std::pair <std::string, std::string>("clienthost", "192.0.2.100"));
    aar.extrainfo.insert(std::pair <std::string, std::string>("localuser", "tb175"));

    adb.updateAAR(aar);
}
