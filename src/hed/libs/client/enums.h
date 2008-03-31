/**
 * GLUE2 enums
 */
#ifndef ARCLIB_ENUMS
#define ARCLIB_ENUMS

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

//closed enumeration
enum ServingState_t{production,
		    draining,
		    queueing,
		    closed};

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


#endif
