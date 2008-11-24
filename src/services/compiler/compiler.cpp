#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/stat.h>
#include <errno.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/data/DMC.h>
#include <arc/data/DataHandle.h>

#include <arc/data/DataMover.h>
#include <glibmm.h>
#include <arc/data/URLMap.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <sstream>

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/security/SecHandler.h>
#include <arc/URL.h>

#include "arex_client.h"

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "compiler.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Compiler::Service_Compiler(cfg);
}

service_descriptors ARC_SERVICE_LOADER = {
    { "compiler", 0, &get_service },
    { NULL, 0, NULL }
};

using namespace Compiler;

Arc::Logger local_logger(Arc::Logger::getRootLogger(), "Compiler");

struct CompileInfo {
    std::string site_url;
    std::string script_url;
    std::string architecture;
    std::string makefile_name;
    std::string cpu_number;
    std::string parameters;
    bool tar;
    std::string compiler;
    bool march;
    std::string job_name;
    std::vector<std::string> sources;
};
	
struct ISIS_Info {
    std::string arex_url;
    std::string url_architecture;
    std::string url_cpu_number;
};
	
	
//to the file transfer
static bool html_to_list(const char* html,const Arc::URL base,std::list<Arc::URL>& urls) {
  for(const char* pos = html;;) {
    // Looking for tag
    const char* tag_start = strchr(pos,'<');
    if(!tag_start) break; // No more tags
    // Looking for end of tag
    const char* tag_end = strchr(tag_start+1,'>');
    if(!tag_end) return false; // Broken html?
    // 'A' tag?
    if(strncasecmp(tag_start,"<A ",3) == 0) {
      // Lookig for HREF
      const char* href = strstr(tag_start+3,"href=");
      if(!href) href=strstr(tag_start+3,"HREF=");
      if(href) {
        const char* url_start = href+5;
        const char* url_end = NULL;
        if((*url_start) == '"') {
          ++url_start;
          url_end=strchr(url_start,'"');
          if((!url_end) || (url_end > tag_end)) url_end=NULL;
        } else if((*url_start) == '\'') {
          url_end=strchr(url_start,'\'');
          if((!url_end) || (url_end > tag_end)) url_end=NULL;
        } else {
          url_end=strchr(url_start,' ');
          if((!url_end) || (url_end > tag_end)) url_end=tag_end;
        };
        if(!url_end) return false; // Broken HTML
        std::string url_str(url_start,url_end-url_start);
        Arc::URL url(url_str);
        if(!url) return false; // Bad URL
        if((url.Protocol() == base.Protocol()) &&
           (url.Host() == base.Host()) &&
           (url.Port() == base.Port())) {
          std::string path = url.Path();
          std::string base_path = base.Path();
          if(base_path.length() < path.length()) {
            if(strncmp(base_path.c_str(),path.c_str(),base_path.length()) == 0) {
              urls.push_back(url);
            };
          };
        };
      };
    };
    pos=tag_end+1;
  };
  return true;
}

static bool PayloadRaw_to_string(Arc::PayloadRawInterface& buf,std::string& str,uint64_t& end) {
  end=0;
  for(int n = 0;;++n) {
    const char* content = buf.Buffer(n);
    if(!content) break;
    int size = buf.BufferSize(n);
    if(size > 0) {
      str.append(content,size);
    };
    end=buf.BufferPos(n)+size;
  };
  return true;
}

static bool PayloadRaw_to_stream(Arc::PayloadRawInterface& buf,std::ostream& o,uint64_t& end) {
  end=0;
  for(int n = 0;;++n) {
    const char* content = buf.Buffer(n);
    if(!content) break;
    uint64_t size = buf.BufferSize(n);
    uint64_t pos = buf.BufferPos(n);
    if(size > 0) {
      o.seekp(pos);
      o.write(content,size);
      if(!o) return false;
    };
    end=pos+size;
  };
  return true;
}

