#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "delegation/DelegationStores.h"
#include "delegation/DelegationStore.h"
#include "job.h"
#include "grid-manager/files/ControlFileHandling.h"

#include "arex.h"

namespace ARex {


static bool match_lists(const std::list<std::string>& list1,
                        const std::list<std::string>& list2) {
  for(std::list<std::string>::const_iterator item1=list1.begin();item1!=list1.end();++item1) {
    for(std::list<std::string>::const_iterator item2=list2.begin();item2!=list2.end();++item2) {
      if(*item1 == *item2) return true;
    };
  };
  return false;
}

static bool match_list(const std::string& item1,
                        const std::list<std::string>& list2) {
  for(std::list<std::string>::const_iterator item2=list2.begin();item2!=list2.end();++item2) {
    if(item1 == *item2) return true;
  };
  return false;
}

#define MAX_ACTIVITIES (10000)

static bool match(const std::pair<std::string,std::list<std::string> >& status,const std::string& es_status, const std::list<std::string>& es_attributes) {
  // Specs do not define how exactly to match status. Assuming
  // exact match is needed.
  // TODO: algorithm below will not work in case of duplicate attributes.
  if(status.first != es_status) return false;
  if(status.second.size() != es_attributes.size()) return false;
  for(std::list<std::string>::const_iterator a = status.second.begin();
                       a != status.second.end();++a) {
    std::list<std::string>::const_iterator es_a = es_attributes.begin();
    for(;es_a != es_attributes.end();++es_a) {
      if((*a) == (*es_a)) break;
    };
    if(es_a == es_attributes.end()) return false;
  };
  return true;
}

static bool match(const std::list< std::pair<std::string,std::list<std::string> > >& statuses, const std::string& es_status, const std::list<std::string>& es_attributes) {
  for(std::list< std::pair<std::string,std::list<std::string> > >::const_iterator s =
                  statuses.begin();s != statuses.end();++s) {
    if(match(*s,es_status,es_attributes)) return true;
  };
  return false;
}

Arc::MCC_Status ARexService::ESListActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
 /*
   ListActivities
     FromDate (xsd:dateTime) 0-1
     ToDate (xsd:dateTime) 0-1
     Limit 0-1
     ActivityStatus 0-
       Status
       Attribute 0-

   ListActivitiesResponse
     ActivityID 0-
     truncated (attribute) - false

   InvalidParameterFault
   estypes:AccessControlFault
   estypes:InternalBaseFault
  */
  Arc::Time from((time_t)(-1));
  Arc::Time to((time_t)(-1));
  Arc::XMLNode node;
  unsigned int limit = MAX_ACTIVITIES;
  std::list< std::pair<std::string,std::list<std::string> > > statuses;
  bool filter_status = false;
  bool filter_time = false;

