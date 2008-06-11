#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "paul.h"

namespace Paul
{

bool PaulService::information_collector(Arc::XMLNode &doc)
{
    // refresh dinamic system information
    sysinfo.refresh();
#if 0
    Arc::XMLNode ad = doc.NewChild("AdminDomain");
    ad.NewChild("ID") = "http://localhost/paul"; // XXX should be better
    ad.NewChild("Name") = "DesktopPC";
    ad.NewChild("Distriuted") = "no";
    ad.NewChild("Owner") = "someone"; // get user name required
    Arc::XMLNode services = ad.NewChild("Services");
    Arc::XMLNode cs = services.NewChild("ComputingService");
    cs.NewAttribute("type") = "Service";
    cs.NewChild("ID") = "http://localhost/paul";
    cs.NewChild("Type") = "org.nordugrid.paul";
    cs.NewChild("QualityLevel") = "production";
    cs.NewChild("Complexity") = "endpoint=1,share=1,resource=1";
    cs.NewChild("TotalJobs") = Arc::tostring(jobq.getTotalJobs());
    cs.NewChild("RunningJobs") = Arc::tostring(jobq.getRunningJobs());
    cs.NewChild("WaitingJobs") = Arc::tostring(jobq.getWaitingJobs());
    cs.NewChild("StageingJobs") = "0"; // XXX get info from jobqueue
    cs.NewChild("PreLRMSWaitingJobs") = "0"; // XXX get info from jobqueue
    Arc::XMLNode endpoint = cs.NewChild("ComputingEndpoint");
    endpoint.NewAttribute("type") = "Endpoint";
    endpoint.NewChild("ID") = "0"; 
    // endpoint.NewChild("URL") = "none"; // Not exsits
    endpoint.NewChild("Specification") = "iBES";
    endpoint.NewChild("Implementator") = "Nordugrid";
    endpoint.NewChild("ImplementationName") = "paul";
    endpoint.NewChild("ImplementationVersion") = "0.1";
    endpoint.NewChild("QualityLevel") = "production";
    // endpoint.NewChild("WSDL") = "http://"; // not required this side
    endpoint.NewChild("HealthState") = "OK";
    // endpoint.NewChild("ServingState") = "open"; // meaingless
    // endpoint.NewChild("StartTime") = "now";
    endpoint.NewChild("StageingCapabilty") = "yes";
    Arc::XMLNode policy = endpoint.NewChild("Policy");
    policy.NewChild("ID") = "0";
    policy.NewChild("Scheme") = "BASIC";
    policy.NewChild("Rule") = "?";
    policy.NewChild("TrustedCA") = "?"; // XXX get DNs from config ca.pem or dir
#if 0
    Arc::XMLNode activities = endpoint.NewChild("Activities");
    JobList::iterator it;
    for (it = job_list.begin(); it != job_list.end(); it++) {
        Job j = (*it);
        Arc::XMLNode jn = activities.NewChild("ComputingActivity");
        jn.NewAttribute("type") = "Computing";
        jn.NewChild("ID") = j.getID();
        jn.NewChild("Type") = "Computing";
        jn.NewChild("Name") = j.getName(); // from jsdl
        jn.NewChild("State") = (std::string)j.getStatus(); // from metadata
    }
#endif
    Arc::XMLNode r = cs.NewChild("ComputingResource");
    r.NewAttribute("type") = "Resource";
    r.NewChild("ID") = "0";
    r.NewChild("Name") = "DesktopPC";
    r.NewChild("LRMSType") = "Fork/Internal";
    r.NewChild("LRMSVersion") = "0.1";
    r.NewChild("Total") = "1"; // number of exec env
    r.NewChild("Used") = "0"; // comes from running job
    r.NewChild("Homogenity") = "True";
    r.NewChild("SessionDirTotal") = Arc::tostring(SysInfo::diskTotal(job_root));
    r.NewChild("SessionDirFree") = Arc::tostring(SysInfo::diskFree(job_root));
    r.NewChild("SessionDirLifetime") = "10"; // XXX should come from config in sec
    Arc::XMLNode ee = r.NewChild("ExecutionEnvironment");
    ee.NewChild("ID") = "0";
    ee.NewChild("Type") = "RealNode"; // XXX should come form config wheter it is running on VM or not
    ee.NewChild("TotalInstances") = "1";
    ee.NewChild("UsedInstances") = "0"; // if I have multicore machine running 1 job than this is 0 or 1 or what
    ee.NewChild("PhysicalCPUs") = Arc::tostring(sysinfo.getPhysicalCPUs());
    ee.NewChild("LogicalCPUs") = Arc::tostring(sysinfo.getLogicalCPUs());
    ee.NewChild("MainMemorySize") = Arc::tostring(sysinfo.getMainMemorySize());
    ee.NewChild("VirtualMemorySize") = Arc::tostring(sysinfo.getVirtualMemorySize());
    ee.NewChild("OSName") = sysinfo.getOSName();
    ee.NewChild("OSRelease") = sysinfo.getOSRelease();
    ee.NewChild("OSVersion") = sysinfo.getOSVersion();
    ee.NewChild("PlatformType") = sysinfo.getPlatform();
    ee.NewChild("ConnectivityIn") = "False";
    ee.NewChild("ConnectivityOut") = "True";
    r.NewChild("PhysicalCPUs") = Arc::tostring(sysinfo.getPhysicalCPUs());
    r.NewChild("LogicalCPUs") = Arc::tostring(sysinfo.getLogicalCPUs());
    Arc::XMLNode s = cs.NewChild("ComputingShare");
    s.NewAttribute("type") = "Share";
    s.NewChild("ID") = "0";
    s.NewChild("Name") = "AllResources";
    Arc::XMLNode spolicy = s.NewChild("ComputingSharePolicy");
    spolicy.NewAttribute("type") = "SharePolicy";
    Arc::XMLNode sstate = s.NewChild("ComputingShareState");
    sstate.NewAttribute("type") = "ShareState";
    sstate.NewChild("TotalJobs") = Arc::tostring(jobq.getTotalJobs());
    sstate.NewChild("RunningJobs") = Arc::tostring(jobq.getRunningJobs());
    sstate.NewChild("LocalRunningJobs") = Arc::tostring(jobq.getLocalRunningJobs());
    sstate.NewChild("WaitingJobs") = Arc::tostring(jobq.getWaitingJobs());
    sstate.NewChild("State") = "?";
    s.NewChild(policy); // use the same policy
#endif
    return true;
}


} // namespace