// TODO: limit recursion depth
static bool get_file(Arc::ClientHTTP& client,const Arc::URL& url,const std::string& dir) {
  // Read file in chunks. Use first chunk to determine type of file.
  Arc::PayloadRaw req; // Empty request body
  Arc::PayloadRawInterface* resp;
  Arc::HTTPClientInfo info;
  const uint64_t chunk_len = 1024*1024; // Some reasonable size - TODO - make ir configurable
  uint64_t chunk_start = 0;
  uint64_t chunk_end = chunk_len;
  // First chunk
  Arc::MCC_Status r = client.process("GET",url.Path(),chunk_start,chunk_end,&req,&info,&resp);
  if(!r) return false;
  if(!resp) return false;
  if((info.code != 200) && (info.code != 206)) { delete resp; return false; };
  if(strcasecmp(info.type.c_str(),"text/html") == 0) {
    std::string html;
    uint64_t file_size = resp->Size();
    PayloadRaw_to_string(*resp,html,chunk_end);
    delete resp; resp=NULL;
    // Fetch whole html
    for(;chunk_end<file_size;) {
      chunk_start=chunk_end;
      chunk_end=chunk_start+chunk_len;
      r=client.process("GET",url.Path(),chunk_start,chunk_end,&req,&info,&resp);
      if(!r) return false;
      if(!resp) return false;
      if(resp->Size() <= 0) break;
      if((info.code != 200) && (info.code != 206)) { delete resp; return false; };
      PayloadRaw_to_string(*resp,html,chunk_end);
      delete resp; resp=NULL;
    };
    if(resp) delete resp;
    // Make directory
    if(mkdir(dir.c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST)  throw std::invalid_argument("Failed to create local directory "+dir+" !");
    };
    // Fetch files
    std::list<Arc::URL> urls;
    if(!html_to_list(html.c_str(),url,urls)) return false;
    for(std::list<Arc::URL>::iterator u = urls.begin();u!=urls.end();++u) {
      if(!get_file(client,*u,dir+(u->Path().substr(url.Path().length())))) return false;
    };
    return true;
  };
  // Start storing file
  uint64_t file_size = resp->Size();
  std::ofstream f(dir.c_str(),std::ios::trunc);
  if(!f) { delete resp; return false; };
  if(!PayloadRaw_to_stream(*resp,f,chunk_end)) { delete resp; return false; };
  delete resp; resp=NULL;
  // Continue fetching file
  for(;chunk_end<file_size;) {
    chunk_start=chunk_end;
    chunk_end=chunk_start+chunk_len;
    r=client.process("GET",url.Path(),chunk_start,chunk_end,&req,&info,&resp);
    if(!r) return false;
    if(!resp) return false;
    if(resp->Size() <= 0) break;
    if((info.code != 200) && (info.code != 206)) { delete resp; return false; };
    if(!PayloadRaw_to_stream(*resp,f,chunk_end)) { delete resp; return false; };
    delete resp; resp=NULL;
  };
  if(resp) delete resp;
  return true;
}

//end of "to the file transfer"

/*  URL => File name
       input: URL
       return value: File name
*/  
std::string File_name(std::string path){
     std::string file_name;
     size_t pos;
     pos = path.find("/");                            // position of "//" in path
     file_name = path.substr (pos+2);                 // get from "//" to the end

     pos = file_name.find("/");                       // position of "/" in path	     
     while( (unsigned int)pos != (unsigned int)-1){   //4294967295 = (2^32)-1=-1
	 file_name = file_name.substr (pos+1);        // get from "/" to the end
	 pos = file_name.find("/");                   // position of "/" in  file_name	 
     }    
     return file_name;
}	
	
	
/*Job submit
  input:  job name
          URL
          script URL
          sources
          architecture
          makefile's name
          CPU number
          others parameters
          source compressed
          compile type
          march: gcc option
		  
  return value: Job ID
  */