  // TODO: Adjust to end of day
  if((bool)(node = in["FromDate"])) {
    from = (std::string)node;
    if(from.GetTime() == (time_t)(-1)) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESInvalidParameterFault(fault,"failed to parse FromDate: "+(std::string)node);
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    filter_time = true;
  };
  if((bool)(node = in["ToDate"])) {
    to = (std::string)node;
    if(to.GetTime() == (time_t)(-1)) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESInvalidParameterFault(fault,"failed to parse ToDate: "+(std::string)node);
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    filter_time = true;
  };
  if((bool)(node = in["Limit"])) {
    if(!Arc::stringto((std::string)node,limit)) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESInternalBaseFault(fault,"failed to parse Limit: "+(std::string)node);
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    if(limit > MAX_ACTIVITIES) limit = MAX_ACTIVITIES;
  };
  for(node=in["ActivityStatus"];(bool)node;++node) {
    std::pair<std::string,std::list<std::string> > status;
    status.first = (std::string)(node["Status"]);
    if(status.first.empty()) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESInvalidParameterFault(fault,"Status in ActivityStatus is missing");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    for(Arc::XMLNode anode=node["Attribute"];(bool)anode;++anode) {
      status.second.push_back((std::string)anode);
    };
    statuses.push_back(status);
    filter_status = true;
  };
  std::list<std::string> job_ids = ARexJob::Jobs(config,logger);
  unsigned int count = 0;
  for(std::list<std::string>::iterator id = job_ids.begin();id!=job_ids.end();++id) {
    if(count >= limit) {
      out.NewAttribute("truncated") = "true";
      break;
    };
    if(filter_time || filter_status) {
      ARexJob job(*id,config,logger_);
      if(!job) continue;
      if(filter_time) {
        Arc::Time t = job.Created();
        if(from.GetTime() != (time_t)(-1)) {
          if(from > t) continue;
        };
        if(to.GetTime() != (time_t)(-1)) {
          if(to < t) continue;
        };
      };
      if(filter_status) {
        bool job_pending = false;
        std::string gm_state = job.State(job_pending);
        bool job_failed = job.Failed();
        std::string failed_cause;
        std::string failed_state = job.FailedState(failed_cause);
        std::string es_status;
        std::list<std::string> es_attributes;
        convertActivityStatusES(gm_state,es_status,es_attributes,job_failed,job_pending,failed_state,failed_cause);
        if(!match(statuses,es_status,es_attributes)) continue;
      };
    };
    out.NewChild("estypes:ActivityID") = *id;
    ++count;
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESGetActivityStatus(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    GetActivityStatus
      estypes:ActivityID 1-

    GetActivityStatusResponse
      ActivityStatusItem 1-
        estypes:ActivityID
        .
          estypes:ActivityStatus
            Status
            Attribute 0-
            Timestamp (dateTime)
            Description 0-1
          estypes:InternalBaseFault
          estypes:AccessControlFault
          UnknownActivityIDFault
          UnableToRetrieveStatusFault
          OperationNotPossibleFault
          OperationNotAllowedFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
   */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esainfo:ActivityStatusItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:GetActivityStatus: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      bool job_pending = false;
      std::string gm_state = job.State(job_pending);
      bool job_failed = job.Failed();
      std::string failed_cause;
      std::string failed_state = job.FailedState(failed_cause);
      Arc::XMLNode status = addActivityStatusES(item,gm_state,Arc::XMLNode(),job_failed,job_pending,failed_state,failed_cause);
      status.NewChild("estypes:Timestamp") = job.Modified().str(Arc::ISOTime); // no definition of meaning in specs
      //status.NewChild("estypes:Description);  TODO
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESGetActivityInfo(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    GetActivityInfo
      estypes:ActivityID 1-
      AttributeName (xsd:QName) 0-

    GetActivityInfoResponse
      ActivityInfoItem 1-
        ActivityID
        .
          ActivityInfoDocument (glue:ComputingActivity_t)
            StageInDirectory
            StageOutDirectory
            SessionDirectory
            ComputingActivityHistory 0-1
              estypes:ActivityStatus 0-  - another ActivityStatus defined in ActivityInfo
              Operation 0-
                RequestedOperation
                Timestamp
                Success
          AttributeInfoItem 1-
            AttributeName
            AttributeValue
          InternalBaseFault
          AccessControlFault
          ActivityNotFoundFault
          UnknownAttributeFault
          UnableToRetrieveStatusFault
          OperationNotPossibleFault
          OperationNotAllowedFault

    estypes:VectorLimitExceededFault
    UnknownAttributeFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
   */
  static const char* job_xml_template = "\
    <ComputingActivity xmlns=\"http://schemas.ogf.org/glue/2009/03/spec_2.0_r1\"\n\
                       BaseType=\"Activity\" CreationTime=\"\" Validity=\"60\">\n\
      <ID></ID>\n\
      <OtherInfo>SubmittedVia=org.ogf.glue.emies.activitycreation</OtherInfo>\n\
      <Type>single</Type>\n\
      <IDFromEndpoint></IDFromEndpoint>\n\
      <JobDescription>emies:adl</JobDescription>\n\
      <State></State>\n\
      <Owner></Owner>\n\
      <Associations>\n\
        <ComputingShareID></ComputingShareID>\n\
      </Associations>\n\
    </ComputingActivity>";
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  std::list<std::string> attributes;
  for(Arc::XMLNode anode = in["AttributeName"];(bool)anode;++anode) {
    attributes.push_back((std::string)anode);
  };
  //if(!attributes.empty()) {
  //  Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
  //  ESUnknownAttributeFault(fault,"Selection by AttributeName is not implemented yet");
  //  out.Destroy();
  //  return Arc::MCC_Status(Arc::STATUS_OK);
  //};
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esainfo:ActivityInfoItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:GetActivityStatus: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      //  ActivityInfoDocument (glue:ComputingActivity_t)
      //    StageInDirectory 0-
      //    StageOutDirectory 0-
      //    SessionDirectory 0-
      //    ComputingActivityHistory 0-1
      std::string glue_s;
      bool response_generated = false;

      Arc::XMLNode glue_xml(job_xml_read_file(jobid,config.GmConfig(),glue_s)?glue_s:"");
      if(!glue_xml) {
        // TODO: if xml information is not ready yet create something minimal
        Arc::XMLNode(job_xml_template).New(glue_xml);
        Arc::URL headnode(config.GmConfig().HeadNode());
        glue_xml["ID"] = std::string("urn:caid:")+headnode.Host()+":org.ogf.glue.emies.activitycreation:"+jobid;
        glue_xml["IDFromEndpoint"] = "urn:idfe:"+jobid;
        {
          // Collecting job state
          bool job_pending = false;
          std::string gm_state = job.State(job_pending);
          bool job_failed = job.Failed();
          std::string failed_cause;
          std::string failed_state = job.FailedState(failed_cause);
          std::string primary_state;
          std::list<std::string> state_attributes;
          convertActivityStatusES(gm_state,primary_state,state_attributes,
                                  job_failed,job_pending,failed_state,failed_cause);
          glue_xml["State"] = "emies:"+primary_state;;
          std::string prefix = glue_xml["State"].Prefix();
          for(std::list<std::string>::iterator attr = state_attributes.begin();
                    attr != state_attributes.end(); ++attr) {
            glue_xml.NewChild(prefix+":State") = "emiesattr:"+(*attr);
          };
        };
        glue_xml["Owner"] = config.GridName();
        glue_xml.Attribute("CreationTime") = job.Created().str(Arc::ISOTime);
      };
      if((bool)glue_xml) {
        if(attributes.empty()) {
            Arc::XMLNode info;
            std::string glue2_namespace = glue_xml.Namespace();
            (info = item.NewChild(glue_xml)).Name("esainfo:ActivityInfoDocument");
            info.Namespaces(ns_);
            std::string glue2_prefix = info.NamespacePrefix(glue2_namespace.c_str());
            /*
            // Collecting job state
            bool job_pending = false;
            std::string gm_state = job.State(job_pending);
            bool job_failed = job.Failed();
            std::string failed_cause;
            std::string failed_state = job.FailedState(failed_cause);
            // Adding EMI ES state along with Glue state.
            // TODO: check if not already in infosys generated xml
            Arc::XMLNode status = info.NewChild(glue2_prefix+":State",0,false);
            {
              std::string primary_state;
              std::list<std::string> state_attributes;
              convertActivityStatusES(gm_state,primary_state,state_attributes,
                                      job_failed,job_pending,failed_state,failed_cause);
              status = std::string("emies:")+primary_state;
            };
            */
            // TODO: all additional ellements can be stored into xml generated by infoprovider
            // Extensions store delegation id(s)
            std::list<std::string> delegation_ids;
            DelegationStores* deleg = config.GmConfig().GetDelegations();
            if(deleg) {
              DelegationStore& dstore = (*deleg)[config.GmConfig().DelegationDir()];
              delegation_ids = dstore.ListLockedCredIDs(jobid, config.GridName());
            };
            if(!delegation_ids.empty()) {
              Arc::XMLNode extensions = info.NewChild(glue2_prefix+":Extensions");
              int n = 0;
              for(std::list<std::string>::iterator id = delegation_ids.begin();
                              id != delegation_ids.end();++id) {
                Arc::XMLNode extension = extensions.NewChild(glue2_prefix+":Extension");
                extension.NewChild(glue2_prefix+":LocalID") = "urn:delegid:nordugrid.org";
                extension.NewChild(glue2_prefix+":Key") = Arc::tostring(n);
                extension.NewChild(glue2_prefix+":Value") = *id;
                // TODO: add source and destination later
              };
            };
            // Additional elements
            info.NewChild("esainfo:StageInDirectory") = config.Endpoint()+"/"+job.ID();
            info.NewChild("esainfo:StageOutDirectory") = config.Endpoint()+"/"+job.ID();
            info.NewChild("esainfo:SessionDirectory") = config.Endpoint()+"/"+job.ID();
            // info.NewChild("esainfo:ComputingActivityHistory")
            response_generated = true;
        } else {
            // Attributes request
            // AttributeInfoItem 1-
            //   AttributeName
            //   AttributeValue
            // UnknownAttributeFault
            bool attribute_added = false;
            for(std::list<std::string>::iterator attr = attributes.begin();
                                      attr != attributes.end(); ++attr) {
              Arc::XMLNode axml = glue_xml[*attr];
              for(;axml;++axml) {
                Arc::XMLNode aitem = item.NewChild("esainfo:AttributeInfoItem");
                aitem.NewChild("esainfo:AttributeName") = *attr;
                aitem.NewChild("esainfo:AttributeValue") = (std::string)axml;
                attribute_added = true;
              };
              if((*attr == "StageInDirectory") ||
                 (*attr == "StageOutDirectory") ||
                 (*attr == "SessionDirectory")) {
                Arc::XMLNode aitem = item.NewChild("esainfo:AttributeInfoItem");
                aitem.NewChild("esainfo:AttributeName") = *attr;
                aitem.NewChild("esainfo:AttributeValue") = config.Endpoint()+"/"+job.ID();
                attribute_added = true;
              };
            };
            // It is not clear if UnknownAttributeFault must be
            // used if any or all attributes not found. Probably
            // it is more useful to do that only if nothing was
            // found.
            if(!attribute_added) {
              ESUnknownAttributeFault(item.NewChild("dummy"),"None of specified attributes is available");
            };
            response_generated = true;
        };
      };
      if(!response_generated) {
        logger_.msg(Arc::ERROR, "EMIES:GetActivityInfo: job %s - failed to retrieve GLUE2 information", jobid);
        ESInternalBaseFault(item.NewChild("dummy"),"Failed to retrieve GLUE2 information");
        // It would be good to have something like UnableToRetrieveStatusFault here
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESNotifyService(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    NotifyService
      NotifyRequestItem 1-
        estypes:ActivityID
        NotifyMessage
          [client-datapull-done|client-datapush-done]

    NotifyServiceResponse
      NotifyResponseItem 1-
        estypes:ActivityID
        .
          Acknowledgement
          OperationNotPossibleFault
          OperationNotAllowedFault
          InternalNotificationFault
          ActivityNotFoundFault
          AccessControlFault
          InternalBaseFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
   */
  Arc::XMLNode item = in["NotifyRequestItem"];
  unsigned int n = 0;
  for(;(bool)item;++item) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many NotifyRequestItem");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  item = in["NotifyRequestItem"];
  for(;(bool)item;++item) {
    std::string jobid = (std::string)(item["ActivityID"]);
    std::string msg = (std::string)(item["NotifyMessage"]);
    Arc::XMLNode ritem = out.NewChild("esmanag:NotifyResponseItem");
    ritem.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:NotifyService: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(ritem.NewChild("dummy"),job.Failure());
    } else {
      if(msg == "client-datapull-done") {
        // Client is done with job. Same as wipe request. Or should job go to deleted?
        if(!job.Clean()) {
          // Failure is not fatal here
          logger_.msg(Arc::ERROR, "EMIES:NotifyService: job %s - %s", jobid, job.Failure());
        };
        ritem.NewChild("esmanag:Acknowledgement");
      } else if(msg == "client-datapush-done") {
        if(!job.ReportFilesComplete()) {
          ESInternalBaseFault(ritem.NewChild("dummy"),job.Failure());
          // TODO: Destroy job (at least try to)
        } else {
          ritem.NewChild("esmanag:Acknowledgement");
          gm_->RequestJobAttention(job.ID()); // Tell GM to resume this job
        };
      } else {
        // Wrong request
        ESInternalNotificationFault(ritem.NewChild("dummy"),"Unsupported notification type "+msg);
        // Or maybe OperationNotPossibleFault?
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

