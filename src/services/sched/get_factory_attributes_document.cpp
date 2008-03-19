#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"
#include <arc/StringConv.h>


namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::GetFactoryAttributesDocument(Arc::XMLNode &in,Arc::XMLNode &out) {
    /*
    GetFactoryAttributesDocument

    GetFactoryAttributesDocumentResponse
        FactoryResourceAttributesDocument
            BasicResourceAttributesDocument (optional)
            IsAcceptingNewActivities (boolean)
            CommonName (optional,string)
            LongDescription (optional,string)
            TotalNumberOfActivities (long)
            ActivityReference (wsa:EndpointReferenceType,unbounded)
            TotalNumberOfContainedResources (long)
            ContainedResource (anyType,unbounded)
            NamingProfile (anyURI,unbounded)
            BESExtension (anyURI,unbounded)
            LocalResourceManagerType (anyURI)
    */
    {
        std::string s;
        in.GetXML(s);
        logger.msg(Arc::DEBUG, "GetFactoryAttributesDocument: request = \n%s", s);
    };
    Arc::XMLNode doc = out.NewChild("bes-factory:FactoryResourceAttributesDocument");
    doc.NewChild("bes-factory:IsAcceptingNewActivities") = (IsAcceptingNewActivities ? "true": "false");
    doc.NewChild("bes-factory:TotalNumberOfActivities") = Arc::tostring(sched_queue.size());
    std::map<const std::string, Job *> all_job = sched_queue.getAllJobs();
    std::map<const std::string, Job *>::iterator iter;
    for (iter = all_job.begin(); iter != all_job.end(); iter++) {
        Arc::WSAEndpointReference identifier(doc.NewChild("bes-factory:ActivityReference"));
        // Make job's ID
        identifier.Address(endpoint); // address of service
        identifier.ReferenceParameters().NewChild("sched:JobID") = iter->first;
    };

    doc.NewChild("bes-factory:TotalNumberOfContainedResources") = Arc::tostring(sched_resources.size());
    doc.NewChild("bes-factory:NamingProfile") = "http://schemas.ggf.org/bes/2006/08/bes/naming/BasicWSAddressing";
    doc.NewChild("bes-factory:BESExtension") = "http://www.nordugrid.org/schemas/a-rex";
    doc.NewChild("bes-factory:LocalResourceManagerType") = "uri:unknown";
    {
        std::string s;
        out.GetXML(s);
        logger.msg(Arc::DEBUG, "GetFactoryAttributesDocument: response = \n%s", s);
    };
    return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 

