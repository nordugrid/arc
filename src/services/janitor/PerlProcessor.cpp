#include <pthread.h>
#include <string>


#include "PerlProcessor.h"




// TODO: IF PERL IS NOT INSTALLED PROPERLY, THE dRE WILL NOT BE COMPILED!!!


EXTERN_C void boot_DynaLoader (pTHXo_ CV* cv);

//#pragma warning(push)
//#pragma warning(disable : 4100)     // my_perl: unreferenced formal parameter
EXTERN_C void
xs_init(pTHXo)
{
     char *file = __FILE__;
     dXSUB_SYS;

     /* DynaLoader is a special case */
     newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);

     // add other modules here. Please don't ask me about this; I don't know
     // perlguts. That's why this class exists!
     // newXS("Socket::bootstrap", boot_Socket, file);
}

#define SIZE 1024



namespace DREService
{

	Arc::Logger  PerlProcessor::logger(Arc::Logger::rootLogger, "PerlProcessor");


	PerlProcessor::PerlProcessor(int threadNumber, TaskQueue* pTaskQueue, TaskSet* pTaskSet, const std::string perlfile) {

		this->threadNumber = threadNumber;

		pThreadinterface =  (PerlProcessor::ThreadInterface*) calloc(1,sizeof(PerlProcessor::ThreadInterface));
		pThreadinterface->state      = PROCESSING;
		pThreadinterface->pTaskQueue = pTaskQueue;
		pThreadinterface->pTaskSet   = pTaskSet;

		pThreadinterface->pPerlfile = (char *) calloc(perlfile.length() + 1, sizeof(char));
		strcpy(pThreadinterface->pPerlfile, perlfile.c_str());

		pthread_mutex_init(&(pThreadinterface->naming_mutex), NULL);
		pThreadinterface->naming_counter = 0;

		pThreads = new pthread_t[threadNumber];
		for(int i = 0; i < threadNumber; i++){
			pthread_create(&(pThreads[i]), NULL, PerlProcessor::run,(void*) pThreadinterface);   //??? GLEICHER TASK?
		}
	}



	PerlProcessor::~PerlProcessor(){

		logger.msg(Arc::INFO, "Stopping Master Thread.");

		pThreadinterface->state = EXITING;
		for(int i = 0; i < threadNumber; i++){
			pthread_join(pThreads[i], NULL); 
		}

		logger.msg(Arc::INFO, "Master Thread is deleting threads.");

		delete [] pThreads;
		logger.msg(Arc::INFO, "Master Thread stopped.");

		free(pThreadinterface->pPerlfile);
		free(pThreadinterface);

		pthread_mutex_destroy(&(pThreadinterface->naming_mutex));
	}

