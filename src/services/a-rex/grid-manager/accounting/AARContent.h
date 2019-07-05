#ifndef ARC_AAR_CONTENT_H
#define ARC_AAR_CONTENT_H

#include <string>
#include <map>
#include <list>
#include <arc/DateTime.h>

namespace ARex {

/*
 * Defines the data types to store A-REX Accounting Records (AAR)
 */

struct aar_endpoint_t {
    std::string interface;
    std::string url;
    bool operator<(const aar_endpoint_t& endpoint) const {
        if ( interface < endpoint.interface ) return true;
        if ( interface == endpoint.interface ) {
            if ( url < endpoint.url ) return true;
            return false;
        }
        return false;
    };
};

struct aar_data_transfer_t {
    std::string url;
    unsigned long long int size;
    Arc::Time transferstart;
    Arc::Time transferend;
    unsigned int type;
};

class AAR {
  public:
    AAR(void): jobid(""), localid(""), queue(""), userdn(""), wlcgvo(""), status(""), 
               exitcode(-1), submittime((time_t)(0)), endtime((time_t)(0)),
               nodecount(0), cpucount(0), usedmemory(0), usedvirtmemory(0),
               usedwalltime(0), usedcpuusertime(0), usedcpukerneltime(0),
               usedscratch(0), stageinvolume(0), stageoutvolume(0) {}
    /* Unique job ids */
    std::string jobid;              // job unique A-REX ID 
    std::string localid;            // job local LRMS ID
    /* Submission data */
    aar_endpoint_t  endpoint;       // endpoint type and URL used to submit job
    std::string queue;              // queue
    std::string userdn;             // distinguished name of the job owner
    std::string wlcgvo;             // WLCG VO name
    /* Completion data */
    std::string status;             // Job completion status
    int exitcode;                   // Job exit code
    /* Main accounting times to search jobs */
    Arc::Time submittime;           // Job submission time
    Arc::Time endtime;              // Job completion time
    /* Used resources */
    int nodecount;
    int cpucount;
    unsigned long long int usedmemory;
    unsigned long long int usedvirtmemory;
    unsigned long long int usedwalltime;
    unsigned long long int usedcpuusertime;
    unsigned long long int usedcpukerneltime;
    unsigned long long int usedscratch;
    unsigned long long int stageinvolume;
    unsigned long long int stageoutvolume;
    /* Complex extra data */
    std::map <std::string, std::string> authtokenattrs; // auth token attributes
    std::map <std::string, Arc::Time> jobevents;        // events of the job
    std::list <std::string> rte;                        // RTEs
    std::list <aar_data_transfer_t> transfers;          // data transfers information
    /* Store non-seachable optional text data, such as:
     *      jobname, lrms, nodename, clienthost, localuser, projectname, systemsoftware, wninstance, benchmark
     */
    std::map <std::string, std::string> extrainfo;      
};

}

#endif