std::string job_submit(const std::string job_name, const std::string site_url, const std::string script_url, 
		       std::vector<std::string> sources, const std::string architecture="", 
		       const std::string makefile="",   const std::string cpu_number="1",
		       const std::string parameters="", bool tar = true,  std::string compiler="make", 
		       bool march = false   ){
    std::string jobid("");
    // call parameter check
    if( site_url == "" || script_url == "" ) return jobid;
    try{
	Arc::URL url(site_url);
	if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
    
	Arc::MCCConfig cfg;
	/*if(!tool.proxy_path.empty()) cfg.AddProxy(tool.proxy_path);
        if(!tool.key_path.empty()) cfg.AddPrivateKey(tool.key_path);
        if(!tool.cert_path.empty()) cfg.AddCertificate(tool.cert_path);
        if(!tool.ca_dir.empty()) cfg.AddCADir(tool.ca_dir);
        cfg.GetOverlay(tool.config_path);*/
	Arc::AREXClient ac(url,cfg);
    
	std::string jsdl_file;
	jsdl_file = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"; 
	jsdl_file +="<JobDefinition \n";
	jsdl_file +=" xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\"\n";
	jsdl_file +=" xmlns:posix=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">\n";
	jsdl_file +=" <JobDescription>\n";
	jsdl_file +="   <JobIdentification>\n";
	jsdl_file +="     <JobName>";
	    jsdl_file +=job_name;
	    jsdl_file +="</JobName>\n";
	jsdl_file +="   </JobIdentification>\n";
	jsdl_file +="   <Application>\n";
	jsdl_file +="     <posix:POSIXApplication>\n";
	jsdl_file +="       <posix:Executable>compiler_script</posix:Executable>\n";
	if( tar )	      jsdl_file +="          <posix:Argument>-t</posix:Argument>\n";
	if( compiler =="make" ){
	      jsdl_file +="          <posix:Argument>make</posix:Argument>\n";
	      if ( makefile != ""){
	         jsdl_file +="          <posix:Argument>-m</posix:Argument>\n";
	         jsdl_file +="          <posix:Argument>";
	         jsdl_file +=makefile;
	         jsdl_file +="</posix:Argument>\n";
	      }
	}	
	else{        //gcc
	      jsdl_file +="          <posix:Argument>gcc</posix:Argument>\n";	
	      if ( cpu_number != "1" ){
		 jsdl_file +="          <posix:Argument>-cpu</posix:Argument>\n";	
		 jsdl_file +="          <posix:Argument>";
		 jsdl_file +=cpu_number;
		 jsdl_file +="</posix:Argument>\n";
	      }
	      if ( parameters != ""){			//gcc parameters
		 jsdl_file +="          <posix:Argument>-opt</posix:Argument>\n";	
		 jsdl_file +="          <posix:Argument>";
		 jsdl_file +=parameters;
		 jsdl_file +="</posix:Argument>\n";
	      }
	      jsdl_file +="          <posix:Argument>";
	      jsdl_file +=File_name(makefile);		//now this is the gcc sequence file's name
	      jsdl_file +="</posix:Argument>\n";
	}
        std::vector<std::string>::iterator it_argname; 
	for ( it_argname=sources.begin() ; it_argname < sources.end(); it_argname++ ){
	    jsdl_file +="       <posix:Argument>";
	    jsdl_file += File_name(*it_argname);
	    jsdl_file += "</posix:Argument>\n";
	}    
	//jsdl_file +="       <posix:Argument>";	//not working yet
	//jsdl_file +=cpu_number;
	//jsdl_file +="</posix:Argument>\n";
                std::vector<std::string>::iterator it_name; 
	for ( it_name=sources.begin() ; it_name < sources.end(); it_name++ ){
	    jsdl_file +="       <posix:Input>";
	    jsdl_file += File_name(*it_name);
	    jsdl_file += "</posix:Input>\n";
	}    
	jsdl_file +="       <posix:Output>out.txt</posix:Output>\n";
	jsdl_file +="       <posix:Error>err.txt</posix:Error>\n";
	jsdl_file +="     </posix:POSIXApplication>\n";
	jsdl_file +="   </Application>\n";
	
	if(architecture != ""){
	    jsdl_file +="   <Resource>\n"; 
	    jsdl_file +="   <CPUArchitecture>\n";
	    jsdl_file +="         <CPUArchitectureName>";
	    jsdl_file +=architecture;
	    jsdl_file +="</CPUArchitectureName>\n";
	    jsdl_file +="   </CPUArchitecture>\n";
	    jsdl_file +="   </Resource>\n"; 
	}
	if( compiler =="gcc" ){
	    jsdl_file +="   <DataStaging>\n";
	    jsdl_file +="     <FileName>";
	    jsdl_file +=File_name(makefile);
	    jsdl_file +="</FileName>\n";
	    jsdl_file +="       <Source><URI>";
	    jsdl_file +=makefile;
	    jsdl_file +="</URI></Source>\n";
	    jsdl_file +="     <DeleteOnTermination>false</DeleteOnTermination>\n";
	    jsdl_file +="   </DataStaging>\n";
	}    
	
	jsdl_file +="   <DataStaging>\n";
	jsdl_file +="     <FileName>compiler_script</FileName>\n";
	jsdl_file +="       <Source><URI>";
	jsdl_file +=script_url;
	
	local_logger.msg(Arc::DEBUG, "file_name:     " + File_name(script_url));
 
	
	jsdl_file +="</URI></Source>\n";
	jsdl_file +="     <DeleteOnTermination>false</DeleteOnTermination>\n";
	jsdl_file +="   </DataStaging>\n";
        std::vector<std::string>::iterator it; 
	for ( it=sources.begin() ; it < sources.end(); it++ ){
	      jsdl_file +="   <DataStaging>\n";
	      jsdl_file +="     <FileName>";
	      jsdl_file += File_name(*it);
	      jsdl_file +="</FileName>\n";
	      jsdl_file +="       <Source><URI>";
	      jsdl_file+=  *it;
	      jsdl_file += "</URI></Source>\n";
	      jsdl_file +="     <DeleteOnTermination>false</DeleteOnTermination>\n";
	      jsdl_file +="   </DataStaging>\n";
        }
	jsdl_file +="   <DataStaging>\n";
	jsdl_file +="     <FileName>outputs</FileName>\n";
	jsdl_file +="     <DeleteOnTermination>false</DeleteOnTermination>\n";
	jsdl_file +="   </DataStaging>\n";
        jsdl_file +="   <DataStaging>\n";
	jsdl_file +="     <FileName>out.txt</FileName>\n";
	jsdl_file +="     <CreationFlag>overwrite</CreationFlag>\n";
	jsdl_file +="     <DeleteOnTermination>false</DeleteOnTermination>\n";
	jsdl_file +="   </DataStaging>\n";
	jsdl_file +="   <DataStaging>\n";
	jsdl_file +="     <FileName>err.txt</FileName>\n";
	jsdl_file +="     <CreationFlag>overwrite</CreationFlag>\n";
	jsdl_file +="     <DeleteOnTermination>false</DeleteOnTermination>\n";
	jsdl_file +="   </DataStaging>\n";
	jsdl_file +=" </JobDescription>\n";
	jsdl_file +="</JobDefinition>";      

	local_logger.msg(Arc::DEBUG, "Jsdl:  " + jsdl_file);

	//creating the temporary jsdl file
	std::time_t rawtime;
	std::time ( &rawtime );	//current time
	tm * ptm;
	ptm = gmtime ( &rawtime );
	
	std::stringstream out;
	out << ptm->tm_year<< ptm->tm_mon<< ptm->tm_mday<< ptm->tm_hour<< ptm->tm_min<< ptm->tm_sec;
	std::string local_string;
	local_string = out.str() +job_name+ ".jsdl";

	char  jsdl_file_name[ local_string.size()];
	for(size_t i = 0; i < local_string.size(); i++)
	{
	    jsdl_file_name[i] = local_string[i];
	}
	jsdl_file_name[local_string.size()]='\0';		//close character

	local_logger.msg(Arc::DEBUG, "The submited JSDL file's name: " + (std::string)jsdl_file_name);
	std::ofstream jsdlfile_o;
	jsdlfile_o.open((const char *)&jsdl_file_name);
	jsdlfile_o << jsdl_file;
	jsdlfile_o.close();
	
	std::ifstream jsdlfile((const char *)&jsdl_file_name);
	if (!jsdlfile)
	    throw std::invalid_argument(std::string("Could not open ")+
				             std::string(jsdl_file_name));
	
	Arc::AREXFileList files;
    
	// Submit job description
	bool delegate = false;
	jobid = ac.submit(jsdlfile, files, delegate);
	if (!jsdlfile)
	    throw std::invalid_argument(std::string("Failed when reading from ")+
					std::string(jsdl_file_name)); 
	
	remove((const char *)&jsdl_file_name);
	local_logger.msg(Arc::INFO, "Jod Id:  "+ jobid);

    }  catch(std::exception& err) {
	std::cerr << "ERROR: " << err.what() << std::endl;
        return "";
    };
    return  jobid;    
}

