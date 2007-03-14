#include <string>
#include <list>
#include <map>
#include <iostream>

#include "SOAPMessage.h"

void printNode(Arc::XMLNode& node,int skip) {
  for(int n=0;n<skip;n++) std::cout<<" ";
  std::string content = (std::string)node;
  std::cout<<"* "<<node.Name()<<"("<<node.Size()<<")"<<" = "<<content<<std::endl;
  for(int n=0;;n++) {
    Arc::XMLNode node_ = node.Child(n);
    if(!node_) break;
    printNode(node_,skip+2);
  };
}

/*
int main(void) {
  std::string xml=
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<SOAP-ENV:Envelope\n"
" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\"\n"
" xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"\n"
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
" xmlns:ns=\"http://www.nordugrid.org/ws/schemas/ARCStorageElement\">\n"
" <SOAP-ENV:Body>\n"
"  <ns:add>\n"
"   <file>\n"
"    <id>ID</id>\n"
"    <size>0</size>\n"
"    <checksum>CHECKSUM</checksum>\n"
"    <acl>ACL</acl>\n"
"    <created>CREATED</created>\n"
"    <state>STATE</state>\n"
"    <url>URL</url>\n"
"   </file>\n"
"   <source>SOURCE</source>\n"
"  </ns:add>\n"
" </SOAP-ENV:Body>\n"
"</SOAP-ENV:Envelope>";
  Arc::SOAPMessage node(xml);

  //Arc::XMLNode::NS ns;
  //ns["soap-enc"]="http://schemas.xmlsoap.org/soap/encoding/";
  //ns["soap-env"]="http://schemas.xmlsoap.org/soap/envelope/";
  //ns["ARC"]="http://www.nordugrid.org/schemas/arc/";
  //node.Namespaces(ns);

  printNode(node,0);
  //node["Envelope"]["Body"]["add"]="ADD";
  node["add"].NewChild("source",0)="SOURCE2";
  printNode(node,0);
  return 0;
}
*/

std::string bes_example_request = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<SOAP-ENV:Envelope\n"
" xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"\n"
" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\"\n"
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
" xmlns:ns2=\"http://www.w3.org/2005/08/addressing\"\n"
" xmlns:ns3=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\"\n"
" xmlns:ns1=\"http://schemas.ggf.org/bes/2006/08/bes-factory\">\n"
" <SOAP-ENV:Body>\n"
"   <ns1:CreateActivity>\n"
"    <ns1:ActivityDocument>\n"
"     <ns3:JobDefinition id=\"\" anyAttribute=\"\">\n"
"      <ns3:JobDescription anyAttribute=\"\">\n"
"       <ns3:JobIdentification anyAttribute=\"\">\n"
"        <ns3:JobName></ns3:JobName>\n"
"        <ns3:Description></ns3:Description>\n"
"        <ns3:JobAnnotation></ns3:JobAnnotation>\n"
"        <ns3:JobProject></ns3:JobProject>\n"
"       </ns3:JobIdentification>\n"
"       <ns3:Application anyAttribute=\"\">\n"
"        <ns3:ApplicationName></ns3:ApplicationName>\n"
"        <ns3:ApplicationVersion></ns3:ApplicationVersion>\n"
"        <ns3:Description></ns3:Description>\n"
"       </ns3:Application>\n"
"       <ns3:Resources anyAttribute=\"\">\n"
"        <ns3:CandidateHosts>\n"
"         <ns3:HostName></ns3:HostName>\n"
"        </ns3:CandidateHosts>\n"
"        <ns3:FileSystem>\n"
"         <ns3:FileSystemType>swap</ns3:FileSystemType>\n"
"         <ns3:Description></ns3:Description>\n"
"         <ns3:MountPoint></ns3:MountPoint>\n"
"         <ns3:DiskSpace anyAttribute=\"\">\n"
"          <ns3:UpperBoundedRange xsi:nil=\"true\"></ns3:UpperBoundedRange>\n"
"          <ns3:LowerBoundedRange xsi:nil=\"true\"></ns3:LowerBoundedRange>\n"
"          <ns3:Exact></ns3:Exact>\n"
"          <ns3:Range></ns3:Range>\n"
"         </ns3:DiskSpace>\n"
"        </ns3:FileSystem>\n"
"        <ns3:ExclusiveExecution>false</ns3:ExclusiveExecution>\n"
"        <ns3:OperatingSystem anyAttribute=\"\">\n"
"         <ns3:OperatingSystemType anyAttribute=\"\">\n"
"          <ns3:OperatingSystemName>Unknown</ns3:OperatingSystemName>\n"
"         </ns3:OperatingSystemType>\n"
"         <ns3:OperatingSystemVersion></ns3:OperatingSystemVersion>\n"
"         <ns3:Description></ns3:Description>\n"
"        </ns3:OperatingSystem>\n"
"        <ns3:CPUArchitecture anyAttribute=\"\">\n"
"         <ns3:CPUArchitectureName>sparc</ns3:CPUArchitectureName>\n"
"        </ns3:CPUArchitecture>\n"
"        <ns3:IndividualCPUSpeed anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualCPUSpeed>\n"
"        <ns3:IndividualCPUTime anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualCPUTime>\n"
"        <ns3:IndividualCPUCount anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualCPUCount>\n"
"        <ns3:IndividualNetworkBandwidth anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualNetworkBandwidth>\n"
"        <ns3:IndividualPhysicalMemory anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualPhysicalMemory>\n"
"        <ns3:IndividualVirtualMemory anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualVirtualMemory>\n"
"        <ns3:IndividualDiskSpace anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:IndividualDiskSpace>\n"
"        <ns3:TotalCPUTime anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:TotalCPUTime>\n"
"        <ns3:TotalCPUCount anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:TotalCPUCount>\n"
"        <ns3:TotalPhysicalMemory anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:TotalPhysicalMemory>\n"
"        <ns3:TotalVirtualMemory anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:TotalVirtualMemory>\n"
"        <ns3:TotalDiskSpace anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:TotalDiskSpace>\n"
"        <ns3:TotalResourceCount anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:TotalResourceCount>\n"
"       </ns3:Resources>\n"
"       <ns3:DataStaging>\n"
"        <ns3:FileName></ns3:FileName>\n"
"        <ns3:FilesystemName></ns3:FilesystemName>\n"
"        <ns3:CreationFlag>overwrite</ns3:CreationFlag>\n"
"        <ns3:DeleteOnTermination>false</ns3:DeleteOnTermination>\n"
"        <ns3:Source anyAttribute=\"\">\n"
"         <ns3:URI></ns3:URI>\n"
"        </ns3:Source>\n"
"        <ns3:Target anyAttribute=\"\">\n"
"         <ns3:URI></ns3:URI>\n"
"        </ns3:Target>\n"
"       </ns3:DataStaging>\n"
"      </ns3:JobDescription>\n"
"     </ns3:JobDefinition>\n"
"    </ns1:ActivityDocument>\n"
"   </ns1:CreateActivity>\n"
" </SOAP-ENV:Body>\n"
"</SOAP-ENV:Envelope>\n";

