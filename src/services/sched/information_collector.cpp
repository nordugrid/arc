#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32 
#include <arc/win32.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {

void GridSchedulerService::InformationCollector(void) {
   for(;;) {

    std::string xml_str = "\
<arc:InfoRoot xmlns:glue=\"http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01\" xmlns:arc=\"urn:knowarc\">\
  <Domains xmlns=\"http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01 pathto/GLUE2.xsd\">\
    <AdminDomain xmlns=\"http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01\">\
      <Services xmlns=\"http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01\">\
        <ComputingService BaseType=\"Service\" CreationTime=\"2009-07-16T07:55:47Z\" Validity=\"600\">\
          <Associations></Associations>\
          <Capability>executionmanagement.jobexecution</Capability>\
          <Complexity>endpoint=1,share=1,resource=1</Complexity>\
          <ComputingEndpoint BaseType=\"Endpoint\" CreationTime=\"2009-07-16T07:55:47Z\" Validity=\"600\">\
            <Associations>\
              <ComputingShareID>urn:ogsa:ComputingShare:knowarc1:2811:knowarc</ComputingShareID>\
            </Associations>\
            <HealthState>ok</HealthState>\
            <ID>urn:ogsa:ComputingEndpoint:knowarc1:2811</ID>\
            <ImplementationName>ARC1</ImplementationName>\
            <ImplementationVersion>0.9</ImplementationVersion>\
            <Implementor>NorduGrid</Implementor>\
            <Interface>OGSA-BES</Interface>\
            <JobDescription>ogf:jsdl:1.0</JobDescription>\
            <PreLRMSWaitingJobs>0</PreLRMSWaitingJobs>\
            <QualityLevel>development</QualityLevel>\
            <RunningJobs>0</RunningJobs>\
            <Semantics>http://svn.nordugrid.org/trac/nordugrid/browser/arc1/trunk/src/services/sched/README</Semantics>\
            <ServingState>production</ServingState>\
            <Staging>staginginout</Staging>\
            <StagingJobs>0</StagingJobs>\
            <SupportedProfile>WS-I 1.0</SupportedProfile>\
            <SupportedProfile>HPC-BP</SupportedProfile>\
            <SuspendedJobs>0</SuspendedJobs>\
            <Technology>webservice</Technology>\
            <TotalJobs>0</TotalJobs>\
            <ComputingActivities>\
            </ComputingActivities>\
          </ComputingEndpoint>\
          <ComputingShares>\
            <ComputingShare BaseType=\"Share\" CreationTime=\"2009-07-16T08:46:43Z\" Validity=\"600\">\
              <Associations>\
                <ComputingEndpointID>urn:ogsa:ComputingEndpoint:knowarc1:2811</ComputingEndpointID>\
              </Associations>\
              <Description>Job scheduler service. It speaks BES and iBES as well. It implements a job queue and schedules job to other BES services like A-REX or iBES clients like Paul.</Description>\
              <FreeSlots>2000</FreeSlots>\
              <FreeSlotsWithDuration>1:6483600</FreeSlotsWithDuration>\
              <ID>urn:ogsa:ComputingShare:knowarc1:2811:knowarc</ID>\
              <LocalRunningJobs>0</LocalRunningJobs>\
              <LocalSuspendedJobs>0</LocalSuspendedJobs>\
              <LocalWaitingJobs>0</LocalWaitingJobs>\
              <MappingQueue>knowarc</MappingQueue>\
              <MaxCPUTime>100860</MaxCPUTime>\
              <MaxMultiSlotWallTime>108060</MaxMultiSlotWallTime>\
              <MaxRunningJobs>4</MaxRunningJobs>\
              <MaxSlotsPerJob>1</MaxSlotsPerJob>\
              <MaxWallTime>108060</MaxWallTime>\
              <MinCPUTime>0</MinCPUTime>\
              <MinWallTime>0</MinWallTime>\
              <Name>knowarc</Name>\
              <PreLRMSWaitingJobs>0</PreLRMSWaitingJobs>\
              <RequestedSlots>0</RequestedSlots>\
              <RunningJobs>0</RunningJobs>\
              <ServingState>draining</ServingState>\
              <StagingJobs>0</StagingJobs>\
              <SuspendedJobs>0</SuspendedJobs>\
              <TotalJobs>0</TotalJobs>\
              <UsedSlots>0</UsedSlots>\
              <WaitingJobs>0</WaitingJobs>\
            </ComputingShare>\
          </ComputingShares>\
          <ID>urn:ogsa:ComputingService:knowarc1:2811</ID>\
          <Name>ARC1, Sched service</Name>\
          <PreLRMSWaitingJobs>0</PreLRMSWaitingJobs>\
          <QualityLevel>development</QualityLevel>\
          <RunningJobs>0</RunningJobs>\
          <StagingJobs>0</StagingJobs>\
          <SuspendedJobs>0</SuspendedJobs>\
          <TotalJobs>0</TotalJobs>\
          <Type>org.nordugrid.sched</Type>\
          <WaitingJobs>0</WaitingJobs>\
        </ComputingService>\
      </Services>\
    </AdminDomain>\
  </Domains>\
  <n:nordugrid xmlns:M0=\"urn:Mds\" xmlns:n=\"urn:nordugrid\" xmlns:nc0=\"urn:nordugrid-cluster\">\
    <M0:validfrom>20090716075547Z</M0:validfrom>\
    <M0:validto>20090716080547Z</M0:validto>\
    <nc0:aliasname>ARC1, Sched service</nc0:aliasname>\
    <nc0:architecture>i686</nc0:architecture>\
  </n:nordugrid>\
</arc:InfoRoot>";
    

    Arc::XMLNode root(xml_str);

    for (Arc::JobQueueIterator jobs = jobq.getAll(); jobs.hasMore(); jobs++) {
       Arc::Job *j = *jobs;
       Arc::XMLNode jobs = root["Domains"]["AdminDomain"]["Services"]["ComputingService"]["ComputingEndpoint"]["ComputingActivities"];
       Arc::XMLNode job = jobs.NewChild("glue:ComputingActivity");
       job.NewChild("glue:ID") =  "urn:ogsa:ComputingActivity:sched:" + j->getID();
       switch (j->getStatus()) {
         case Arc::JOB_STATUS_SCHED_NEW:
             job.NewChild("glue:State") = "bes:Pending";
             job.NewChild("glue:State") = "nordugrid:ACCEPTED";
             break; 
         case Arc::JOB_STATUS_SCHED_RESCHEDULED:
             job.NewChild("glue:State") = "bes:Pending";
             job.NewChild("glue:State") = "nordugrid:ACCEPTED";
             break; 
         case Arc::JOB_STATUS_SCHED_STARTING:
             job.NewChild("glue:State") = "bes:Pending";
             job.NewChild("glue:State") = "nordugrid:PREPARING";
             break; 
         case Arc::JOB_STATUS_SCHED_RUNNING:
             job.NewChild("glue:State") = "bes:Running";
             job.NewChild("glue:State") = "nordugrid:INLRMS:R";
             break; 
         case Arc::JOB_STATUS_SCHED_CANCELLED:
             job.NewChild("glue:State") = "bes:Cancelled";
             job.NewChild("glue:State") = "nordugrid:KILLED";
             break; 
         case Arc::JOB_STATUS_SCHED_FAILED:
             job.NewChild("glue:State") = "bes:Failed";
             job.NewChild("glue:State") = "nordugrid:FAILED";
             break; 
         case Arc::JOB_STATUS_SCHED_FINISHED:
             job.NewChild("glue:State") = "bes:Finished";
             job.NewChild("glue:State") = "nordugrid:FINISHED";
             break; 
         case Arc::JOB_STATUS_SCHED_KILLED:
             job.NewChild("glue:State") = "bes:Cancelled";
             job.NewChild("glue:State") = "nordugrid:KILLED";
             break; 
         case Arc::JOB_STATUS_SCHED_KILLING:
             job.NewChild("glue:State") = "bes:Cancelled";
             job.NewChild("glue:State") = "nordugrid:KILLED";
             break; 
         case Arc::JOB_STATUS_SCHED_UNKNOWN:
             job.NewChild("glue:State") = "bes:Unknown";
             job.NewChild("glue:State") = "nordugrid:UNKNOWN";
             break; 
       }
    }

    if(root) {
      // Put result into container
      infodoc_.Assign(root,true);

      std::string s;

      root.GetDoc(s, true);

      logger_.msg(Arc::DEBUG,"Assigned new informational document");
    } else {
      logger_.msg(Arc::ERROR,"Failed to create informational document");
    };
  
    sleep(5); 

   } // end of the for loop
  
}

bool GridSchedulerService::RegistrationCollector(Arc::XMLNode &doc) {
  logger_.msg(Arc::VERBOSE,"Passing service's information from collector to registrator");
  Arc::XMLNode empty(ns_, "RegEntry");
  empty.New(doc);

  doc.NewChild("SrcAdv");
  doc.NewChild("MetaSrcAdv");
  doc["SrcAdv"].NewChild("Type") = "org.nordugrid.execution.sched";
  doc["SrcAdv"].NewChild("EPR").NewChild("Address") = endpoint;
  return true;
}

} // end of the namespace