	// TODO: We have more than one interpreter instance running at the same time. This is feasible, 
	// but only if you used the -DMULTIPLICITY flag when building Perl. By default, that 
	// sets PL_perl_destruct_level to 1. 
	void* PerlProcessor::run(void* pVoid){

 		ThreadInterface* pTI = ((ThreadInterface*)pVoid);

		int threadID;
		pthread_mutex_lock(&(pTI->naming_mutex));
			(pTI->naming_counter)++;
			threadID = (pTI->naming_counter);
		pthread_mutex_unlock(&(pTI->naming_mutex));

// 		logger.msg(Arc::INFO, "Thread %d, is summoned.", threadID);

		int commandPfd[2];
		int dataInPfd[2];
		int dataOutPfd[2];
		int nread;
		int pid;
		char buf[SIZE];
		char etx[2];   // end of text
		etx[0] = 0x03;
		etx[1] = 0x00;
		
		if (pipe(commandPfd) == -1 || pipe(dataInPfd) == -1 || pipe(dataOutPfd) == -1)
		{
			logger.msg(Arc::INFO, "Thread %d, Pipes failed",threadID);
			return pVoid;
		}
		if ((pid = fork()) < 0)
		{
			logger.msg(Arc::INFO, "Thread %d, Fork failed", threadID);
			return pVoid;
		}
		
		if (pid == 0){

			/* child */
			close(commandPfd[1]);
			close(dataInPfd[1]);
			close(dataOutPfd[0]);

			char command = '1';

			std::string message = "";
			std::string temp    = "";
			std::string result  = "";

			PerlInterpreter *my_perl = 0;
		
			if((my_perl = perl_alloc()) == 0) {
				// Using the logger leads to dead locks
				printf("Thread %d:      Child was unable to allocate memory for Perl.\n", threadID);
				close(commandPfd[0]);
				close(dataInPfd[0]);
				close(dataOutPfd[1]);
				exit(0);
			}

			perl_construct(my_perl);
			char *embedding[] = { (char*)"", pTI->pPerlfile}; 

			if(perl_parse(my_perl, xs_init, 2, embedding, NULL)) {
				printf("Thread %d:      An error occurred while initializing Perl.\n", threadID);
				close(commandPfd[0]);
				close(dataInPfd[0]);
				close(dataOutPfd[1]);
				exit(0);
			}

			// Run main function to initialize global variables
			perl_run(my_perl);


			do{
				printf("Thread %d:      Child is ready.\n", threadID);
				if((nread = read(commandPfd[0], buf, SIZE)) != 0){
					if(strncmp(buf,"1",1) == 0){
						command = '1';	// work
					}else{
						command = '0';	// terminate
					}
				}else{
					break;
				}

				if(command == '1'){ // buf == work
					message = "";
					/** Get task */
					while ((nread = read(dataInPfd[0], buf, SIZE)) != 0){ // handle input of length nread
						temp.append(buf);
						size_t pos = temp.find(etx);
						if(pos != std::string::npos){ // buf contains the 0x03 signal
							message = temp.substr(0,pos);
							temp    = temp.substr(pos+1);
							break;
						}
					}
					message.append("\r\n");
					/** Process message */
 					printf("Thread %d:      Child received message %s.\n", threadID, message.c_str());
					
					char* rstr = NULL;
					perl_call_va(my_perl,(char*) "process",
						"s", message.c_str(),
						"OUT",
						"s",  &rstr,         //Output parameter 
						NULL);     //Don't forget this! 
					if(rstr != 0){
						result.assign(rstr);
						// Leads to bad exceptions... so it's better not to do that
						//delete rstr;
					}else{
						result = "";
					}

					// check $@ 
					if(SvTRUE(ERRSV) || result.empty()){
						printf( "Thread %d:      Experienced an error while function was evaluated:\n  %s\n",  threadID,SvPV(ERRSV,PL_na));	
						// makefault
						Arc::NS ns("", "urn:perlprocessor");
						Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns,true);
						Arc::SOAPFault* fault = outpayload->Fault();
						if(fault) {
							fault->Code(Arc::SOAPFault::Sender);
							if(result.empty()){
								fault->Reason("Perl returned no content.");
							}else{
								fault->Reason(SvPV(ERRSV,PL_na));
							}
						};
						outpayload->GetXML(result, 1);
					}
					/** */


					/** Return result*/
					write(dataOutPfd[1], result.c_str(), result.length());	
					write(dataOutPfd[1], etx, strlen(etx));	
					/** */

				}
			}while(command == '1');

			logger.msg(Arc::INFO, "Thread %d, child is terminating.", threadID);

 			PL_perl_destruct_level = 0;
			perl_destruct(my_perl);

 			perl_free(my_perl);

			close(commandPfd[0]);
			close(dataInPfd[0]);
			close(dataOutPfd[1]);
		} else {
			/* parent */
			close(commandPfd[0]);
			close(dataInPfd[0]);
			close(dataOutPfd[1]);

			const char* work = "1\0";
			const char* end  = "0\0";

			std::string message = "";
			std::string temp    = "";
			do{

				logger.msg(Arc::INFO, "Thread %d is ready.", threadID);
				Task* pTask = pTI->pTaskQueue->shiftTask();
				if(pTask != 0){
					logger.msg(Arc::INFO, "Thread %d got Task %d.", threadID, pTask->getTaskID());

					/** Send command*/
					write(commandPfd[1], work, 1);
					/** */

					/** Send task */
					Arc::PayloadSOAP* inpayload  = NULL;

					try {
						inpayload = dynamic_cast<Arc::PayloadSOAP*>(pTask->getRequestMessage()->Payload());
					} catch(std::exception& e) { };

					if(!inpayload) {
						logger.msg(Arc::ERROR, "Thread %d, Input is not SOAP", threadID);
// 						return makeFault(outmsg, "Received message was not a valid SOAP message.");
					};
					std::string xml;
					inpayload->GetXML(xml, 0);

					write(dataInPfd[1], xml.c_str(), xml.length());	
					write(dataInPfd[1], etx, strlen(etx));	
					/** */

					message = "";
					/** Get result */
					while ((nread = read(dataOutPfd[0], buf, SIZE)) != 0){ // handle input of length nread

						temp.append(buf);
						size_t pos = temp.find(etx);
						if(pos != std::string::npos){ // buf contains the 0x03 signal
							message = temp.substr(0,pos);
							temp    = temp.substr(pos+1);
							break;
						}
					}
					/**  */
					logger.msg(Arc::INFO, "Thread %d:      Task %d       Result:\n%s\n", threadID, pTask->getTaskID(), message.c_str());

					pTI->pTaskSet->addTask(pTask);
				}else{
					logger.msg(Arc::INFO, "Thread %d, TaskQueue returned empty Task.", threadID);
					sleep(2);
				}
			}while(pTI->state == PROCESSING);
			
			strcpy(buf, "0");  // command for the end
			write(commandPfd[1], buf, strlen(buf)+1);
				
			close(commandPfd[1]);
			close(dataInPfd[1]);
			close(dataOutPfd[0]);
		}




	}






