/*
     URL, architecture, CPU count, etc informations from the ISIS
  */
std::vector<CompileInfo> Info_from_ISIS(Arc::XMLNode soap_xml, Service_Compiler service){
    std::vector<CompileInfo> return_List;     
       
    //ISIS and SOAP
    std::vector<ISIS_Info> info_vector;
      
    //they will be come from the ISIS, when working it
    ISIS_Info info;
    info.arex_url = "http://knowarc1.grid.niif.hu:50000/arex";
    info.url_cpu_number = "1";
    info.url_architecture= "x86_32";                  //etc.: ia64,x86_32,sparc
    if ((std::string)soap_xml["make"]["cpu_architecture"] != "")
       info.url_architecture = (std::string)soap_xml["make"]["cpu_architecture"];
	
    info_vector.push_back(info);
    //end of the connection with the ISIS
       
       
    std::string soap_compiler(soap_xml["make"]["compiler_type"]); 
    std::string not_default_make_file(soap_xml["make"]["makefile"]);
    std::string gcc_sequence_file(soap_xml["make"]["gcc_sequence"]);
    std::string gcc_parameters(soap_xml["make"]["gcc_parameters"]);
       
    bool tar = false;
    if((std::string)soap_xml["make"]["compressed"] == "yes" ||
       (std::string)soap_xml["make"]["compressed"] == "YES" ||
       (std::string)soap_xml["make"]["compressed"] == "y" ||
       (std::string)soap_xml["make"]["compressed"] == "Y" ||
       (std::string)soap_xml["make"]["compressed"] == "1" ) tar = true;
    const std::string j_name(soap_xml["make"]["name"]);
    std::vector<std::string> sources;
    bool march = false;	//TODO: not working yet
       
    int j= 0;	
    while( soap_xml["make"]["sourcefile"][j] != 0 ){    	   
	std::stringstream tmp;
	tmp << j;
	sources.push_back( soap_xml["make"]["sourcefile"][j] );
	local_logger.msg(Arc::DEBUG, "sourcefile"+tmp.str()+ ":  "+ (std::string)soap_xml["make"]["sourcefile"][j]);
        j++;
	if(tar) break;
    }
    //end of ISIS and SOAP
       
    std::vector<ISIS_Info>::iterator it; 
    for ( it=info_vector.begin(); it < info_vector.end(); it++ ){
        CompileInfo compile_info;
	compile_info.site_url =  (*it).arex_url;
	compile_info.script_url = service.Get_Script_Url();
	compile_info.architecture = (*it).url_architecture;
	compile_info.makefile_name=not_default_make_file;
        if ( soap_compiler == "gcc"){
	   compile_info.makefile_name=gcc_sequence_file;
	}
	compile_info.cpu_number = (*it).url_cpu_number;
	compile_info.parameters = gcc_parameters;
	compile_info.tar = tar;
	compile_info.compiler = soap_compiler; 
	compile_info.march = march;
	compile_info.job_name = j_name;
	compile_info.sources = sources;
   
	return_List.push_back(compile_info);
    }
       
    return return_List;
}

