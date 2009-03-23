#include "SRM1Client.h"

//namespace Arc {

  //Arc::Logger SRM1Client::logger(SRMClient::logger, "SRM1Client");
  
  SRM1Client::SRM1Client(SRMURL url) {
    version = "v1";
    implementation = SRM_IMPLEMENTATION_UNKNOWN;
    service_endpoint = url.ContactURL();
    csoap = new Arc::HTTPSClientSOAP(service_endpoint.c_str(),&soapobj,true,request_timeout,false);
    if(!csoap) { csoap=NULL; return; };
    if(!*csoap) { delete csoap; csoap=NULL; return; };
    soapobj.namespaces=srm1_soap_namespaces;
  }
  
  SRM1Client::~SRM1Client(void) {
    if(csoap) { csoap->disconnect(); delete csoap; };
  }
  
  static const char* Supported_Protocols[] = {
    "gsiftp","https","httpg","http","ftp","se"
  };
  
  bool SRM1Client::getTURLs(SRMClientRequest& req,
                            std::list<std::string>& urls) {
    if(!csoap) return false;
    if(!connect()) return false;
  
    SRMURL srmurl(req.surls().front().c_str());
    int soap_err = SOAP_OK;
    std::list<int> file_ids;
    ArrayOfstring* SURLs = soap_new_ArrayOfstring(&soapobj,-1);
    ArrayOfstring* Protocols = soap_new_ArrayOfstring(&soapobj,-1);
    struct SRMv1Meth__getResponse r; r._Result=NULL;
    if((!SURLs) || (!Protocols)) {
      csoap->reset(); return false;
    };
    Protocols->__ptr=(char**)Supported_Protocols;
    Protocols->__size=sizeof(Supported_Protocols)/sizeof(Supported_Protocols[0]);
    std::string file_url = srmurl.FullURL();
    const char* surl[] = { file_url.c_str() };
    SURLs->__ptr=(char**)surl;
    SURLs->__size=1;
    if((soap_err=soap_call_SRMv1Meth__get(&soapobj,csoap->SOAP_URL(),"get",SURLs,Protocols,r)) != SOAP_OK) {
      logger.msg(Arc::INFO, "SOAP request failed (get)");
      if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
      csoap->disconnect();
      return false;
    };
    if(r._Result == NULL) {  
      logger.msg(Arc::INFO, "SRM did not return any information");
      return false;
    };
    char* request_state = r._Result->state;
    req.request_id(r._Result->requestId);
    SRMv1Type__RequestStatus& result = *(r._Result);
    time_t t_start = time(NULL);
    for(;;) {
      ArrayOfRequestFileStatus* fstatus = result.fileStatuses;
      if(fstatus && (fstatus->__size) && (fstatus->__ptr)) {
        for(int n=0;n<fstatus->__size;n++) {
          SRMv1Type__RequestFileStatus* fs = fstatus->__ptr[n];
          if(fs && fs->state && (strcasecmp(fs->state,"ready") == 0)) {
            if(fs->TURL) {
              urls.push_back(std::string(fs->TURL));
              file_ids.push_back(fs->fileId);
            };
          };
        };
      };
      if(urls.size()) break; // Have requested data
      if(!request_state) break; // No data and no state - fishy
      if(strcasecmp(request_state,"pending") != 0) break;
      if((time(NULL) - t_start) > request_timeout) break;
      if(result.retryDeltaTime < 1) result.retryDeltaTime=1;
      if(result.retryDeltaTime > 10) result.retryDeltaTime=10;
      sleep(result.retryDeltaTime);
      SRMv1Meth__getRequestStatusResponse r;
      if((soap_err=soap_call_SRMv1Meth__getRequestStatus(&soapobj,csoap->SOAP_URL(),
               "getRequestStatus",req.request_id(),r)) != SOAP_OK) {
        logger.msg(Arc::INFO, "SOAP request failed (getRequestStatus)");
        if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
        csoap->disconnect();
        return false;
      };
      if(r._Result == NULL) {  
        logger.msg(Arc::INFO, "SRM did not return any information");
        return false;
      };
      request_state = r._Result->state;
      result = *(r._Result);
    };
    req.file_ids(file_ids);
    if(urls.size() == 0) return false;
    return acquire(req,urls);
  }
  
  bool SRM1Client::putTURLs(SRMClientRequest& req,
                            std::list<std::string>& urls,
                            unsigned long long size) {
    if(!csoap) return false;
    if(!connect()) return false;
  
    SRMURL srmurl(req.surls().front().c_str());
    int soap_err = SOAP_OK;
    std::list<int> file_ids;
    // Request place for new file to put
    ArrayOfstring* src_file_names = soap_new_ArrayOfstring(&soapobj,-1);
    ArrayOfstring* dst_file_names = soap_new_ArrayOfstring(&soapobj,-1);
    ArrayOflong* sizes = soap_new_ArrayOflong(&soapobj,-1);
    ArrayOfboolean* wantPermanent = soap_new_ArrayOfboolean(&soapobj,-1);
    ArrayOfstring* protocols = soap_new_ArrayOfstring(&soapobj,-1);
    struct SRMv1Meth__putResponse r; r._Result=NULL;
    //r._Result=soap_new_SRMv1Type__RequestStatus(&soapobj,-1);
    if((!src_file_names) || (!dst_file_names) || (!sizes) || 
       (!wantPermanent) || (!protocols)) {
      csoap->reset(); return false;
    };
    protocols->__ptr=(char**)Supported_Protocols;
    protocols->__size=sizeof(Supported_Protocols)/sizeof(Supported_Protocols[0]);
    LONG64 sizes_[] = { size }; // TODO
    bool wantPermanent_[] = { true };
    std::string file_url = srmurl.FullURL();
    const char* surl[] = { file_url.c_str() };
    src_file_names->__ptr=(char**)surl; src_file_names->__size=1;
    dst_file_names->__ptr=(char**)surl; dst_file_names->__size=1;
    sizes->__ptr=sizes_; sizes->__size=1;
    wantPermanent->__ptr=wantPermanent_; wantPermanent->__size=1;
    if((soap_err=soap_call_SRMv1Meth__put(&soapobj,csoap->SOAP_URL(),"put",
                 src_file_names,dst_file_names,sizes,
                 wantPermanent,protocols,r)) != SOAP_OK) {
      logger.msg(Arc::INFO, "SOAP request failed (put)");
      if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
      csoap->disconnect();
      return false;
    };
    if(r._Result == NULL) {
      logger.msg(Arc::INFO, "SRM did not return any information");
      return false;
    };
    char* request_state = r._Result->state;
    req.request_id(r._Result->requestId);
    SRMv1Type__RequestStatus* result = r._Result;
    time_t t_start = time(NULL);
    // Ask for request state in loop till SRM server returns  
    // request status !Pending and file status ready
    for(;;) {
      ArrayOfRequestFileStatus* fstatus = result->fileStatuses;
      if(fstatus && (fstatus->__size) && (fstatus->__ptr)) {
        for(int n=0;n<fstatus->__size;n++) {
          SRMv1Type__RequestFileStatus* fs = fstatus->__ptr[n];
          if(fs && fs->state && (strcasecmp(fs->state,"ready") == 0)) {
            if(fs->TURL) {
              urls.push_back(std::string(fs->TURL));
              file_ids.push_back(fs->fileId);
            };
          };
        };
      };
      if(urls.size()) break; // Have requested data
      if(!request_state) break; // No data and no state - fishy
      // Leave if state is not pending and no endpoints 
      if(strcasecmp(request_state,"pending") != 0) break;
      if((time(NULL) - t_start) > request_timeout) break;
      if(result->retryDeltaTime < 1) result->retryDeltaTime=1;
      if(result->retryDeltaTime > 10) result->retryDeltaTime=10;
      sleep(result->retryDeltaTime);
      SRMv1Meth__getRequestStatusResponse r;
      if((soap_err=soap_call_SRMv1Meth__getRequestStatus(&soapobj,csoap->SOAP_URL(),
               "getRequestStatus",req.request_id(),r)) != SOAP_OK) {
        logger.msg(Arc::INFO, "SOAP request failed (getRequestStatus)");
        if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
        csoap->disconnect();
        return false;
      };
      if(r._Result == NULL) {
        logger.msg(Arc::INFO, "SRM did not return any information");
        return false;
      };
      request_state = r._Result->state;
      result = r._Result;
    };
    req.file_ids(file_ids);
    if(urls.size() == 0) return false;
    return acquire(req,urls);
  }
  
  bool SRM1Client::copy(SRMClientRequest& req,
                        const std::string& source) {
    if(!csoap) return false;
    if(!connect()) return false;
  
    SRMURL srmurl(req.surls().front().c_str());
    int soap_err = SOAP_OK;
    std::list<int> file_ids;
    // Request place for new file to put
    ArrayOfstring* src_file_names = soap_new_ArrayOfstring(&soapobj,-1);
    ArrayOfstring* dst_file_names = soap_new_ArrayOfstring(&soapobj,-1);
    ArrayOfboolean* bools = soap_new_ArrayOfboolean(&soapobj,-1);
    struct SRMv1Meth__copyResponse r; r._Result=NULL;
    if((!src_file_names) || (!dst_file_names)) {
      csoap->reset(); return false;
    };
    std::string file_url = srmurl.FullURL();
    const char* surl[] = { file_url.c_str() };
    const char* srcurl[] = { source.c_str() };
    bool bools_[] = { false };
    src_file_names->__ptr=(char**)srcurl; src_file_names->__size=1;
    dst_file_names->__ptr=(char**)surl; dst_file_names->__size=1;
    bools->__ptr=bools_; bools->__size=1;
    if((soap_err=soap_call_SRMv1Meth__copy(&soapobj,csoap->SOAP_URL(),"copy",
                 src_file_names,dst_file_names,bools,r)) != SOAP_OK) {
      logger.msg(Arc::INFO, "SOAP request failed (copy)");
      if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
      csoap->disconnect();
      return false;
    };
    if(r._Result == NULL) {
      logger.msg(Arc::INFO, "SRM did not return any information");
      return false;
    };
    char* request_state = r._Result->state;
    req.request_id(r._Result->requestId);
    SRMv1Type__RequestStatus* result = r._Result;
    time_t t_start = time(NULL);
    // Ask for request state in loop till SRM server returns
    // request status !Pending and file status Ready
    for(;;) {
      ArrayOfRequestFileStatus* fstatus = result->fileStatuses;
      if(fstatus && (fstatus->__size) && (fstatus->__ptr)) {
        for(int n=0;n<fstatus->__size;n++) {
          SRMv1Type__RequestFileStatus* fs = fstatus->__ptr[n];
          if(fs && fs->state && (strcasecmp(fs->state,"ready") == 0)) {
            file_ids.push_back(fs->fileId);
          };
        };
      };
      if(file_ids.size()) break; // Have requested data
      if(!request_state) break; // No data and no state - fishy
      if((strcasecmp(request_state,"pending") != 0) && 
         (strcasecmp(request_state,"active") != 0)) break;
      if((time(NULL) - t_start) > request_timeout) break;
      if(result->retryDeltaTime < 5) result->retryDeltaTime=5;
      if(result->retryDeltaTime > 30) result->retryDeltaTime=30;
      sleep(result->retryDeltaTime);
      SRMv1Meth__getRequestStatusResponse r;
      if((soap_err=soap_call_SRMv1Meth__getRequestStatus(&soapobj,csoap->SOAP_URL(),
               "getRequestStatus",req.request_id(),r)) != SOAP_OK) {
        logger.msg(Arc::INFO, "SOAP request failed (getRequestStatus)");
        if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
        csoap->disconnect();
        return false;
      };
      if(r._Result == NULL) {
        logger.msg(Arc::INFO, "SRM did not return any information");
        return false;
      };
      request_state = r._Result->state;
      result = r._Result;
    };
    if(file_ids.size() == 0) return false;
    req.file_ids(file_ids);
    return release(req);
  }
  
  bool SRM1Client::acquire(SRMClientRequest& req,std::list<std::string>& urls) {
    int soap_err = SOAP_OK;
    std::list<int> file_ids = req.file_ids();
    // Tell server to move files into "Running" state
    std::list<int>::iterator file_id = file_ids.begin();
    std::list<std::string>::iterator f_url = urls.begin();
    for(;file_id!=file_ids.end();) {
      SRMv1Meth__setFileStatusResponse r; r._Result=NULL;
      if((soap_err=soap_call_SRMv1Meth__setFileStatus(&soapobj,csoap->SOAP_URL(),
              "setFileStatus",req.request_id(),*file_id,"Running",r)) != SOAP_OK) {
        logger.msg(Arc::INFO, "SOAP request failed (setFileStatus)");
        if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
        file_id=file_ids.erase(file_id); f_url=urls.erase(f_url);
        continue;
      };
      SRMv1Type__RequestStatus* result = r._Result;
      ArrayOfRequestFileStatus* fstatus = result->fileStatuses;
      if(fstatus && (fstatus->__size) && (fstatus->__ptr)) {
        int n;
        for(n=0;n<fstatus->__size;n++) {
          SRMv1Type__RequestFileStatus* fs = fstatus->__ptr[n];
          if(!fs) continue;
          if(fs->fileId != *file_id) continue;
          if(fs->state && (strcasecmp(fs->state,"running") == 0)) {
            ++file_id; ++f_url; break;
          };
        };
        if(n<fstatus->__size) continue;
      };
      logger.msg(Arc::DEBUG, "File could not be moved to Running state: %s", *f_url);
      file_id=file_ids.erase(file_id); f_url=urls.erase(f_url);
    };
    req.file_ids(file_ids);
    if(urls.size() == 0) return false;
    // Do not disconnect
    return true;
  }
  
  bool SRM1Client::remove(SRMClientRequest& req) {
    if(!csoap) return false;
    if(!connect()) return false;
  
    SRMURL srmurl(req.surls().front().c_str());
    int soap_err = SOAP_OK;
    ArrayOfstring* SURLs = soap_new_ArrayOfstring(&soapobj,-1);
    if(!SURLs) {
      csoap->reset(); return false;
    };
    std::string file_url = srmurl.FullURL();
    const char* surl[] = { file_url.c_str() };
    SURLs->__ptr=(char**)surl;
    SURLs->__size=1;
    struct SRMv1Meth__advisoryDeleteResponse r;
    if((soap_err=soap_call_SRMv1Meth__advisoryDelete(&soapobj,csoap->SOAP_URL(),
                        "advisoryDelete",SURLs,r)) != SOAP_OK) {
      logger.msg(Arc::INFO, "SOAP request failed (SRMv1Meth__advisoryDelete)");
      if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
      csoap->disconnect();
      return false;
    };
    return true; 
  }
  
  bool SRM1Client::info(SRMClientRequest& req,
                        std::list<struct SRMFileMetaData>& metadata,
                        const int recursive) {
    if(!csoap) return false;
    if(!connect()) return false;
  
    SRMURL srmurl(req.surls().front().c_str());
    int soap_err = SOAP_OK;
    ArrayOfstring* SURLs = soap_new_ArrayOfstring(&soapobj,-1);
    if(!SURLs) {
      csoap->reset(); return false;
    };
    std::string file_url = srmurl.FullURL();
    const char* surl[] = { file_url.c_str() };
    SURLs->__ptr=(char**)surl;
    SURLs->__size=1;
    struct SRMv1Meth__getFileMetaDataResponse r; r._Result=NULL;
    if((soap_err=soap_call_SRMv1Meth__getFileMetaData(&soapobj,csoap->SOAP_URL(),
                        "getFileMetaData",SURLs,r)) != SOAP_OK) {
      logger.msg(Arc::INFO, "SOAP request failed (getFileMetaData)");
      if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
      csoap->disconnect();
      return false;
    };
    if(r._Result == NULL) {
      logger.msg(Arc::INFO, "SRM did not return any information");
      return false;
    };
    if((r._Result->__size == 0) || 
       (r._Result->__ptr == NULL) ||
       (r._Result->__ptr[0] == NULL)) {
      logger.msg(Arc::INFO, "SRM did not return any useful information");
      return false;
    };
    SRMv1Type__FileMetaData& mdata = *(r._Result->__ptr[0]);
    struct SRMFileMetaData md;
    md.path=srmurl.FileName();
    // tidy up path
    std::string::size_type i = md.path.find("//", 0);
    while (i != std::string::npos) {
    	md.path.erase(i, 1);
    	i = md.path.find("//", 0);
    };
    if (md.path.find("/") != 0) md.path = "/" + md.path;
    // date, type and locality not supported in v1
    md.createdAtTime=0;
    md.fileType = SRM_FILE_TYPE_UNKNOWN;
    md.fileLocality = SRM_UNKNOWN;
    md.size=mdata.size;
    md.checkSumType="";
    md.checkSumValue="";
    if(mdata.checksumType) { md.checkSumType=mdata.checksumType; };
    if(mdata.checksumValue) { md.checkSumValue=mdata.checksumValue; };
    metadata.push_back(md);
    return true; 
  }
  
  bool SRM1Client::release(SRMClientRequest& req) {
    if(!csoap) return false;
    if(!connect()) return false;
    int soap_err = SOAP_OK;
    std::list<int> file_ids = req.file_ids();
    // Tell server to move files into "Done" state
    std::list<int>::iterator file_id = file_ids.begin();
    for(;file_id!=file_ids.end();) {
      SRMv1Meth__setFileStatusResponse r; r._Result=NULL;
      if((soap_err=soap_call_SRMv1Meth__setFileStatus(&soapobj,csoap->SOAP_URL(),
              "setFileStatus",req.request_id(),*file_id,"Done",r)) != SOAP_OK) {
        logger.msg(Arc::INFO, "SOAP request failed (setFileStatus)");
        if(logger.getThreshold() > Arc::FATAL) soap_print_fault(&soapobj, stderr);
        ++file_id; continue;
      };
      SRMv1Type__RequestStatus* result = r._Result;
      ArrayOfRequestFileStatus* fstatus = result->fileStatuses;
      if(fstatus && (fstatus->__size) && (fstatus->__ptr)) {
        int n;
        for(n=0;n<fstatus->__size;n++) {
          SRMv1Type__RequestFileStatus* fs = fstatus->__ptr[n];
          if(fs->fileId != *file_id) continue;
          if(fs && fs->state && (strcasecmp(fs->state,"Done") == 0)) {
            file_id=file_ids.erase(file_id); break;
          };
        };
        if(n<fstatus->__size) continue;
      };
      logger.msg(Arc::DEBUG, "File could not be moved to Done state");
      ++file_id;
    };
    req.file_ids(file_ids);
    return true; 
  }
   
  bool SRM1Client::releaseGet(SRMClientRequest& req) {
    return release(req);
  }
  
  bool SRM1Client::releasePut(SRMClientRequest& req) {
    return release(req);
  }
  
  bool SRM1Client::abort(SRMClientRequest& req) {
    return release(req);
  }
  
//} // namespace Arc