std::string bes_example_response = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<SOAP-ENV:Envelope\n"
" xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"\n"
" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\"\n"
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
" xmlns:ns2=\"http://www.w3.org/2005/08/addressing\"\n"
" xmlns:ns3=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\"\n"
" xmlns:ns1=\"http://schemas.ggf.org/bes/2006/08/bes-factory\">\n"
" <SOAP-ENV:Body>\n"
"  <ns1:CreateActivityResponse>\n"
"   <ns1:ActivityIdentifier anyAttribute=\"\">\n"
"    <ns2:Address anyAttribute=\"\">gsiftp://grid.uio.no:2811/1234567890</ns2:Address>\n"
"    <ns2:ReferenceParameters anyAttribute=\"\">\n"
"    </ns2:ReferenceParameters>\n"
"    <ns2:Metadata anyAttribute=\"\">\n"
"    </ns2:Metadata>\n"
"   </ns1:ActivityIdentifier>\n"
"   <ns1:ActivityDocument>\n"
"    <ns3:JobDefinition id=\"\" anyAttribute=\"\">\n"
"     <ns3:JobDescription anyAttribute=\"\">\n"
"      <ns3:JobIdentification anyAttribute=\"\">\n"
"       <ns3:JobName></ns3:JobName>\n"
"       <ns3:Description></ns3:Description>\n"
"       <ns3:JobAnnotation></ns3:JobAnnotation>\n"
"       <ns3:JobProject></ns3:JobProject>\n"
"      </ns3:JobIdentification>\n"
"      <ns3:Application anyAttribute=\"\">\n"
"       <ns3:ApplicationName></ns3:ApplicationName>\n"
"       <ns3:ApplicationVersion></ns3:ApplicationVersion>\n"
"       <ns3:Description></ns3:Description>\n"
"      </ns3:Application>\n"
"      <ns3:Resources anyAttribute=\"\">\n"
"       <ns3:CandidateHosts>\n"
"        <ns3:HostName></ns3:HostName>\n"
"       </ns3:CandidateHosts>\n"
"       <ns3:FileSystem>\n"
"        <ns3:FileSystemType>swap</ns3:FileSystemType>\n"
"        <ns3:Description></ns3:Description>\n"
"        <ns3:MountPoint></ns3:MountPoint>\n"
"        <ns3:DiskSpace anyAttribute=\"\">\n"
"         <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"         <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"         <ns3:Exact></ns3:Exact>\n"
"         <ns3:Range>\n"
"          <ns3:LowerBound xsi:nil=\"true\"></ns3:LowerBound>\n"
"          <ns3:UpperBound xsi:nil=\"true\"></ns3:UpperBound>\n"
"         </ns3:Range>\n"
"        </ns3:DiskSpace>\n"
"       </ns3:FileSystem>\n"
"       <ns3:ExclusiveExecution>false</ns3:ExclusiveExecution>\n"
"       <ns3:OperatingSystem anyAttribute=\"\">\n"
"        <ns3:OperatingSystemType anyAttribute=\"\">\n"
"         <ns3:OperatingSystemName>Unknown</ns3:OperatingSystemName>\n"
"        </ns3:OperatingSystemType>\n"
"        <ns3:OperatingSystemVersion></ns3:OperatingSystemVersion>\n"
"        <ns3:Description></ns3:Description>\n"
"       </ns3:OperatingSystem>\n"
"       <ns3:CPUArchitecture anyAttribute=\"\">\n"
"        <ns3:CPUArchitectureName>sparc</ns3:CPUArchitectureName>\n"
"       </ns3:CPUArchitecture>\n"
"       <ns3:IndividualCPUSpeed anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualCPUSpeed>\n"
"       <ns3:IndividualCPUTime anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualCPUTime>\n"
"       <ns3:IndividualCPUCount anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualCPUCount>\n"
"       <ns3:IndividualNetworkBandwidth anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualNetworkBandwidth>\n"
"       <ns3:IndividualPhysicalMemory anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualPhysicalMemory>\n"
"       <ns3:IndividualVirtualMemory anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualVirtualMemory>\n"
"       <ns3:IndividualDiskSpace anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:IndividualDiskSpace>\n"
"       <ns3:TotalCPUTime anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:TotalCPUTime>\n"
"       <ns3:TotalCPUCount anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:TotalCPUCount>\n"
"       <ns3:TotalPhysicalMemory anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:TotalPhysicalMemory>\n"
"       <ns3:TotalVirtualMemory anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:TotalVirtualMemory>\n"
"       <ns3:TotalDiskSpace anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:TotalDiskSpace>\n"
"       <ns3:TotalResourceCount anyAttribute=\"\">\n"
"        <ns3:UpperBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBoundedRange>\n"
"        <ns3:LowerBoundedRange exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBoundedRange>\n"
"        <ns3:Exact></ns3:Exact>\n"
"        <ns3:Range>\n"
"         <ns3:LowerBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:LowerBound>\n"
"         <ns3:UpperBound exclusiveBound=\"\" anyAttribute=\"\"></ns3:UpperBound>\n"
"        </ns3:Range>\n"
"       </ns3:TotalResourceCount>\n"
"      </ns3:Resources>\n"
"      <ns3:DataStaging>\n"
"       <ns3:FileName></ns3:FileName>\n"
"       <ns3:FilesystemName></ns3:FilesystemName>\n"
"       <ns3:CreationFlag>overwrite</ns3:CreationFlag>\n"
"       <ns3:DeleteOnTermination>false</ns3:DeleteOnTermination>\n"
"       <ns3:Source anyAttribute=\"\">\n"
"        <ns3:URI></ns3:URI>\n"
"       </ns3:Source>\n"
"       <ns3:Target anyAttribute=\"\">\n"
"        <ns3:URI></ns3:URI>\n"
"       </ns3:Target>\n"
"      </ns3:DataStaging>\n"
"     </ns3:JobDescription>\n"
"    </ns3:JobDefinition>\n"
"   </ns1:ActivityDocument>\n"
"  </ns1:CreateActivityResponse>\n"
" </SOAP-ENV:Body>\n"
"</SOAP-ENV:Envelope>\n";