/*
     When the Job status is Finished, then this function returned with "true",  otherwise returned with "false".
     
     input: Job ID
     return value: true/false
*/
bool Job_Status_Finished(std::string jobid){
      std::string urlstr;
      Arc::XMLNode jobxml(jobid);
      if(!jobxml)
        throw std::invalid_argument("Could not process Job ID from " + jobid);
      urlstr=(std::string)(jobxml["Address"]); // TODO: clever service address extraction
      
      if(urlstr.empty())
      throw std::invalid_argument("Missing service URL");
      Arc::URL url(urlstr);
      if(!url)
                throw std::invalid_argument("Can't parse service URL " + urlstr);
      Arc::MCCConfig cfg;
      //    if(!proxy_path.empty()) cfg.AddProxy(proxy_path);
      //    if(!key_path.empty()) cfg.AddPrivateKey(key_path);
      //    if(!cert_path.empty()) cfg.AddCertificate(cert_path);
      //    if(!ca_dir.empty()) cfg.AddCADir(ca_dir);
      //    cfg.GetOverlay(config_path);
      Arc::AREXClient ac(url,cfg);
      std::string status= ac.stat(jobid);
      local_logger.msg(Arc::INFO, "STATUS: " + status);
 
      if ( status.substr(0,8) == "Finished" ||                    // status ="Finished/Finished" or "Finished/Deleted"
           status.substr(0,6) == "Failed"  ||                     // status ="Failed/Failed" or "Failed/Deleted"
           status.substr(0,9) == "Cancelled" ) return true;       // status ="Cancelled/Failed" or "Cancelled/Deleted"
      return false; 
}

