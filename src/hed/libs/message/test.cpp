#include "SOAPEnvelope.h"

const char* xml_str = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SOAP-ENV:Envelope \
xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" \
xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" \
xmlns:a-rex=\"http://www.nordugrid.org/schemas/a-rex\" \
xmlns:bes-factory=\"http://schemas.ggf.org/bes/2006/08/bes-factory\" \
xmlns:jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\" \
xmlns:wsa=\"http://www.w3.org/2005/08/addressing\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><SOAP-ENV:Body><bes-factory:CreateActivity><bes-factory:ActivityDocument><JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\"> \
    <JobDescription> \
        <JobIdentification> \
            <JobName>Demo</JobName> \
            <JobProject>HPCPInterop</JobProject> \
        </JobIdentification> \
        <Application> \
            <HPCProfileApplication xmlns=\"http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa\"> \
                <Executable>/home/interop/demo_job.sh</Executable> \
                <Argument>60</Argument> \
                <Output>demo_job.out.%J</Output> \
                <WorkingDirectory>/home/interop/username</WorkingDirectory> \
            </HPCProfileApplication> \
        </Application> \
        <Resources> \
            <TotalCPUCount> \
                <Exact>1</Exact> \
            </TotalCPUCount> \
        </Resources> \
    </JobDescription> \
</JobDefinition></bes-factory:ActivityDocument></bes-factory:CreateActivity></SOAP-ENV:Body></SOAP-ENV:Envelope>";

int main(void) {
  Arc::SOAPEnvelope soap(xml_str);
  return 0;
}
