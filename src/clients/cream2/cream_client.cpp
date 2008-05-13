// cream_client.cpp

#include "cream_client.h"
#include "openSSLFunctions.cpp"

namespace Arc{
    namespace Cream{
        CREAMClientError::CREAMClientError(const std::string& what) : std::runtime_error(what){  }
        Arc::Logger CREAMClient::logger(Arc::Logger::rootLogger, "A-REX-Client");
        
        static void set_cream_namespaces(Arc::NS& ns) {
            ns["SOAP-ENV"]="http://schemas.xmlsoap.org/soap/envelope/";
            ns["SOAP-ENC"]="http://schemas.xmlsoap.org/soap/encoding/";
            ns["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
            ns["xsd"]="http://www.w3.org/2001/XMLSchema";
            ns["ns1"]="http://www.gridsite.org/namespaces/delegation-2";
            ns["ns2"]="http://glite.org/2007/11/ce/cream/types";
            ns["ns3"]="http://glite.org/2007/11/ce/cream";
        }
        
        CREAMClient::CREAMClient(std::string configFile) throw(CREAMClientError):client_config(NULL),client_loader(NULL),client(NULL),client_entry(NULL) {
            logger.msg(Arc::INFO, "Creating a CREAM client.");
        
            if (configFile=="" && getenv("ARC_CREAM_CONFIG")) configFile = getenv("ARC_CREAM_CONFIG");
            if (configFile=="") configFile = "./cream_client.xml";
        
            client_config = new Arc::Config(configFile.c_str());
            if(!*client_config) {
                logger.msg(Arc::ERROR, "Failed to load client configuration.");
                throw CREAMClientError("Failed to load client configuration.");
            }
        
            client_loader = new Arc::Loader(client_config);
            logger.msg(Arc::INFO, "Client side MCCs are loaded.");
            client_entry = (*client_loader)["soap"];
            if(!client_entry) {
                logger.msg(Arc::ERROR, "Client chain does not have entry point.");
                throw CREAMClientError("Client chain does not have entry point.");
            }
        
            set_cream_namespaces(cream_ns);
        }
        
        CREAMClient::CREAMClient(const Arc::URL& url, const Arc::MCCConfig& cfg) throw(CREAMClientError):client_config(NULL),client_loader(NULL),client(NULL),client_entry(NULL) {
            logger.msg(Arc::INFO, "Creating a CREAM client.");
            client = new Arc::ClientSOAP(cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
            set_cream_namespaces(cream_ns);
        }
        
        CREAMClient::~CREAMClient() {
            if(client_loader) delete client_loader;
            if(client_config) delete client_config;
            if(client) delete client;
        }
        
        std::string CREAMClient::stat(const std::string& jobid) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending a status request.");
            
            // Massive TODO
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobStatusRequest = req.NewChild("JobStatusRequest", ns2);
              Arc::XMLNode jobId = jobStatusRequest.NewChild("jobId", ns2);
                Arc::XMLNode id = jobId.NewChild("id", ns2);
                id.Set(jobid);
                Arc::XMLNode creamURL = jobId.NewChild("creamURL", ns2);
    
            // Send status request
            Arc::PayloadSOAP* resp = NULL;
            
            std::string test;
            req.GetDoc(test,true);
            std::cout << test << std::endl;

            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobStatus",&req,&resp);
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                }
            } else if(client_entry) {
                Arc::Message reqmsg;
                Arc::Message repmsg;
                Arc::MessageAttributes attributes_req;
                attributes_req.set("SOAP:ACTION","http://glite.org/2007/11/ce/cream/JobStatus");
                Arc::MessageAttributes attributes_rep;
                Arc::MessageContext context;
                reqmsg.Payload(&req);
                reqmsg.Attributes(&attributes_req);
                reqmsg.Context(&context);
                repmsg.Attributes(&attributes_rep);
                repmsg.Context(&context);
                Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
                if(!status) {
                    logger.msg(Arc::ERROR, "A status request failed.");
                    throw CREAMClientError("The status request failed.");
                }
                logger.msg(Arc::INFO, "A status request succeed.");
                if(repmsg.Payload() == NULL) {
                    logger.msg(Arc::ERROR, "There were no response to a status request.");
                    throw CREAMClientError("There were no response.");
                }
                try {
                    resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
                } catch(std::exception&) { };
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"The response of a status request was not a SOAP message.");
                    delete repmsg.Payload();
                    throw CREAMClientError("The response is not a SOAP message.");
                }
            } else {
                throw CREAMClientError("There is no connection chain configured.");
            };
            Arc::XMLNode name, failureReason;
            (*resp)["JobStatusResponse"]["result"]["jobStatus"]["name"].New(name);
            if ((*resp)["JobStatusResponse"]["result"]["jobStatus"]["failureReason"]) (*resp)["JobStatusResponse"]["result"]["jobStatus"]["failureReason"].New(failureReason);
            std::string statusarc;
            std::string status = (std::string)name;
            std::string faultstring = (std::string)failureReason;
            delete resp;
            
            //translate to ARC terminology
            if (!status.compare("REGISTERED")){
                statusarc.assign("ACCEPTING");
            }
            if (!status.compare("PENDING")){
                statusarc.assign("SUBMITTING");
            }
            if (!status.compare("IDLE")){
                statusarc.assign("INLRMS:Q");
            }
            if (!status.compare("RUNNING")){
                statusarc.assign("INLRMS:R");
            }
            if (!status.compare("REALLY-RUNNING")){
                statusarc.assign("INLRMS:R");
            }
            if (!status.compare("HELD")){
                statusarc.assign("INLRMS:S");
            }
            if (!status.compare("CANCELLED")){
                statusarc.assign("KILLED");
            }
            if (!status.compare("DONE-OK")){
                statusarc.assign("FINISHED");
            }
            if (!status.compare("DONE-FAILED")){ // Job failure
                statusarc.assign("FAILED");
            }
            if (!status.compare("ABORTED")){ //failure at LRMS level
                statusarc.assign("FAILES");
            }
            if (!status.compare("UNKNOWN")){ 
                statusarc.assign("EXECUTED"); //Chosen because this seems to be the default ARC behaviour
            }
            
            if (faultstring!="") throw CREAMClientError(faultstring);
            else if (status=="") throw CREAMClientError("The job status could not be retrieved.");
            else return statusarc;
            
        }  // CREAMClient::stat()
        
        void CREAMClient::cancel(const std::string& jobid) throw(CREAMClientError) {
            logger.msg(Arc::INFO, "Creating and sending request to terminate a job.");
    
            Arc::PayloadSOAP req(cream_ns);
            Arc::NS ns2;
            ns2["ns2"]="http://glite.org/2007/11/ce/cream/types";
            Arc::XMLNode jobStatusRequest = req.NewChild("JobCancelRequest", ns2);
              Arc::XMLNode jobId = jobStatusRequest.NewChild("jobId", ns2);
                Arc::XMLNode id = jobId.NewChild("id", ns2);
                id.Set(jobid);
                Arc::XMLNode creamURL = jobId.NewChild("creamURL", ns2);
    
            // Send kill request
            Arc::PayloadSOAP* resp = NULL;
            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobCancel",&req,&resp);
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                }
            } else if(client_entry) {
                Arc::Message reqmsg;
                Arc::Message repmsg;
                Arc::MessageAttributes attributes_req;
                attributes_req.set("SOAP:ACTION","http://glite.org/2007/11/ce/cream/JobCancel");
                Arc::MessageAttributes attributes_rep;
                Arc::MessageContext context;
                reqmsg.Payload(&req);
                reqmsg.Attributes(&attributes_req);
                reqmsg.Context(&context);
                repmsg.Attributes(&attributes_rep);
                repmsg.Context(&context);
                Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
                if(!status) {
                    logger.msg(Arc::ERROR, "A job termination request failed.");
                    throw CREAMClientError("The job termination request failed.");
                }
                logger.msg(Arc::INFO, "A job termination request succeed.");
                if(repmsg.Payload() == NULL) {
                    logger.msg(Arc::ERROR,"There was no response to a job termination request.");
                    throw CREAMClientError("There was no response to the job termination request.");
                }
                try {
                    resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
                } catch(std::exception&) { };
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,
                    "The response of a job termination request was not a SOAP message");
                    delete repmsg.Payload();
                    throw CREAMClientError("The response is not a SOAP message.");
                }
            } else {
                throw CREAMClientError("There is no connection chain configured.");
            };

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
            Arc::XMLNode jobStatusRequest = req.NewChild("JobPurgeRequest", ns2);
              Arc::XMLNode jobId = jobStatusRequest.NewChild("jobId", ns2);
                Arc::XMLNode id = jobId.NewChild("id", ns2);
                id.Set(jobid);
                Arc::XMLNode creamURL = jobId.NewChild("creamURL", ns2);
            
            // Send clean request
            Arc::PayloadSOAP* resp = NULL;
            if(client) {
                Arc::MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobPurge",&req,&resp);
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                }
            } else if(client_entry) {
                Arc::Message reqmsg;
                Arc::Message repmsg;
                Arc::MessageAttributes attributes_req;
                attributes_req.set("SOAP:ACTION","http://glite.org/2007/11/ce/cream/JobPurge");
                Arc::MessageAttributes attributes_rep;
                Arc::MessageContext context;
                reqmsg.Payload(&req);
                reqmsg.Attributes(&attributes_req);
                reqmsg.Context(&context);
                repmsg.Attributes(&attributes_rep);
                repmsg.Context(&context);
                Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
                if(!status) {
                    logger.msg(Arc::ERROR, "A job cleaning request failed.");
                    throw CREAMClientError("The job cleaning request failed.");
                }
                logger.msg(Arc::INFO, "A job cleaning request succeed.");
                if(repmsg.Payload() == NULL) {
                    logger.msg(Arc::ERROR,"There was no response to a job cleaning request.");
                    throw CREAMClientError("There was no response to the job cleaning request.");
                }
                try {
                    resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
                } catch(std::exception&) { };
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"The response of a job cleaning request was not a SOAP message");
                    delete repmsg.Payload();
                    throw CREAMClientError("The response is not a SOAP message.");
                }
            } else {
              throw CREAMClientError("There is no connection chain configured.");
            };
        
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
 /*       
        std::string CREAMClient::submit(std::istream& jsdl_file,CREAMFileList& file_list,bool delegate) throw(CREAMClientError) {
            std::string jobid, faultstring;
            file_list.resize(0);
            logger.msg(Arc::INFO, "Creating and sending request.");
            Arc::PayloadSOAP req(cream_ns);
            Arc::XMLNode op = req.NewChild("bes-factory:CreateActivity");
            Arc::XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
            std::string jsdl_str; 
            std::getline<char>(jsdl_file,jsdl_str,0);
            act_doc.NewChild(Arc::XMLNode(jsdl_str));
            act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces
            Arc::PayloadSOAP* resp = NULL;
            
            XMLNode ds = act_doc["jsdl:JobDefinition"]["jsdl:JobDescription"]["jsdl:DataStaging"];
            for(;(bool)ds;ds=ds[1]) {
                XMLNode source = ds["jsdl:Source"];
                XMLNode target = ds["jsdl:Target"];
                if((bool)source) {
                    std::string s_name = ds["jsdl:FileName"];
                    if(!s_name.empty()) {
                        XMLNode x_url = source["jsdl:URI"];
                        std::string s_url = x_url;
                        if(s_url.empty()) {
                            s_url="./"+s_name;
                        } else {
                            URL u_url(s_url);
                            if(!u_url) {
                                if(s_url[0] != '/') s_url="./"+s_url;
                            } else {
                                if(u_url.Protocol() == "file") {
                                    s_url=u_url.Path();
                                    if(s_url[0] != '/') s_url="./"+s_url;
                                } else {
                                    s_url.resize(0);
                                };
                            };
                        };
                        if(!s_url.empty()) {
                            x_url.Destroy();
                            CREAMFile file(s_name,s_url);
                            file_list.push_back(file);
                        };
                    };
                };
            }; 
            act_doc.GetXML(jsdl_str);
            logger.msg(Arc::VERBOSE, "Job description to be sent: %s",jsdl_str);

            // Try to figure out which credentials are used
            // TODO: Method used is unstable beacuse it assumes some predefined 
            // structure of configuration file. Maybe there should be some 
            // special methods of ClientTCP class introduced.
            std::string deleg_cert;
            std::string deleg_key;
            if(delegate) {
                client->Load(); // Make sure chain is ready
                Arc::XMLNode tls_cfg = find_xml_node((client->GetConfig())["Chain"],"Component","name","tls.client");
                if(tls_cfg) {
                    deleg_cert=(std::string)(tls_cfg["ProxyPath"]);
                    if(deleg_cert.empty()) {
                        deleg_cert=(std::string)(tls_cfg["CertificatePath"]);
                        deleg_key=(std::string)(tls_cfg["KeyPath"]);
                    } else {
                        deleg_key=deleg_cert;
                    };
                };
                if(deleg_cert.empty() || deleg_key.empty()) {
                    std::string s;
                    client->GetConfig().GetXML(s);
                    std::cerr<<s<<std::endl;
                    logger.msg(Arc::ERROR,"Failed to find delegation credentials in client configuration.");
                    throw CREAMClientError("Failed to find delegation credentials in client configuration.");
                };
            };

            // Send job request + delegation
            if(client) {
                {
                    if(delegate) {
                        Arc::DelegationProviderSOAP deleg(deleg_cert,deleg_key);
                        logger.msg(Arc::INFO, "Initiating delegation procedure");
                        if(!deleg.DelegateCredentialsInit(*(client->GetEntry()),&(client->GetContext()))) {
                            logger.msg(Arc::ERROR,"Failed to initiate delegation.");
                            throw CREAMClientError("Failed to initiate delegation.");
                        };
                        deleg.DelegatedToken(op);
                    };
                };
                Arc::MCC_Status status = client->process("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity", &req,&resp);
                if(!status) {
                    logger.msg(Arc::ERROR, "Submission request failed.");
                    throw CREAMClientError("Submission request failed.");
                }
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"There was no SOAP response.");
                    throw CREAMClientError("There was no SOAP response.");
                };
            } else if (client_entry) {
                Arc::Message reqmsg;
                Arc::Message repmsg;
                Arc::MessageAttributes attributes_req;
                attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity");
                Arc::MessageAttributes attributes_rep;
                Arc::MessageContext context;
                {
                    if(delegate) {
                        Arc::DelegationProviderSOAP deleg(deleg_cert,deleg_key);
                        logger.msg(Arc::INFO, "Initiating delegation procedure");
                        if(!deleg.DelegateCredentialsInit(*client_entry,&context)) {
                            logger.msg(Arc::ERROR,"Failed to initiate delegation.");
                            throw CREAMClientError("Failed to initiate delegation.");
                        };
                        deleg.DelegatedToken(op);
                    };
                };
                reqmsg.Payload(&req);
                reqmsg.Attributes(&attributes_req);
                reqmsg.Context(&context);
                repmsg.Attributes(&attributes_rep);
                repmsg.Context(&context);
                Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
                if(!status) {
                    logger.msg(Arc::ERROR, "Submission request failed.");
                    throw CREAMClientError("Submission request failed.");
                }
                logger.msg(Arc::INFO, "Submission request succeed.");
                if(repmsg.Payload() == NULL) {
                    logger.msg(Arc::ERROR, "There were no response to a submission request.");
                    throw CREAMClientError("There were no response to the submission request.");
                }
                try {
                    resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
                } catch(std::exception&) { };
                if(resp == NULL) {
                    logger.msg(Arc::ERROR,"A response to a submission request was not a SOAP message.");
                    delete repmsg.Payload();
                    throw CREAMClientError("The response to the submission request was not a SOAP message.");
                };
            } else throw CREAMClientError("There is no connection chain configured.");
            Arc::XMLNode id, fs;
            (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
            (*resp)["Fault"]["faultstring"].New(fs);
            id.GetDoc(jobid);
            faultstring=(std::string)fs;
            delete resp;
            if (faultstring=="") return jobid;
            else throw CREAMClientError(faultstring);
        }    
 */
        
    } // namespace cream
} // namespace arc
