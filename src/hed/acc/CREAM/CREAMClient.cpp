#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/miscutils.h>

#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>
#include <arc/message/MCC.h>
#include <arc/Logger.h>
#include <arc/URL.h>

#include "CREAMClient.h"
#include "OpenSSLFunctions.h"

namespace Arc{

        Logger CREAMClient::logger(Logger::rootLogger, "CREAM-Client");
        
        static void set_cream_namespaces(NS& ns) {
            ns["SOAP-ENV"]="http://schemas.xmlsoap.org/soap/envelope/";
            ns["SOAP-ENC"]="http://schemas.xmlsoap.org/soap/encoding/";
            ns["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
            ns["xsd"]="http://www.w3.org/2001/XMLSchema";
            ns["ns1"]="http://www.gridsite.org/namespaces/delegation-2";
            ns["ns2"]="http://glite.org/2007/11/ce/cream/types";
            ns["ns3"]="http://glite.org/2007/11/ce/cream";
        }
                
        CREAMClient::CREAMClient(const URL& url, const MCCConfig& cfg)
          : client(NULL) {
            logger.msg(INFO, "Creating a CREAM client.");
            MCCConfig modified_cfg = cfg;
            UserConfig uc;
            const XMLNode cfgtree = uc.ConfTree();
            modified_cfg.AddProxy(cfgtree["ProxyPath"]);
            client = new ClientSOAP(modified_cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
            set_cream_namespaces(cream_ns);
        }
        
        CREAMClient::~CREAMClient() {
            if(client) delete client;
        }
        
        bool CREAMClient::stat(const std::string& jobid, std::string& status) {
            logger.msg(INFO, "Creating and sending a status request.");
            
            PayloadSOAP req(cream_ns);
            NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            XMLNode jobStatusRequest = req.NewChild("ns2:JobStatusRequest", ns2);
            XMLNode jobId = jobStatusRequest.NewChild("ns2:jobId", ns2);
            XMLNode id = jobId.NewChild("ns2:id", ns2);
            id.Set(jobid);
            if (this->delegationId != "") {
                XMLNode delegId = jobStatusRequest.NewChild("ns2:delegationProxyId", ns2);
                delegId.Set(this->delegationId);
            }
    
            // Send status request
            PayloadSOAP* resp = NULL;
            
            if(client) {
                MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobStatus",&req,&resp);
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                }
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }
            
            XMLNode name, failureReason, fault;
            (*resp)["JobStatusResponse"]["result"]["jobStatus"]["name"].New(name);
            if ((*resp)["JobStatusResponse"]["result"]["jobStatus"]["failureReason"]) (*resp)["JobStatusResponse"]["result"]["jobStatus"]["failureReason"].New(failureReason);
            status = (std::string)name;
            std::string faultstring = (std::string)failureReason;
            
            std::string result = (std::string)name;
            if ((*resp)["JobStatusResponse"]["result"]["JobUnknownFault"]) (*resp)["JobStatusResponse"]["result"]["JobUnknownFault"].New(fault);
            if ((*resp)["JobStatusResponse"]["result"]["JobStatusInvalidFault"]) (*resp)["JobStatusResponse"]["result"]["JobStatusInvalidFault"].New(fault);
            if ((*resp)["JobStatusResponse"]["result"]["DelegationIdMismatchFault"]) (*resp)["JobStatusResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
            if ((*resp)["JobStatusResponse"]["result"]["DateMismatchFault"]) (*resp)["JobStatusResponse"]["result"]["DateMismatchFault"].New(fault);
            if ((*resp)["JobStatusResponse"]["result"]["LeaseIdMismatchFault"]) (*resp)["JobStatusResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
            if ((*resp)["JobStatusResponse"]["result"]["GenericFault"]) (*resp)["JobStatusResponse"]["result"]["GenericFault"].New(fault);
            delete resp;
            if ((bool)fault) {
                logger.msg(ERROR, (std::string)(fault["Description"]));
                return false;
            }
            if (status=="") {
                logger.msg(ERROR, "The job status could not be retrieved.");
                return false;
            }
            else {
                return true;
            }
            
        }  // CREAMClient::stat()
        
        bool CREAMClient::cancel(const std::string& jobid) {
            logger.msg(INFO, "Creating and sending request to terminate a job.");
    
            PayloadSOAP req(cream_ns);
            NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            XMLNode jobCancelRequest = req.NewChild("ns2:JobCancelRequest", ns2);
            XMLNode jobId = jobCancelRequest.NewChild("ns2:jobId", ns2);
            XMLNode id = jobId.NewChild("ns2:id", ns2);
            id.Set(jobid);
            
            // Send cancel request
            PayloadSOAP* resp = NULL;
            if(client) {
                MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobCancel",&req,&resp);
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                }
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }

            XMLNode cancelled, fault;
            (*resp)["JobCancelResponse"]["result"]["jobId"]["id"].New(cancelled);
            std::string result = (std::string)cancelled;
            if ((*resp)["JobCancelResponse"]["result"]["JobUnknownFault"]) (*resp)["JobCancelResponse"]["result"]["JobUnknownFault"].New(fault);
            if ((*resp)["JobCancelResponse"]["result"]["JobStatusInvalidFault"]) (*resp)["JobCancelResponse"]["result"]["JobStatusInvalidFault"].New(fault);
            if ((*resp)["JobCancelResponse"]["result"]["DelegationIdMismatchFault"]) (*resp)["JobCancelResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
            if ((*resp)["JobCancelResponse"]["result"]["DateMismatchFault"]) (*resp)["JobCancelResponse"]["result"]["DateMismatchFault"].New(fault);
            if ((*resp)["JobCancelResponse"]["result"]["LeaseIdMismatchFault"]) (*resp)["JobCancelResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
            if ((*resp)["JobCancelResponse"]["result"]["GenericFault"]) (*resp)["JobCancelResponse"]["result"]["GenericFault"].New(fault);
            delete resp;
            if ((bool)fault) {
                logger.msg(ERROR, (std::string)(fault["Description"]));
                return false;
            }
            if (result=="") {
                logger.msg(ERROR, "Job termination failed.");
                return false;
            }
            return true;
        }  // CREAMClient::cancel()
        
        bool CREAMClient::purge(const std::string& jobid) {
            logger.msg(INFO, "Creating and sending request to clean a job.");
            
            PayloadSOAP req(cream_ns);
            NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            XMLNode jobStatusRequest = req.NewChild("ns2:JobPurgeRequest", ns2);
            XMLNode jobId = jobStatusRequest.NewChild("ns2:jobId", ns2);
            XMLNode id = jobId.NewChild("ns2:id", ns2);
            id.Set(jobid);
            XMLNode creamURL = jobId.NewChild("ns2:creamURL", ns2);
            
            // Send clean request
            PayloadSOAP* resp = NULL;
            if(client) {
                MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobPurge",&req,&resp);
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                }
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }
        
            XMLNode cancelled, fault;
            (*resp)["JobPurgeResponse"]["result"]["jobId"]["id"].New(cancelled);
            std::string result = (std::string)cancelled;
            if ((*resp)["JobPurgeResponse"]["result"]["JobUnknownFault"]) (*resp)["JobPurgeResponse"]["result"]["JobUnknownFault"].New(fault);
            if ((*resp)["JobPurgeResponse"]["result"]["JobStatusInvalidFault"]) (*resp)["JobPurgeResponse"]["result"]["JobStatusInvalidFault"].New(fault);
            if ((*resp)["JobPurgeResponse"]["result"]["DelegationIdMismatchFault"]) (*resp)["JobPurgeResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
            if ((*resp)["JobPurgeResponse"]["result"]["DateMismatchFault"]) (*resp)["JobPurgeResponse"]["result"]["DateMismatchFault"].New(fault);
            if ((*resp)["JobPurgeResponse"]["result"]["LeaseIdMismatchFault"]) (*resp)["JobPurgeResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
            if ((*resp)["JobPurgeResponse"]["result"]["GenericFault"]) (*resp)["JobPurgeResponse"]["result"]["GenericFault"].New(fault);
            delete resp;
            if ((bool)fault) {
                logger.msg(ERROR, (std::string)(fault["Description"]));
                return false;
            }
            if (result=="") {
                logger.msg(ERROR, "Job cleaning failed.");
                return false;
            }
            return true;
        }  // CREAMClient::purge()
       
        bool CREAMClient::registerJob(const std::string& jdl_text, creamJobInfo& info) {
            logger.msg(INFO, "Creating and sending job register request.");
            
            PayloadSOAP req(cream_ns);
            NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            XMLNode jobRegisterRequest = req.NewChild("ns2:JobRegisterRequest", ns2);
            XMLNode act_job = jobRegisterRequest.NewChild("ns2:JobDescriptionList", ns2);
            XMLNode jdl_node = act_job.NewChild("ns2:JDL", ns2);
            jdl_node.Set(jdl_text);
            if (this->delegationId != "") {
                XMLNode delegId = act_job.NewChild("ns2:delegationId", ns2);
                delegId.Set(this->delegationId);
            }
            XMLNode autostart_node = act_job.NewChild("ns2:autoStart", ns2);
            autostart_node.Set("false");
            PayloadSOAP* resp = NULL;
            
            logger.msg(VERBOSE, "Job description to be sent: %s",jdl_text);
            
            // Send job request
            if(client) {
                MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobRegister", &req,&resp);
                if(!status) {
                    logger.msg(ERROR, "Submission request failed.");
                    return false;
                }
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                };
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }
            XMLNode id, fault;
            (*resp)["JobRegisterResponse"]["result"]["jobId"]["id"].New(id);

            std::string result = (std::string)id;
            if ((*resp)["JobRegisterResponse"]["result"]["JobUnknownFault"]) (*resp)["JobRegisterResponse"]["result"]["JobUnknownFault"].New(fault);
            if ((*resp)["JobRegisterResponse"]["result"]["JobStatusInvalidFault"]) (*resp)["JobRegisterResponse"]["result"]["JobStatusInvalidFault"].New(fault);
            if ((*resp)["JobRegisterResponse"]["result"]["DelegationIdMismatchFault"]) (*resp)["JobRegisterResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
            if ((*resp)["JobRegisterResponse"]["result"]["DateMismatchFault"]) (*resp)["JobRegisterResponse"]["result"]["DateMismatchFault"].New(fault);
            if ((*resp)["JobRegisterResponse"]["result"]["LeaseIdMismatchFault"]) (*resp)["JobRegisterResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
            if ((*resp)["JobRegisterResponse"]["result"]["GenericFault"]) (*resp)["JobRegisterResponse"]["result"]["GenericFault"].New(fault);
            
            // Create the return value
            XMLNode property;
            if ((*resp)["JobRegisterResponse"]["result"]["jobId"]["creamURL"]) info.creamURL = (std::string)((*resp)["JobRegisterResponse"]["result"]["jobId"]["creamURL"]);
            property = (*resp)["JobRegisterResponse"]["result"]["jobId"]["property"];
            while (property != 0) {
                if ((std::string)(property["name"]) == "CREAMInputSandboxURI") {
                    info.ISB_URI = (std::string)(property["value"]);
                } else if ((std::string)(property["name"]) == "CREAMOutputSandboxURI") {
                    info.OSB_URI = (std::string)(property["value"]);
                }
                ++property;
            }
            
            delete resp;
            if ((bool)fault) {
                logger.msg(ERROR, (std::string)(fault["Description"]));
                return false;
            }
            if (result=="") {
                logger.msg(ERROR, "No job ID has been received");
                return false;
            }
            info.jobId = result;
            return true;
        } // CREAMClient::registerJob()
       
        bool CREAMClient::startJob(const std::string& jobid) {
            logger.msg(INFO, "Creating and sending job start request.");
            
            PayloadSOAP req(cream_ns);
            NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            XMLNode jobStartRequest = req.NewChild("ns2:JobStartRequest", ns2);
            XMLNode jobId = jobStartRequest.NewChild("ns2:jobId", ns2);
            XMLNode id_node = jobId.NewChild("ns2:id", ns2);
            id_node.Set(jobid);
            if (this->delegationId != "") {
                XMLNode delegId = jobStartRequest.NewChild("ns2:delegationId", ns2);
                delegId.Set(this->delegationId);
            }
            PayloadSOAP* resp = NULL;
            
            // Send job request
            if(client) {
                MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobStart", &req,&resp);
                if(!status) {
                    logger.msg(ERROR, "Submission request failed.");
                    return false;
                }
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                };
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }
            XMLNode id, fault;
            (*resp)["JobStartResponse"]["result"]["jobId"]["id"].New(id);

            std::string result = (std::string)id;
            if ((*resp)["JobStartResponse"]["result"]["JobUnknownFault"]) (*resp)["JobStartResponse"]["result"]["JobUnknownFault"].New(fault);
            if ((*resp)["JobStartResponse"]["result"]["JobStatusInvalidFault"]) (*resp)["JobStartResponse"]["result"]["JobStatusInvalidFault"].New(fault);
            if ((*resp)["JobStartResponse"]["result"]["DelegationIdMismatchFault"]) (*resp)["JobStartResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
            if ((*resp)["JobStartResponse"]["result"]["DateMismatchFault"]) (*resp)["JobStartResponse"]["result"]["DateMismatchFault"].New(fault);
            if ((*resp)["JobStartResponse"]["result"]["LeaseIdMismatchFault"]) (*resp)["JobStartResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
            if ((*resp)["JobStartResponse"]["result"]["GenericFault"]) (*resp)["JobStartResponse"]["result"]["GenericFault"].New(fault);
            delete resp;
            if ((bool)fault) {
                logger.msg(ERROR, (std::string)(fault["Description"]));
                return false;
            }
            if (result=="") {
                logger.msg(ERROR, "Job starting failed.");
                return false;
            }
            return true;
        } // CREAMClient::startJob()
       
        bool CREAMClient::createDelegation(const std::string& delegation_id) {
            logger.msg(INFO, "Creating delegation.");
            
            PayloadSOAP req(cream_ns);
            NS ns1;
            ns1["ns1"]="http://www.gridsite.org/namespaces/delegation-2";
            XMLNode getProxyReqRequest = req.NewChild("ns1:getProxyReq", ns1);
            XMLNode delegid = getProxyReqRequest.NewChild("delegationID", ns1);
            delegid.Set(delegation_id);
            PayloadSOAP* resp = NULL;
            
            // Send job request
            if(client) {
                MCC_Status status = client->process("", &req,&resp);
                if(!status) {
                    logger.msg(ERROR, "Submission request failed.");
                    return false;
                }
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                };
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }

            std::string getProxyReqReturnValue;

            if ((bool)(*resp) && (bool)((*resp)["getProxyReqResponse"]["getProxyReqReturn"]) && ((std::string)(*resp)["getProxyReqResponse"]["getProxyReqReturn"] != "")) {
                getProxyReqReturnValue = (std::string)(*resp)["getProxyReqResponse"]["getProxyReqReturn"];
            }
            else {
                logger.msg(ERROR, "Delegation creating failed.");
                return false;
            }
            delete resp;

            UserConfig uc;
            const XMLNode cfgtree = uc.ConfTree();
            std::string proxy = (std::string) cfgtree["ProxyPath"];
            std::string signedcert;
            char *cert=NULL; 
            int timeleft = getCertTimeLeft(proxy);
            
            if (makeProxyCert(&cert,(char*) getProxyReqReturnValue.c_str(),(char*) proxy.c_str(),(char *) proxy.c_str(),timeleft)) {
                logger.msg(ERROR, "DelegateProxy failed.");
                return false;
            }
            signedcert.assign(cert);
  
            PayloadSOAP req2(cream_ns);
            XMLNode putProxyRequest = req2.NewChild("ns1:putProxy", ns1);
            XMLNode delegid_node = putProxyRequest.NewChild("delegationID", ns1);
            delegid_node.Set(delegation_id);
            XMLNode proxy_node = putProxyRequest.NewChild("proxy", ns1);
            proxy_node.Set(signedcert);
            resp = NULL;
            
            // Send job request
            if(client) {
                MCC_Status status = client->process("", &req2,&resp);
                if(!status) {
                    logger.msg(ERROR, "Submission request failed.");
                    return false;
                }
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                };
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }

            if (!(bool)(*resp) || !(bool)((*resp)["putProxyResponse"])) {
                logger.msg(ERROR, "Delegation creating failed.");
                return false;
            }
            delete resp;
            return true;
            
        } // CREAMClient::createDelegation()
        
        bool CREAMClient::destroyDelegation(const std::string& delegation_id) {
            logger.msg(INFO, "Creating delegation.");
            
            PayloadSOAP req(cream_ns);
            NS ns1;
            ns1["ns1"]="http://www.gridsite.org/namespaces/delegation-2";
            XMLNode getProxyReqRequest = req.NewChild("ns1:destroy", ns1);
            XMLNode delegid = getProxyReqRequest.NewChild("delegationID", ns1);
            delegid.Set(delegation_id);
            PayloadSOAP* resp = NULL;
            
            // Send job request
            if(client) {
                MCC_Status status = client->process("", &req,&resp);
                if(!status) {
                    logger.msg(ERROR, "Submission request failed.");
                    return false;
                }
                if(resp == NULL) {
                    logger.msg(ERROR,"There was no SOAP response.");
                    return false;
                };
            } else {
                logger.msg(ERROR, "There is no connection chain configured.");
                return false;
            }

            std::string getProxyReqReturnValue;

            if (!(bool)(*resp) || !(bool)((*resp)["destroyResponse"])) {
                logger.msg(ERROR, "Delegation destroying failed.");
                return false;               
            }
            delete resp;
            return true;
        } // CREAMClient::destroyDelegation()

} // namespace Arc