class master1 {
 public:
  int val;
  master1(void):val(1) { };
  virtual ~master1(void) { };
};

class child1: public master1 {
 public:
  child1(void) { };
  virtual ~child1(void) { };
};

class child2: public master1 {
 public:
  child2(void) { };
  virtual ~child2(void) { };
};




int main(void) {
  std::string xml;
  Arc::XMLNode::NS ns;
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
  ns["bes"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
  Arc::SOAPMessage req(ns);
  Arc::XMLNode job = req.NewChild("bes:CreateActivity").\
                    NewChild("bes:ActivityDocument").\
                    NewChild("jsdl:JobDefinition").\
                    NewChild("jsdl:JobDescription");
  job.NewChild("jsdl:JobIdentification").NewChild("jsdl:JobName")="TEST";
  job.NewChild("jsdl:Application").NewChild("jsdl:ApplicationName")="/bin/echo";
  job.NewAttribute("jsdl:id")="unknown";
  printNode(req,0);
  req.GetXML(xml);
  std::cout<<xml<<std::endl;

  Arc::SOAPMessage resp(bes_example_response);
  printNode(resp,0);
  Arc::XMLNode Id = resp["CreateActivityResponse"]["ActivityIdentifier"];
  std::string addr = Id["Address"];
  std::cout<<"Job address: "<<addr<<std::endl;
  //resp.GetXML(xml);
  //std::cout<<xml<<std::endl;

  Arc::XMLNode::NS ns_;
  Arc::SOAPMessage fault(ns_,true);
  fault.Fault()->Subcode(1,"Unexpected");
  fault.Fault()->Reason("Failed processing request");
  fault.GetXML(xml);
  std::cout<<xml<<std::endl;

  child1 m;
  master1& t = m;
  try {
    child2& c = dynamic_cast<child2&>(t);
  } catch(std::exception& e) { };


  return 0;
}

