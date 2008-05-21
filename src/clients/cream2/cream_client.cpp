// cream_client.cpp

#include "cream_client.h"
#include "openSSLFunctions.cpp"

namespace Arc{
    namespace Cream{
        CREAMClientError::CREAMClientError(const std::string& what) : std::runtime_error(what){  }
        Arc::Logger CREAMClient::logger(Arc::Logger::rootLogger, "CREAMClient");
        
        static void set_cream_namespaces(Arc::NS& ns) {
            ns["SOAP-ENV"]="http://schemas.xmlsoap.org/soap/envelope/";
            ns["SOAP-ENC"]="http://schemas.xmlsoap.org/soap/encoding/";
            ns["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
            ns["xsd"]="http://www.w3.org/2001/XMLSchema";
            ns["ns1"]="http://www.gridsite.org/namespaces/delegation-2";
            ns["ns2"]="http://glite.org/2007/11/ce/cream/types";
            ns["ns3"]="http://glite.org/2007/11/ce/cream";
        }
                
        CREAMClient::CREAMClient(const Arc::URL& url, const Arc::MCCConfig& cfg) throw(CREAMClientError):client(NULL) {
            logger.msg(Arc::INFO, "Creating a CREAM client.");
            Arc::MCCConfig modified_cfg = cfg;
            client = new Arc::ClientSOAP(modified_cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
            set_cream_namespaces(cream_ns);
        }
        
        CREAMClient::~CREAMClient() {
            if(client) delete client;
        }
        
        std::string CREAMClient::stat(const std::string& jobid) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending a status request.");
            
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobStatusRequest = req.NewChild("ns2:JobStatusRequest", ns2);
            Arc::XMLNode jobId = jobStatusRequest.NewChild("ns2:jobId", ns2);
            Arc::XMLNode id = jobId.NewChild("ns2:id", ns2);
            id.Set(jobid);
            if (this->delegationId != "") {
                Arc::XMLNode delegId = jobStatusRequest.NewChild("ns2:delegationProxyId", ns2);
                delegId.Set(this->delegationId);
            }
    
            // Send status request
            Arc::PayloadSOAP* resp = NULL;
            
            // Testing: write the outgoing SOAP message
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;

            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobStatus",&req,&resp);
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                }
            } else throw CREAMClientError("There is no connection chain configured.");
            
            // Testing: write the incoming SOAP message
            std::cout << "jobStatusResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;
            
            Arc::XMLNode name, failureReason;
            (*resp)["JobStatusResponse"]["result"]["jobStatus"]["name"].New(name);
            if ((*resp)["JobStatusResponse"]["result"]["jobStatus"]["failureReason"]) (*resp)["JobStatusResponse"]["result"]["jobStatus"]["failureReason"].New(failureReason);
            std::string statusarc;
            std::string status = (std::string)name;
            std::string faultstring = (std::string)failureReason;
            delete resp;
            
            //translate to ARC terminology
            if (!status.compare("REGISTERED")) statusarc.assign("ACCEPTING");
            if (!status.compare("PENDING")) statusarc.assign("SUBMITTING");
            if (!status.compare("IDLE")) statusarc.assign("INLRMS:Q");
            if (!status.compare("RUNNING")) statusarc.assign("INLRMS:R");
            if (!status.compare("REALLY-RUNNING")) statusarc.assign("INLRMS:R");
            if (!status.compare("HELD")) statusarc.assign("INLRMS:S");
            if (!status.compare("CANCELLED")) statusarc.assign("KILLED");
            if (!status.compare("DONE-OK")) statusarc.assign("FINISHED");
            if (!status.compare("DONE-FAILED")) statusarc.assign("FAILED"); // Job failure
            if (!status.compare("ABORTED")) statusarc.assign("FAILES"); //failure at LRMS level
            if (!status.compare("UNKNOWN")) statusarc.assign("EXECUTED"); //Chosen because this seems to be the default ARC behaviour
            
