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

typedef enum {
    dtr_input = 10,
    dtr_cache_input = 11,
    dtr_output = 20
} dtr_type;

struct aar_data_transfer_t {
    std::string url;
    unsigned long long int size;
    Arc::Time transferstart;
    Arc::Time transferend;
    dtr_type type;
};

typedef std::pair <std::string, Arc::Time> aar_jobevent_t;
typedef std::pair <std::string, std::string> aar_authtoken_t;

class GMJob;
class GMConfig;

/*
 * C++ class representing A-REX Accounting Record (AAR) structure and corresponding methods to build it
 */

class AAR {
  public:
    AAR(void): jobid(""), localid(""), queue(""), userdn(""), wlcgvo(""), status(""), 
               exitcode(1), submittime((time_t)(0)), endtime((time_t)(0)),
               nodecount(1), cpucount(1), usedmemory(0), usedvirtmemory(0),
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
    unsigned int nodecount;
    unsigned int cpucount;
    unsigned long long int usedmemory;
    unsigned long long int usedvirtmemory;
    unsigned long long int usedwalltime;
    unsigned long long int usedcpuusertime;
    unsigned long long int usedcpukerneltime;
    unsigned long long int usedscratch;
    unsigned long long int stageinvolume;
    unsigned long long int stageoutvolume;
    /* Complex extra data */
    std::list <aar_authtoken_t> authtokenattrs;     // auth token attributes
    std::list <aar_jobevent_t> jobevents;           // events of the job
    std::list <std::string> rtes;                   // RTEs
    std::list <aar_data_transfer_t> transfers;      // data transfers information
    /* Store non-seachable optional text data, such as:
     *      jobname, lrms, nodename, clienthost, localuser, projectname, systemsoftware, wninstance, benchmark
     */
    std::map <std::string, std::string> extrainfo;

    /// Fetch info from the job's controldir files and fill AAR data structures
    bool FetchJobData(const GMJob &job,const GMConfig& config);
  private:
    static Arc::Logger logger;
};

}

#endif