Arc::Logger Service_Compiler::logger(Arc::Logger::getRootLogger(), "Compiler");

Service_Compiler::Service_Compiler(Arc::Config *cfg):Service(cfg) {
  ns_["compiler"]="urn:compiler";
  script_url_=(std::string)((*cfg)["scriptfile_url"]);
}

Service_Compiler::~Service_Compiler(void) {
}

Arc::MCC_Status Service_Compiler::make_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::GENERIC_ERROR);
}

Arc::MCC_Status Service_Compiler::process(Arc::Message& inmsg,Arc::Message& outmsg) {
   // Both input and output are supposed to be SOAP 
   // Extracting payload
   Arc::PayloadSOAP* inpayload = NULL;
   try {
     inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
   } catch(std::exception& e) { };
   if(!inpayload) {
     logger.msg(Arc::ERROR, "Input is not SOAP");
     return make_fault(outmsg);
   };
   // Analyzing request 
   Arc::XMLNode compiler_op = (*inpayload)["compiler"];
   if(!compiler_op) {
     logger.msg(Arc::ERROR, "Request is not supported - %s", compiler_op.Name());
     return make_fault(outmsg);
   };
  
   std::string xml;
   inpayload->GetDoc(xml, true);
   std::cout << "XML: "<< xml << std::endl;
   
   Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
   Arc::XMLNode outpayload_node = outpayload->NewChild("compiler:compilerResponse");

   //info from the ISIS
   logger.msg(Arc::DEBUG, "Info from the ISIS");
   std::vector<CompileInfo> Site_List;
   
   Site_List = Info_from_ISIS(compiler_op, *this);

   //job submit
   logger.msg(Arc::DEBUG, "job(s) submit");
   std::vector<std::string> jobid_request;
   std::vector<CompileInfo>::iterator it; 
   for ( it=Site_List.begin() ; it < Site_List.end(); it++ ){
	
	jobid_request.push_back(job_submit((*it).job_name, (*it).site_url, (*it).script_url, (*it).sources,
				             (*it).architecture,  (*it).makefile_name, (*it).cpu_number,
				             (*it).parameters, (*it).tar, 
				             (*it).compiler, (*it).march  ));
	if( jobid_request.back() == "" ){
	       std::string message = "Wrong Job submitting!      URL: " + (*it).site_url;
	       message =  message + "     Achitecture: " + (*it).architecture;
	       outpayload_node.NewChild("compiler:error")= message;
	       
	       logger.msg(Arc::ERROR, "Wrong Job submitting!      URL: " + (*it).site_url);
	       logger.msg(Arc::ERROR, "     Achitecture: " + (*it).architecture);
        }
   }
    
   //current time (GMT)
   std::time_t rawtime;
   std::time ( &rawtime );	//current time
   tm * ptm;
   ptm = gmtime ( &rawtime );

   std::stringstream out;
   int year = ptm->tm_year;
   year = year +1900;
   
   int mon = ptm->tm_mon;
   mon++;
   out << "GMT  "<< year << "."  << mon << "." << ptm->tm_mday;
   out << "    " << ptm->tm_hour<< ":" << ptm->tm_min <<  ":" << ptm->tm_sec;
   
   std::string hear = "All Job(s) submited:    "+ out.str();
   outpayload_node.NewChild("compiler:response")=hear;
   outmsg.Payload(outpayload);
   logger.msg(Arc::DEBUG, hear);
    
    
   //output file(s) copy to the added directory
   logger.msg(Arc::DEBUG, "Result(s) download");
   logger.msg(Arc::DEBUG, "Download Place: " + compiler_op["make"]["download_place"]);
   try{
        std::string urlstr;
        std::vector<CompileInfo>::iterator it1;
        it1=Site_List.end();
        std::string Current_Architecture("");
	
        //sleep(10);
        while( !jobid_request.empty() ){
 	      logger.msg(Arc::DEBUG, "Download cycle: start");              
              if(it1 != Site_List.begin())
	             it1--;
              Current_Architecture = (*it1).architecture;
              logger.msg(Arc::DEBUG, "Current Arhitecture: "+Current_Architecture);
	      //Job ID checking
	      if( jobid_request.back() == "" ){
	            jobid_request.pop_back();
	            logger.msg(Arc::DEBUG, "Empty Job ID. Go to the next Job ID.");
                    continue;
	      }
	      //Wait until the job is Finished
	      while ( !Job_Status_Finished( jobid_request.back() ) ){
	            std::string message="Waiting 15 seconds";
	            outpayload_node.NewChild("compiler:response")=message;
	            outmsg.Payload(outpayload);
	            logger.msg(Arc::DEBUG, message);
	            sleep(15);
	     }
	     //sleep(10);
             Arc::XMLNode jobxml(jobid_request.back());
             if(!jobxml)
                    throw std::invalid_argument(std::string("Could not process Job ID from ")+jobid_request.back());
             urlstr=(std::string)(jobxml["ReferenceParameters"]["JobSessionDir"]); // TODO: clever service address extraction
	     std::string local_dir(compiler_op["make"]["download_place"]);   // for example: /home/user/tmp/s/
	     if ( Current_Architecture != "")
	        local_dir=  local_dir + Current_Architecture + "/";  
 	     jobid_request.pop_back();
	   
	     logger.msg(Arc::DEBUG, "Download url:  " + urlstr);
	     logger.msg(Arc::DEBUG, "Download path:  " + local_dir);
	   
	     if(urlstr.empty())
		throw std::invalid_argument("Missing service URL");
	     Arc::URL download_url(urlstr);
	     if(!download_url)
		throw std::invalid_argument(std::string("Can't parse service URL ")+urlstr);
	     Arc::MCCConfig download_cfg;
                 //    if(!tool.proxy_path.empty()) download_cfg.AddProxy(tool.proxy_path);
                 //    if(!tool.key_path.empty()) download_cfg.AddPrivateKey(tool.key_path);
                 //    if(!tool.cert_path.empty()) download_cfg.AddCertificate(tool.cert_path);
                 //    if(!tool.ca_dir.empty()) download_cfg.AddCADir(tool.ca_dir);
                 //    download_cfg.GetOverlay(tool.config_path);
	     Arc::AREXClient download_ac(download_url,download_cfg);
	     bool r = get_file(*(download_ac.SOAP()),download_url, local_dir);
	     if(!r) throw std::invalid_argument("Failed to download files!");
	     logger.msg(Arc::DEBUG, "Download cycle: end");
        }
   }
   catch (std::exception& err){
         std::cerr << "ERROR: " << err.what() << std::endl;
         return  Arc::MCC_Status(Arc::GENERIC_ERROR,"compiler","");
   }  
   //download end

   std::time ( &rawtime );	//current time
   ptm = gmtime ( &rawtime );

   std::stringstream out_finish;
   year = ptm->tm_year +1900;
   
   mon = ptm->tm_mon;
   mon++;
   out_finish << "GMT  "<< year << "."  << mon << "." << ptm->tm_mday;
   out_finish << "    " << ptm->tm_hour<< ":" << ptm->tm_min <<  ":" << ptm->tm_sec;

   logger.msg(Arc::DEBUG, "Finished the compile:  " + out_finish.str() );
    
   std::string say = compiler_op["make"]["name"];
   hear = "Finished the compile: " + out_finish.str();
  
   //the response
   outpayload_node.NewChild("compiler:response")=hear;
   outmsg.Payload(outpayload);
   logger.msg(Arc::DEBUG, "               The SOAP message send and return");

   Arc::MCC_Status return_Status(Arc::STATUS_OK, "compiler" , "this is the explanation" );
   return return_Status;
}