            if (faultstring!="") throw CREAMClientError(faultstring);
            else if (status=="") throw CREAMClientError("The job status could not be retrieved.");
            else return statusarc;
            
        }  // CREAMClient::stat()
        
        void CREAMClient::cancel(const std::string& jobid) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending request to terminate a job.");
    
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobCancelRequest = req.NewChild("ns2:JobCancelRequest", ns2);
            Arc::XMLNode jobId = jobCancelRequest.NewChild("ns2:jobId", ns2);
            Arc::XMLNode id = jobId.NewChild("ns2:id", ns2);
            id.Set(jobid);
            if (this->delegationId != "") {
                Arc::XMLNode delegId = jobCancelRequest.NewChild("ns2:delegationProxyId", ns2);
                delegId.Set(this->delegationId);
            }
            
            // Testing: write the outgoing SOAP message
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;
            
            // Send cancel request
            Arc::PayloadSOAP* resp = NULL;
            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobCancel",&req,&resp);
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                }
            } else throw CREAMClientError("There is no connection chain configured.");

            // Testing: write the incoming SOAP message
            std::cout << "jobCancelResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;
            
            Arc::XMLNode cancelled;
            (*resp)["JobCancelResponse"]["Response"]["JobId"]["id"].New(cancelled);
            std::string result = (std::string)cancelled;
            std::string faultstring = "";
            if ((*resp)["JobCancelResponse"]["Response"]["JobUnknownFault"]) faultstring += (*resp)["JobCancelResponse"]["Response"]["JobUnknownFault"]["FaultCause"] + "\n";
            if ((*resp)["JobCancelResponse"]["Response"]["JobStatusInvalidFault"]) faultstring += (*resp)["JobCancelResponse"]["Response"]["JobStatusInvalidFault"]["FaultCause"] + "\n";
            if ((*resp)["JobCancelResponse"]["Response"]["DelegationIdMismatchFault"]) faultstring += (*resp)["JobCancelResponse"]["Response"]["DelegationIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobCancelResponse"]["Response"]["DateMismatchFault"]) faultstring += (*resp)["JobCancelResponse"]["Response"]["DateMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobCancelResponse"]["Response"]["LeaseIdMismatchFault"]) faultstring += (*resp)["JobCancelResponse"]["Response"]["LeaseIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobCancelResponse"]["Response"]["GenericFault"]) faultstring += (*resp)["JobCancelResponse"]["Response"]["GenericFault"]["FaultCause"] + "\n";
            delete resp;
            if (faultstring!="") throw CREAMClientError(faultstring);
            if (result=="") throw CREAMClientError("Job termination failed.");
        }  // CREAMClient::cancel()
        
        void CREAMClient::purge(const std::string& jobid) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending request to clean a job.");
            
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobStatusRequest = req.NewChild("ns2:JobPurgeRequest", ns2);
            Arc::XMLNode jobId = jobStatusRequest.NewChild("ns2:jobId", ns2);
            Arc::XMLNode id = jobId.NewChild("ns2:id", ns2);
            id.Set(jobid);
            Arc::XMLNode creamURL = jobId.NewChild("ns2:creamURL", ns2);
            
            // Testing: write the outgoing SOAP message
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;
            
            // Send clean request
            Arc::PayloadSOAP* resp = NULL;
            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobPurge",&req,&resp);
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                }
            } else throw CREAMClientError("There is no connection chain configured.");
        
            // Testing: write the incoming SOAP message
            std::cout << "jobPurgeResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;
            
            Arc::XMLNode cancelled;
            (*resp)["JobPurgeResponse"]["Response"]["JobId"]["id"].New(cancelled);
            std::string result = (std::string)cancelled;
            std::string faultstring = "";
            if ((*resp)["JobPurgeResponse"]["Response"]["JobUnknownFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["JobUnknownFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["JobStatusInvalidFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["JobStatusInvalidFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["DelegationIdMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["DelegationIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["DateMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["DateMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["LeaseIdMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["LeaseIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["GenericFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["GenericFault"]["FaultCause"] + "\n";
            delete resp;
            if (faultstring!="") throw CREAMClientError(faultstring);
            if (result=="") throw CREAMClientError("Job cleaning failed.");
        }  // CREAMClient::purge()
       
        std::string CREAMClient::registerJob(std::string& jsdl_text) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending job register request.");

            // JSDL -> JDL conversion should be here
            std::string jdl_text = jsdl_text;
            
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobRegisterRequest = req.NewChild("ns2:JobRegisterRequest", ns2);
            Arc::XMLNode act_job = jobRegisterRequest.NewChild("ns2:JobDescriptionList", ns2);
            Arc::XMLNode jdl_node = act_job.NewChild("ns2:JDL", ns2);
            jdl_node.Set(jdl_text);
            if (this->delegationId != "") {
                Arc::XMLNode delegId = jobRegisterRequest.NewChild("ns2:delegationProxyId", ns2);
                delegId.Set(this->delegationId);
            }
            Arc::XMLNode autostart_node = act_job.NewChild("ns2:autostart", ns2);
            autostart_node.Set("false");
            Arc::PayloadSOAP* resp = NULL;
            
            logger.msg(Arc::VERBOSE, "Job description to be sent: %s",jdl_text);
            
            // Testing: write the outgoing SOAP message
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;
            
            // Send job request
            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobRegister", &req,&resp);
                if(!status) {
                    logger.msg(Arc::ERROR, "Submission request failed.");
                    throw CREAMClientError("Submission request failed.");
                }
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                };
            } else throw CREAMClientError("There is no connection chain configured.");
            Arc::XMLNode id, fs;
            (*resp)["JobRegisterResponse"]["result"]["JobId"]["id"].New(id);
            (*resp)["Fault"]["faultstring"].New(fs);

            // Testing: write the incoming SOAP message
            std::cout << "jobRegisterResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;

            std::string faultstring = "";
            faultstring=(std::string)fs;
            if ((*resp)["JobPurgeResponse"]["Response"]["JobUnknownFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["JobUnknownFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["JobStatusInvalidFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["JobStatusInvalidFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["DelegationIdMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["DelegationIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["DateMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["DateMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["LeaseIdMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["LeaseIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["GenericFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["GenericFault"]["FaultCause"] + "\n";
            delete resp;
            if (faultstring=="" && (bool)id) return (std::string)id;
            else throw CREAMClientError(faultstring);
        } // CREAMClient::registerJob()
       
        void CREAMClient::startJob(const std::string& jobid) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending job start request.");
            
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobStartRequest = req.NewChild("ns2:JobStartRequest", ns2);
            Arc::XMLNode jobId = jobStartRequest.NewChild("ns2:jobId", ns2);
            Arc::XMLNode id_node = jobId.NewChild("ns2:id", ns2);
            id_node.Set(jobid);
            if (this->delegationId != "") {
                Arc::XMLNode delegId = jobStartRequest.NewChild("ns2:delegationProxyId", ns2);
                delegId.Set(this->delegationId);
            }
            Arc::PayloadSOAP* resp = NULL;
            
            // Testing: write the outgoing SOAP message
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;
            
            // Send job request
            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobStart", &req,&resp);
                if(!status) {
                    logger.msg(Arc::ERROR, "Submission request failed.");
                    throw CREAMClientError("Submission request failed.");
                }
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                };
            } else throw CREAMClientError("There is no connection chain configured.");
            Arc::XMLNode id, fs;
            (*resp)["JobRegisterResponse"]["result"]["JobId"]["id"].New(id);
            (*resp)["Fault"]["faultstring"].New(fs);

            // Testing: write the incoming SOAP message
            std::cout << "jobStartResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;
            
            std::string faultstring = "";
            faultstring=(std::string)fs;
            if ((*resp)["JobPurgeResponse"]["Response"]["JobUnknownFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["JobUnknownFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["JobStatusInvalidFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["JobStatusInvalidFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["DelegationIdMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["DelegationIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["DateMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["DateMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["LeaseIdMismatchFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["LeaseIdMismatchFault"]["FaultCause"] + "\n";
            if ((*resp)["JobPurgeResponse"]["Response"]["GenericFault"]) faultstring += (*resp)["JobPurgeResponse"]["Response"]["GenericFault"]["FaultCause"] + "\n";
            delete resp;
            if (faultstring!="") throw CREAMClientError(faultstring);
            if (!(bool)id && (std::string)id=="") throw CREAMClientError("Job starting failed.");
        } // CREAMClient::startJob()
       
        std::string CREAMClient::submit(std::string& jsdl_text) throw(CREAMClientError) {
            std::string jobid;
            
            // Register the new job
            try {
                jobid = this->registerJob(jsdl_text);
            } catch (CREAMClientError cce) {
                throw cce;
            }
            
            // File submission should be here
            
            // Start executing of the job
            try {
                this->startJob(jobid);
            } catch (CREAMClientError cce) {
                throw cce;
            }
            
            return jobid;
        }
        
        void CREAMClient::createDelegation(std::string& delegation_id) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating delegation.");
            
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns1;
            ns1["ns1"]="http://www.gridsite.org/namespaces/delegation-2";
            Arc::XMLNode getProxyReqRequest = req.NewChild("ns1:getProxyReqRequest", ns1);
            Arc::XMLNode delegid = getProxyReqRequest.NewChild("ns1:delegationID", ns1);
            delegid.Set(delegation_id);
            Arc::PayloadSOAP* resp = NULL;
            
            // Testing: write the outgoing SOAP message
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;
            
            // Send job request
            if(client) {
                Arc::MCC_Status status = client->process("", &req,&resp);
                if(!status) {
                    logger.msg(Arc::ERROR, "Submission request failed.");
                    throw CREAMClientError("Submission request failed.");
                }
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                };
            } else throw CREAMClientError("There is no connection chain configured.");

            // Testing: write the incoming SOAP message
            std::cout << "getProxyReqResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;
                        
            std::string getProxyReqReturnValue;
            if ((bool)(*resp) && (bool)(*resp)["JobRegisterResponse"]["getProxyReqReturn"] && (std::string)(*resp)["JobRegisterResponse"]["getProxyReqReturn"] != "") getProxyReqReturnValue = (std::string)(*resp)["JobRegisterResponse"]["getProxyReqReturn"];
            else throw CREAMClientError("Delegation creating failed.");
            delete resp;

            std::string proxy = getProxy();
            std::string signedcert;
            char *cert=NULL; 
            int timeleft = getCertTimeLeft(proxy);
            
            if (makeProxyCert(&cert,(char*) getProxyReqReturnValue.c_str(),(char*) proxy.c_str(),(char *) proxy.c_str(),timeleft)) throw CREAMClientError("DelegateProxy failed.");
            signedcert.assign(cert);
  
            Arc::PayloadSOAP req2(cream_ns);
            Arc::XMLNode putProxyRequest = req2.NewChild("ns1:putProxyRequest", ns1);
            
            Arc::XMLNode delegid_node = putProxyRequest.NewChild("ns1:delegationID", ns1);
            delegid_node.Set(delegation_id);
            Arc::XMLNode proxy_node = putProxyRequest.NewChild("ns1:proxy", ns1);
            proxy_node.Set(signedcert);
            resp = NULL;
            
            // Testing: write the outgoing SOAP message
            req.GetDoc(test,true);
            std::cout << test << std::endl;
            
            // Send job request
            if(client) {
                Arc::MCC_Status status = client->process("", &req2,&resp);
                if(!status) {
                    logger.msg(Arc::ERROR, "Submission request failed.");
                    throw CREAMClientError("Submission request failed.");
                }
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                };
            } else throw CREAMClientError("There is no connection chain configured.");

            // Testing: write the incoming SOAP message
            std::cout << "putProxyResponse:" << std::endl;
            (*resp).GetDoc(test,true);
            std::cout << test << std::endl;
            
            //Error handling!!!
            
        } // CREAMClient::createDelegation()
    } // namespace cream
} // namespace arc