/*
 *
 *
 *
 *
 * WARNING: If output is "s", that a pointer to a char array (char**) has to be passed. The
 *          array itself will be allocated with new within the method with the proper size
 *          and has to be delete outside this method if no longer needed.
 */
int PerlProcessor::perl_call_va (PerlInterpreter* my_perl, char *subname, ...)
{
    char *p;
    char *str = NULL; int i = 0; double d = 0;
    int  nret = 0; /* number of return params expected*/
    int  ax;
    int ii=0;

    typedef struct {
      char type;       
      void *pdata;
    } Out_Param;


    Out_Param op[20];
    va_list vl;
    int out = 0;
    int result = 0;

    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    va_start (vl, subname);
 
    /*printf ("Entering perl_call %s\n", subname);*/
    while (p = va_arg(vl, char *)) {
        /*printf ("Type: %s\n", p);*/
        switch (*p)
        {
        case 's' :
            if (out) {
                op[nret].pdata = (void*) va_arg(vl, char **); // huuu
                op[nret++].type = 's';
            } else {
                str = va_arg(vl, char *);
                 /*printf ("IN: String %s\n", str);*/
                ii = strlen(str);
                XPUSHs(sv_2mortal(newSVpv(str,ii)));
            }
            break;
        case 'i' :
            if (out) {
                op[nret].pdata = (void*) va_arg(vl, int *);
                op[nret++].type = 'i';
            } else {
                ii = va_arg(vl, int);
                /*printf ("IN: Int %d\n", ii);*/
                XPUSHs(sv_2mortal(newSViv(ii)));
            }
            break;
        case 'd' :
            if (out) {
                op[nret].pdata = (void*) va_arg(vl, double *);
                op[nret++].type = 'd';
            } else {
               d = va_arg(vl, double);
               /*printf ("IN: Double %f\n", d);*/
               XPUSHs(sv_2mortal(newSVnv(d)));
            }
            break;
        case 'O':
            out = 1;  /* Out parameters starting */
            break;
        default:
             fprintf (stderr, "perl_eval_va: Unknown option \'%c\'.\n"
                               "Did you forget a trailing NULL ?\n", *p);
            return 0;
        }
    }
   
    va_end(vl);
 
    PUTBACK;
    result = perl_call_pv(subname, (nret == 0) ? G_DISCARD :
                                   (nret == 1) ? G_SCALAR  :
                                                 G_ARRAY  );
 
    
 
    SPAGAIN;
    /*printf ("nret: %d, result: %d\n", nret, result);*/
    if (nret > result)
        nret = result;
 
    for (i = --nret; i >= 0; i--) {
        switch (op[i].type) {
        case 's':
            str = POPp;
	    *((char**)op[i].pdata) = new char[strlen(str)];
            /*printf ("String: %s\n", str);*/
            strcpy(*((char**)op[i].pdata), str);
            /* printf ("String: %s\n",*((char**)op[i].pdata));*/
            break;
        case 'i':
            ii = POPi;
            /*printf ("Int: %d\n", ii);*/
            *((int *)(op[i].pdata)) = ii;
            break;
        case 'd':
            d = POPn;
            /*printf ("Double: %f\n", d);*/
            *((double *) (op[i].pdata)) = d;
            break;
        }
    }

    FREETMPS ;
    LEAVE ;
    return result;
}

















}

