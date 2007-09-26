#include <sstream>

#include "ldif/LDIFtoXML.h"
#include "arex.h"

namespace ARex {


static const char* ldif_str = 
"dn: nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-cluster\n"
"nordugrid-cluster-name: grid.uio.no\n"
"nordugrid-cluster-aliasname: EPF (UiO/FI)\n"
"nordugrid-cluster-owner: University of Oslo\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-queue-name=bigmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-queue\n"
"nordugrid-queue-name: bigmem\n"
"nordugrid-queue-status: active\n"
"nordugrid-queue-homogeneity: TRUE\n"
"nordugrid-queue-nodecpu: Intel(R) Pentium(R) D CPU @ 3000 MHz\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-info-group-name=jobs, nordugrid-queue-name=bigmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-info-group\n"
"nordugrid-info-group-name: jobs\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-job-globalid=gsiftp://grid.uio.no:2811/jobs/2288011907374131619643632, nordugrid-info-group-name=jobs, nordugrid-queue-name=bigmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-job\n"
"nordugrid-job-globalid: gsiftp://grid.uio.no:2811/jobs/2288011907374131619643632\n"
"nordugrid-job-globalowner: /C=CH/O=Universite de Geneve/CN=Xin Zhou/emailAddress=xin.zhou@medecine.unige.ch\n"
"nordugrid-job-jobname: geneva-gift0\n"
"nordugrid-job-execcluster: grid.uio.no\n"
"nordugrid-job-execqueue: bigmem\n"
"nordugrid-job-status: FINISHED\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-job-globalid=gsiftp://grid.uio.no:2811/jobs/2288011907374131619643633, nordugrid-info-group-name=jobs, nordugrid-queue-name=bigmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-job\n"
"nordugrid-job-globalid: gsiftp://grid.uio.no:2811/jobs/2288011907374131619643633\n"
"nordugrid-job-globalowner: /C=CH/O=Universite de Geneve/CN=Xin Zhou/emailAddress=xin.zhou@medecine.unige.ch\n"
"nordugrid-job-jobname: geneva-gift0\n"
"nordugrid-job-execcluster: grid.uio.no\n"
"nordugrid-job-execqueue: bigmem\n"
"nordugrid-job-status: FINISHED\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-queue-name=smallmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-queue\n"
"nordugrid-queue-name: smallmem\n"
"nordugrid-queue-status: active\n"
"nordugrid-queue-homogeneity: TRUE\n"
"nordugrid-queue-nodecpu: Intel(R) Pentium(R) D CPU @ 3000 MHz\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-info-group-name=users, nordugrid-queue-name=smallmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-info-group\n"
"nordugrid-info-group-name: jobs\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-authuser-name=Marko Niinimaki, nordugrid-info-group-name=users, nordugrid-queue-name=smallmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-authuser\n"
"nordugrid-authuser-name: Marko Niinimaki...15\n"
"nordugrid-authuser-sn: /O=Grid/O=NorduGrid/OU=hip.fi/CN=Marko Niinimaki\n"
"nordugrid-authuser-freecpus: 4\n"
"nordugrid-authuser-diskspace: 22211\n"
"nordugrid-authuser-queuelength: 0\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n"
"\n"
"dn: nordugrid-authuser-name=Nobody, nordugrid-info-group-name=users, nordugrid-queue-name=smallmem,nordugrid-cluster-name=grid.uio.no,Mds-Vo-name=local,o=grid\n"
"objectClass: Mds\n"
"objectClass: nordugrid-authuser\n"
"nordugrid-authuser-name: Nobody\n"
"nordugrid-authuser-sn: /O=Grid/O=NorduGrid/OU=nowhere/CN=Nobody\n"
"nordugrid-authuser-freecpus: 4\n"
"nordugrid-authuser-diskspace: 22211\n"
"nordugrid-authuser-queuelength: 0\n"
"Mds-validfrom: 20070925162548Z\n"
"Mds-validto: 20070925162618Z\n";

void ARexService::InformationCollector(void) {
  for(;;) {
    // Run information providers
    // For testing purposes use hardcoded information
    std::string s(ldif_str);
    std::istringstream ldif(s);
    // Convert result to XML
    Arc::NS ns;
    ns["n"]="urn:nordugrid";
    Arc::XMLNode xml(ns);
    Arc::XMLNode root = xml.NewChild("n:nordugrid");
    if(LDIFtoXML(ldif,"Mds-Vo-name=local,o=Grid",root)) {
      // Put result into container
      infodoc_.Assign(root);
std::cerr<<"Assigned new informational document"<<std::endl;
    } else {
std::cerr<<"Failed to create informational document"<<std::endl;
    };
    sleep(60);
  };
}

}

