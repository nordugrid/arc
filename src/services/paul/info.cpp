#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/DateTime.h> 
#include <arc/GUID.h>
#include "paul.h"
#include "sysinfo.h"

namespace Paul
{

bool PaulService::information_collector(Arc::XMLNode &doc)
{
    // refresh dynamic system information
    sysinfo.refresh();
    // common variables
    std::string created = TimeStamp(Arc::UTCTime);
    std::string validity = Arc::tostring(configurator.getPeriod()*2);
    std::string id = "urn:nordugrid:paul:" + Arc::UUID(); 
    std::string ee_id = id + ":executionenvironment:0";
    // Set Domain
    doc.NewAttribute("CreationTime") = created;
    doc.NewAttribute("Validity") = validity;
    // Set AdminDomain
    Arc::XMLNode ad = doc.NewChild("AdminDomain");
    ad.NewAttribute("CreationTime") = created;
    ad.NewAttribute("Validity") = validity;
    ad.NewAttribute("BaseType") = "Domain";
    ad.NewChild("ID") = id;
    ad.NewChild("Distributed") = "False";
    // Set Computing Service
    //      Service Part
    Arc::XMLNode services = ad.NewChild("Services");
    Arc::XMLNode cs = services.NewChild("ComputingService");
    cs.NewAttribute("CreationTime") = created;
    cs.NewAttribute("Validity") = validity;
    cs.NewAttribute("BaseType") = "Service";
    cs.NewChild("ID") = id + ":computingservice:0";
    cs.NewChild("Name") = "Arc Paul Service";
    cs.NewChild("Type") = "org.nordugrid.paul";
    cs.NewChild("QualityLevel") = "production";
    // No endpont -> pull modell
    //      Computing Service part
    cs.NewChild("TotalJobs") = Arc::tostring(jobq.getTotalJobs());
    cs.NewChild("RunningJobs") = Arc::tostring(jobq.getRunningJobs());
    cs.NewChild("WaitingJobs") = Arc::tostring(jobq.getWaitingJobs());
    cs.NewChild("StageingJobs") = Arc::tostring(jobq.getStageingJobs());
    Arc::XMLNode sh = cs.NewChild("ComputingShares").NewChild("ComputingShare");
    sh.NewAttribute("CreationTime") = created;
    sh.NewAttribute("Validity") = validity;
    sh.NewAttribute("BaseType") = "Share";
    //      Share part
    sh.NewChild("LocalID") = "computingshare:0";
    //      Computing Share part
    sh.NewChild("MaxTotalJobs") = Arc::tostring(sysinfo.getLogicalCPUs());
    sh.NewChild("MaxMemory") = Arc::tostring((int)sysinfo.getMainMemorySize()); // in MB
    sh.NewChild("MaxDiskSpace") = Arc::tostring((int)(SysInfo::diskFree(configurator.getJobRoot())/1024)); // in GB
    sh.NewChild("ServingState") = "production";
    sh.NewChild("TotalJobs") = Arc::tostring(jobq.getTotalJobs());
    sh.NewChild("RunningJobs") = Arc::tostring(jobq.getRunningJobs());
    sh.NewChild("WaitingJobs") = Arc::tostring(jobq.getWaitingJobs());
    sh.NewChild("StageingJobs") = Arc::tostring(jobq.getStageingJobs());
    sh.NewChild("Associations").NewChild("ExecutionEnvironmentLocalID") = ee_id;
    //      Manager part
    Arc::XMLNode mg = cs.NewChild("ComputingManager");
    mg.NewAttribute("CreationTime") = created;
    mg.NewAttribute("Validity") = validity;
    mg.NewAttribute("BaseType") = "Manager";
    mg.NewChild("ID") = id + ":computingmanager:0";
    mg.NewChild("Name") = "Paul";
    //      Computing Manager part
    mg.NewChild("Type") = "fork";
    mg.NewChild("Version") = "1.0";
    mg.NewChild("TotalPhysicalCPUs") = Arc::tostring(sysinfo.getPhysicalCPUs());
    mg.NewChild("TotalLogicalCPUs") = Arc::tostring(sysinfo.getLogicalCPUs());
    mg.NewChild("Homogenety") = "True";
    mg.NewChild("WorkingAreaTotal") = Arc::tostring((int)(SysInfo::diskTotal(configurator.getJobRoot())/1024)); // in GB
    mg.NewChild("WorkingAreaFree") = Arc::tostring((int)(SysInfo::diskFree(configurator.getJobRoot())/1024)); // in GB
    // Application environment
#if 0
    Arc::XMLNode ae = mg.NewChild("ApplicationEnvironments");
    Arc::XMLNode appenvs = configurator.getApplicationEnvironments();
    Arc::XMLNode env;

    for (int i = 0; (env = appenvs["ApplicationEnvironment"]) != false; i++) {
        env.NewAttribute("CreationTime") = created;
        env.NewAttribute("Validity") = validity;
        env.NewChild("LocalID") = ("applicationenvironment:" + Arc::tostring(i));
        env.NewChild("Associations").NewChild("ExecutionEnvironmentLocalID") = ee_id;
        ae.NewChild(env);
    }
 #endif   
    //  Resource part
    Arc::XMLNode exec = mg.NewChild("ExecutionEnvironment");
    exec.NewAttribute("CreationTime") = created;
    exec.NewAttribute("Validity") = validity;
    exec.NewAttribute("BaseType") = "Resource";
    exec.NewChild("ID") = ee_id;
    //  ExecutionEnvironment part
    exec.NewChild("Platform") = sysinfo.getPlatform();
    exec.NewChild("PhysicalCPUs") = Arc::tostring(sysinfo.getPhysicalCPUs());
    exec.NewChild("LogicalCPUs") = Arc::tostring(sysinfo.getLogicalCPUs());
    exec.NewChild("MainMemorySize") = Arc::tostring(sysinfo.getMainMemorySize()); // in MB
    exec.NewChild("VirtualMemorySize") = Arc::tostring(sysinfo.getVirtualMemorySize()); // in MB
    exec.NewChild("OSFamily") = sysinfo.getOSFamily();
    exec.NewChild("OSName") = sysinfo.getOSName();
    exec.NewChild("OSVersion") = sysinfo.getOSVersion();
    return true;
}


} // namespace
