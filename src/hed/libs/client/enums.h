/**
 * GLUE2 enums
 */
#ifndef GLUE2_ENUMS
#define GLUE2_ENUMS

#include <string>

//enums presently replaced with typedefs in order 
//to avoid crashes during runtime

typedef std::string ServingState_t;
typedef std::string ComputingActivityState_t;
typedef std::string Staging_t;
typedef std::string LRMSType_t;
typedef std::string NetworkInfo_t;
typedef std::string SchedulingPolicy_t;
typedef std::string ServingState_t;
typedef std::string EndpointType_t;
typedef std::string QualityLevel_t;
typedef std::string EndpointHealthState_t;
typedef std::string EndpointCapability_t;
typedef std::string ServiceCapability_t;
typedef std::string ServiceType_t;
typedef std::string ActivityType_t;
typedef std::string CPUMultiplicity_t;
typedef std::string OSFamily_t;
typedef std::string OSName_t;
typedef std::string Benchmark_t;
typedef std::string AppEnvState_t;
typedef std::string License_t;
typedef std::string ParallelType_t;
typedef std::string ApplicationHandle_t;
typedef std::string Platform_t;

/*

//closed enumeration
enum ServingState_t{production,
		    draining,
		    queueing,
		    closed};

//open enumeration
enum ComputingActivityState_t{running,
			      failed};

//open enumeration
enum Staging_t{none,
	       stagingin,
	       stagingout,
	       staginginout};

//open enumeration
enum LRMSType_t{openpbs,
		Isf};

//open enumeration
enum NetworkInfo_t{gigabitethernet,
		   myrinet,
		   infiniband};

//open enumeration
enum SchedulingPolicy_t{fairshare,
			fifo,
			RANDOM};

//open enumeration
enum EndpointType_t{webservice,
		    jndi};

//closed enumeration
enum QualityLevel_t{development,
		    testing,
		    pre_production,
		    PRODUCTION};

//closed enumeration
enum EndpointHealthState_t{ok,
			   warning,
			   critical,
			   unknown,
			   other};

//open enumeration
enum ServiceCapability_t{security_authentication,
			 security_credentialStorage,
			 security_delegation,
			 security_authorization,
			 security_identitymapping,
			 security_attributeauthority,
			 security_accounting,
			 data_transfer,
			 data_management_transfer,
			 data_management_replica,
			 data_management_storage,
			 data_naming_resolver,
			 data_naming_scheme,
			 data_access_relational,
			 data_access_xml,
			 data_access_flatfiles,
			 information_model,
			 information_discovery,
			 information_logging,
			 information_monitoring,
			 information_provenance,
			 execman_bes,
			 execman_jobdescription,
			 execman_jobmanager,
			 execman_executionandplanning,
			 execman_candidatesetgenerator,
			 execman_reservation};

//open enumeration
typedef ServiceCapability_t EndpointCapability_t;

//open enumeration
enum ServiceType_t{org_glite_wms,
		   org_glite_lb};

//open enumeration
enum ActivityType_t{computing};

//closed enumeration
enum CPUMultiplicity_t{singlecpu_singlecore,
		       singlecpu_mulitcore,
		       multicpu_singlecore,
		       mulitcpu_mutlicore};
//open enumeration
enum OSFamily_t{linux,
		macos,
		windows,
		solaris};


//open enumeration
enum OSName_t{scientificlinuxcern,
	      scientificlinux,
	      windowsxp,
	      windowsvista,
	      ubuntu,
	      debian,
	      centos,
	      leopard};

//open enumeration
enum Benchmark_t{specint2000,
		 specfp2000,
		 bogomips};

//open enumeration
enum AppEnvState_t{tested,
		   installed,
		   dynamic,
                   toberemoved};

//open enumeration
enum License_t{opensource,
	       commercial,
               unknown};

//open enumeration
enum ParallelType_t{Mpi,
	            openmp,
                    none};

//open enumeration
enum ApplicationHandle_t{module,
	                 softenv,
                         path,
                         executable};

*/

#endif
