#include <list>
#include <string>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <getopt.h>
#include <arc/ArcLocation.h>

#define _(A) (A)

static void arcusage(const std::string & progname, const char * optstring) {

	bool havegiisurlopt = false;

	std::cout << _("Usage") << ": ng" << progname
	          << " [" << _("options") << "]";
	
	if (progname == "acl")
	  std::cout << " get|put " << _("URL");
	else if (progname == "sub")
	  std::cout << " [" << _("filename") << " ...]";
	else if (progname == "stage")
	  std::cout << " [" << _("URL(s)") << "]";
	else if (progname != "sync" && progname != "test")
	  std::cout << " [" << _("job") << " ...]";

	std::cout << std::endl << std::endl;
	std::cout << _("Options") << ":" << std::endl;
	for (int i = 0; optstring[i]; i++) {
		if (optstring[i] == ':') continue;
		if (optstring[i + 1] == ':') {
			switch (optstring[i]) {
			case 'c':
              if (progname == "stage") {
                std::cout << _(
"  -c, -cancel    request_id    cancel a request"
                                       ) << std::endl;
              }
              else {				
                std::cout << _(
"  -c, -cluster   [-]name       explicity select or reject a specific cluster"
				                       ) << std::endl;
              }
              break;
			case 'C':

				std::cout << _(
"  -C, -clustlist [-]filename   list of clusters to select or reject"
				                       ) << std::endl;
				break;
			case 'd':
				std::cout << _(
"  -d, -debug     debuglevel    from -3 (quiet) to 3 (verbose) - default 0"
				                       ) << std::endl;
				break;
			case 'D':
				std::cout << _(
"      -dir                     download directory (the job directory will\n"
"                                 be created in this directory)"
				                       ) << std::endl;
				break;
			case 'e':
				std::cout << _(
"  -e, -xrsl      xrslstring    xrslstring describing the job to be submitted"
				                       ) << std::endl;
				break;
			case 'f':
				std::cout << _(
"  -f, -file      filename      xrslfile describing the job to be submitted"
				                       ) << std::endl;
				break;
			case 'g':
				std::cout << _(
"  -g, -giisurl   url           url to a GIIS"
				                       ) << std::endl;
				havegiisurlopt = true;
				break;
			case 'G':
				std::cout << _(
"  -G, -giislist  filename      list of GIIS urls"
				                       ) << std::endl;
				break;
			case 'i':
				std::cout << _(
"  -i, -joblist   filename      file containing a list of jobids"
				                       ) << std::endl;
				break;
			case 'J':
				std::cout << _(
"  -J, -job       jobid         submit the testjob given by the jobid"
				                       ) << std::endl;
				break;
			case 'k':
				std::cout << _(
"  -k, -kluster   [-]name       explicity select or reject a specific\n"
"                                 cluster as resubmission target"
				                       ) << std::endl;
				break;
			case 'K':
				std::cout << _(
"  -K, -klustlist [-]filename   list of clusters to select or reject as\n"
"                                 resubmission target"
				                       ) << std::endl;
				break;
			case 'o':
				std::cout << _(
"  -o, -joblist   filename      file where the jobids will be stored"
				                       ) << std::endl;
				break;
			case 'q':
				std::cout << _(
"  -q, -query     request_id    query the status of a request"
				                       ) << std::endl;
				break;
			case 'r':
				if ((progname != "cp") && (progname != "ls") && (progname != "acl") && (progname != "stage")) {
					std::cout << _(
"  -r, -runtime   time          run testjob 1 for the specified number of\n"
"                                 minutes"
					                       ) << std::endl;
				}
				else {
					std::cout << _(
"  -r, -recursive level         operate recursively (if possible) up to\n"
"                                 specified level (0 - no recursion)"
					                       ) << std::endl;
				}
				break;
			case 'R':
				std::cout << _(
"  -R, -retries   number        how many times to retry transfer of\n"
"                                 every file before failing"
				                       ) << std::endl;
				break;
			case 's':
				if (progname == "transfer") {
					std::cout << _(
"  -s, -source    URL           transfer from this URL"
					                       ) << std::endl;
				}
                else if (progname == "stage") {
					std::cout << _(
"  -s, -endpoint  URL           service endpoint to connect to"
					                       ) << std::endl;
				}
                  
				else {
					std::cout << _(
"  -s, -status    statusstr     only select jobs whose status is statusstr"
					                       ) << std::endl;
				}
				break;
			case 't':
				std::cout << _(
"  -t, -timeout   time          timeout in seconds (default 20)"
			    	                   ) << std::endl;
				break;
			case 'y':
				std::cout << _(
"  -y, -cache     path          path to local cache (use to put file into\n"
"                                 cache)"
				                       ) << std::endl;
				break;
			case 'Y':
				std::cout << _(
"  -Y, -cachedata path          path for cache data (if different from -y)"
				                       ) << std::endl;
				break;
			}
		}
		else {
			switch (optstring[i]) {
			case 'a':
				std::cout << _(
"  -a, -all                     all jobs"
				                       ) << std::endl;
				break;
			case 'D':
				std::cout << _(
"  -D  -dryrun                  add dryrun option"
				                       ) << std::endl;
				break;
			case 'e':
				std::cout << _(
"  -e, -stderr                  show stderr of the job"
				                       ) << std::endl;
				break;
			case 'E':
				std::cout << _(
"  -E, -certificate             prints info about installed user- and\n"
"                                 CA-certificates"
				                       ) << std::endl; 
				break;
			case 'f':
				if (progname == "clean") {
					std::cout << _(
"  -f, -force                   remove the job from the local list of jobs\n"
"                                 even if the job is not found in the MDS"
					                       ) << std::endl;
				}
				else if (progname == "cat") {
					std::cout << _(
"  -f, -follow                  show tail of requested file and follow it's\n"
"                                 changes"
                                           ) << std::endl;
				}
				else if (progname == "cp") {
					std::cout << _(
"  -f, -force                   if the destination is an indexing service\n"
"                                 and not the same as the source and the\n"
"                                 destination is already registered, then\n"
"                                 the copy is normally not done. However, if\n"
"                                 this option is specified the source is\n"
"                                 assumed to be a replica of the destination\n"
"                                 created in an uncontrolled way and the\n"
"                                 copy is done like in case of replication"
					                       ) << std::endl;
				}
				else if (progname == "rm") {
					std::cout << _(
"  -f, -force                   remove logical file name registration even\n"
"                                 if not all physical instances were removed"
					                       ) << std::endl;
				}
				else {
					std::cout << _(
"  -f, -force                   do not ask for verification"
					                       ) << std::endl;
				}
				break;
			case 'h':
				std::cout << _(
"  -h, -help                    print this help"
				                       ) << std::endl;
				break;
			case 'i':
				std::cout << _(
"  -i, -indicate                show progress indicator"
				                       ) << std::endl;
				break;
			case 'j':
				std::cout << _(
"  -j, -usejobname              use the jobname instead of the short ID as\n"
"                                 the job directory name"
				                       ) << std::endl;
				break;
			case 'l':
				if (progname == "cat") {
					std::cout << _(
"  -l, -gmlog                   show the grid manager's error log of the job"
					                       ) << std::endl;
				}
				else if (progname == "stage") {
					std::cout << _(
"  -l, -list                    list all request ids"
					                       ) << std::endl;
				}
				else {
					std::cout << _(
"  -l, -long                    long format (more information)"
					                       ) << std::endl;
				}
				break;
			case 'L':
				std::cout << _(
"  -L, -locations               show URLs of file locations"
				                       ) << std::endl;
				break;
			case 'n':
				std::cout << _(
"  -n, -nopassive               do not try to force passive transfer"
				                       ) << std::endl;
				break;
			case 'o':
				std::cout << _(
"  -o, -stdout                  show stdout of the job (default)"
				                       ) << std::endl;
				break;
			case 'O':
				std::cout << _(
"  -O, -configuration           prints user configuration"
				                       ) << std::endl;
				break;
			case 'p':
				if (progname == "cp") {
					std::cout << _(
"  -p, -passive                 use passive transfer (does not work if\n"
"                                 secure is on, default if secure is not\n"
"                                 requested)"
					                       ) << std::endl;
				}
				else {
					std::cout << _(
"  -p, -renewproxy              renew the job's proxy"
					                       ) << std::endl;
				}
				break;
			case 'P':
				std::cout << _(
"      -dumpxrsl                do not submit - dump transformed xrsl to\n"
"                                 stdout"
				                       ) << std::endl;
				break;
			case 'q':
				std::cout << _(
"  -q, -queues                  show information about clusters and queues"
				                       ) << std::endl;
				break;
			case 'r':
				std::cout << _(
"      -keep                    keep files on gatekeeper (do not clean)"
				                       ) << std::endl;
				break;
			case 'R':
				std::cout << _(
"  -R, -resources               prints user-authorization information for\n"
"                                 clusters and storage-elements"
				                       ) << std::endl;
				break;
			case 'T':
				std::cout << _(
"  -T, -notransfer              do not transfer file, just register it -\n"
"                                 destination must be non-existing meta-url"
				                       ) << std::endl;
				break;
			case 'u':
				std::cout << _(
"  -u, -secure                  use secure transfer (insecure by default)"
				                       ) << std::endl;
				break;
			case 'U':
				std::cout << _(
"  -U, -unknownattr             allow unknown attributes in job description"
				                       ) << std::endl;
				break;
			case 'v':
				std::cout << _(
"  -v, -version                 print version information"
				                       ) << std::endl;
				break;
			case 'V':
				std::cout << _(
"  -V, -vo                      prints member-information about nordugrid VO's"
				                       ) << std::endl << std::endl;
				break;
			case 'x':
				std::cout << _(
"  -x, -anonymous               use anonymous bind for MDS queries (default)"
				                       ) << std::endl;
				break;
			case 'X':
				std::cout << _(
"  -X, -gsi                     use gsi-gssapi bind for MDS queries"
				                       ) << std::endl;
				break;
			}
		}
	}
	if ((progname != "sync") &&
	    (progname != "test") &&
	    (progname != "acl") &&
	    (progname != "ls") &&
	    (progname != "cp") &&
	    (progname != "stage") &&
	    (progname != "transfer") &&
	    (progname != "rm")) {
		std::cout << std::endl;
		std::cout << _("Arguments") << ":" << std::endl;
		if (progname == "sub")
			std::cout << _(
"  filename ...                 xrslfiles describing the jobs to be submitted"
			                       ) << std::endl;
		else
			std::cout << _(
"  job ...                      list of jobids and/or jobnames"
			                       ) << std::endl;
	}
	if (progname=="test") {
		std::cout << std::endl;
		std::cout << _(
"Available test-jobs in this version of ngtest are:"
		                         ) << std::endl << std::endl;
		std::cout << _(
"Testjob 1: This test-job calculates prime-numbers for 2 minutes and saves the list. The source-code for the prime-number program, the Makefile and the executable is downloaded to the cluster chosen from various servers and the program is compiled before the run. In this way, the test-job constitute a fairly comprehensive test of the basic setup of a grid cluster."
		                         ) << std::endl;
		std::cout << _(
"Testjob 2: This test-job does nothing but print \"hello, grid\" to stdout."
		                         ) << std::endl;
		std::cout << _(
"Testjob 3: This test-job tests download capabilities of the chosen cluster by downloading 8 inputfiles over different protocols -- HTTP, FTP, gsiFTP and RLS."
		                         ) << std::endl;
	}
	if (havegiisurlopt) {
	  std::cout<< std::endl;
	  std::cout<<"Argument to (-g, -G) has format:"<<std::endl;
	  std::cout<<"GRID:URL e.g."<<std::endl;
	  std::cout<<"ARC0:ldap://grid.tsl.uu.se:2135/mds-vo-name=sweden,O=grid"<<std::endl;
	  std::cout<<"CREAM:ldap://cream.grid.upjs.sk:2170/o=grid"<<std::endl;

	  std::cout<< std::endl;
	  std::cout<<"Argument to (-c, -C) has format:"<<std::endl;
	  std::cout<<"GRID:URL e.g."<<std::endl;
	  std::cout<<"ARC0:ldap://grid.tsl.uu.se:2135/nordugrid-cluster-name=grid.tsl.uu.se,Mds-Vo-name=local,o=grid"<<std::endl;

	}
	std::cout << std::endl;
	std::cout << _(
"Default values for various options can be set in the ARC configuration\n"
"files. See man arc.conf(5) for details."
	                       ) << std::endl;
}

//function GetOption empty for now
static std::string GetOption(const std::string& newname,
                             const std::string& oldname) {

} //end GetOption

#define MAX_EXPECTED_OPTIONS 32

int main(int argc, char ** argv) {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
 
  Arc::ArcLocation::Init(argv[0]);
  
  if (!argv[0]) {
    logger.msg(Arc::ERROR, "No program name");
    return 1;
  }
  
  std::string progname = argv[0];
  
#ifdef WIN32
  //cut .exe|.com|.bat
  //progname = progname.substr(progname.length() - 6, 2);
#else
  size_t LastBeforeRealProgName = progname.rfind("arc");
  progname = progname.substr(LastBeforeRealProgName+3);
#endif
  
  std::cout<< "progname = "<<progname<<std::endl;

  char * optstring;
  
  if (progname == "cat")
    optstring = "ai:c:C:s:oeflt:d:xXvh";
  else if (progname == "clean")
    optstring = "ai:c:C:s:ft:d:xXvh";
  else if (progname == "get")
    optstring = "ai:c:C:s:D:jrt:d:xXvh";
  else if (progname == "kill")
    optstring = "ai:c:C:s:rt:d:xXvh";
  else if (progname == "renew")
    optstring = "ai:c:C:s:t:d:xXvh";
  else if (progname == "resub")
    optstring = "ai:c:C:s:k:K:g:G:o:DPrt:d:xXUvh";
  else if (progname == "resume")
    optstring = "ai:c:C:s:pt:d:xXvh";
  else if (progname == "stat")
    optstring = "ai:c:C:s:g:G:qlt:d:xXvh";
  else if (progname == "sub")
    optstring = "c:C:g:G:e:f:o:DPt:d:xXUvh";
  else if (progname == "sync")
    optstring = "c:C:g:G:ft:d:xXvh";
  else if (progname == "test")
    optstring = "EJ:ROVr:c:C:g:G:o:DPt:d:xXvh";
  else if (progname == "acl")
    optstring = "r:d:vh";
  else if (progname == "transfer")
    optstring = "s:t:d:vh";
  else if (progname == "stage")
    optstring = "q:c:lDd:r:s:t:vh";
  else{
    logger.msg(Arc::ERROR, "Unknown program name: %s",
	       progname);
    return 1;
  }
  
  struct option longoptions[MAX_EXPECTED_OPTIONS+1];
  int nopt = 0;
  for (int i = 0; optstring[i]; i++) {
    if (optstring[i] == ':') continue;
    if(nopt >= MAX_EXPECTED_OPTIONS) break;
    if (optstring[i + 1] == ':') {
      switch (optstring[i]) {
      case 'c':
	{
	  if (progname == "stage") {
	    struct option lopt = {"cancel", 1, 0, 'c'};
	    longoptions[nopt++] = lopt;
	  }
	  else {
	    struct option lopt = {"cluster", 1, 0, 'c'};
	    longoptions[nopt++] = lopt;
	  }
	  break;
	}
      case 'C':
	{
	  struct option lopt = {"clustlist", 1, 0, 'C'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'd':
	{
	  struct option lopt = {"debug", 1, 0, 'd'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'D':
	{
	  struct option lopt = {"dir", 1, 0, 'D'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'e':
	{
	  struct option lopt = {"xrsl", 1, 0, 'e'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'f':
	{
	  struct option lopt = {"file", 1, 0, 'f'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'g':
	{
	  struct option lopt = {"giisurl", 1, 0, 'g'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'G':
	{
	  struct option lopt = {"giislist", 1, 0, 'G'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'i':
	{
	  struct option lopt = {"joblist", 1, 0, 'i'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'J':
	{
	  struct option lopt = {"job", 1, 0, 'J'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'k':
	{
	  struct option lopt = {"kluster", 1, 0, 'c'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'K':
	{
	  struct option lopt = {"klustlist", 1, 0, 'C'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'o':
	{
	  struct option lopt = {"joblist", 1, 0, 'o'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'q':
	{
	  struct option lopt = {"query", 1, 0, 'q'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'r':
	{
	  if (progname == "test") {
	    struct option lopt = {"runtime", 1, 0, 'r'};
	    longoptions[nopt++] = lopt;
	  }
	  else {
	    struct option lopt = {"recursive", 1, 0, 'r'};
	    longoptions[nopt++] = lopt;
	  }
	  break;
	}
      case 'R':
	{
	  struct option lopt = {"retries", 1, 0, 'R'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 's':
	{
	  if (progname == "transfer") {
	    struct option lopt = {"source", 1, 0, 's'};
	    longoptions[nopt++] = lopt;
	  }
	  else if (progname == "stage") {
	    struct option lopt = {"service-endpoint", 1, 0, 's'};
	    longoptions[nopt++] = lopt;
	  }                          
	  else {
	    struct option lopt = {"status", 1, 0, 's'};
	    longoptions[nopt++] = lopt;
	  }
	  break;
	}
      case 't':
	{
	  struct option lopt = {"timeout", 1, 0, 't'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'y':
	{
	  struct option lopt = {"cache", 1, 0, 'y'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'Y':
	{
	  struct option lopt = {"cachedata", 1, 0, 'Y'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      }
    }//end if (optstring[i+1] = ":")
    else {
      switch (optstring[i]) {
      case 'a':
	{
	  struct option lopt = {"all", 0, 0, 'a'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'D':
	{
	  struct option lopt = {"dryrun", 0, 0, 'D'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'e':
	{
	  struct option lopt = {"stderr", 0, 0, 'e'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'E':
	{
	  struct option lopt = {"certificate", 0, 0, 'E'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'f':
	{
	  if (progname == "cat") {
	    struct option lopt = {"follow", 0, 0, 'f'};
	    longoptions[nopt++] = lopt;
	  }
	  else {
	    struct option lopt = {"force", 0, 0, 'f'};
	    longoptions[nopt++] = lopt;
	  }
	  break;
	}
      case 'h':
	{
	  struct option lopt = {"help", 0, 0, 'h'};
	  longoptions[nopt++] = lopt;
	  break;
				  }
      case 'i':
	{
	  struct option lopt = {"indicate", 0, 0, 'i'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'j':
	{
	  struct option lopt = {"usejobname", 0, 0, 'j'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'l':
	{
	  if (progname == "cat") {
	    struct option lopt = {"gmlog", 0, 0, 'l'};
	    longoptions[nopt++] = lopt;
	  }
	  else if (progname == "stage") {
	    struct option lopt = {"list", 0, 0, 'l'};
	    longoptions[nopt++] = lopt;
						}
	  else {
	    struct option lopt = {"long", 0, 0, 'l'};
	    longoptions[nopt++] = lopt;
	  }
	  break;
	}
      case 'L':
	{
	  struct option lopt = {"locations", 0, 0, 'L'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'n':
	{
	  struct option lopt = {"nopassive", 0, 0, 'n'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'o':
	{
	  struct option lopt = {"stdout", 0, 0, 'o'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'O':
	{
	  struct option lopt = {"configuration", 0, 0, 'O'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'p':
	{
	  if (progname == "cp") {
	    struct option lopt = {"passive", 0, 0, 'p'};
	    longoptions[nopt++] = lopt;
	  }
	  else {
	    struct option lopt = {"renewproxy", 0, 0, 'p'};
	    longoptions[nopt++] = lopt;
	  }
	  break;
	}
      case 'P':
	{
	  struct option lopt = {"dumpxrsl", 0, 0, 'P'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'q':
	{
	  struct option lopt = {"queues", 0, 0, 'q'};
	  longoptions[nopt++] = lopt;
	  break;
					}
      case 'r':
	{
	  struct option lopt = {"keep", 0, 0, 'r'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'R':
	{
	  struct option lopt = {"resources", 0, 0, 'R'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'T':
	{
	  struct option lopt = {"notransfer", 0, 0, 'T'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'u':
	{
	  struct option lopt = {"secure", 0, 0, 'u'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'U':
	{
	  struct option lopt = {"unknownattr", 0, 0, 'U'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'v':
	{
	  struct option lopt = {"version", 0, 0, 'v'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      case 'V':
	{
	  struct option lopt = {"vo", 0, 0, 'V'};
	  longoptions[nopt++] = lopt;
				    break;
	}
      case 'x':
	{
	  struct option lopt = {"anonymous", 0, 0, 'x'};
	  longoptions[nopt++] = lopt;
	  break;
					}
      case 'X':
	{
	  struct option lopt = {"gsi", 0, 0, 'X'};
	  longoptions[nopt++] = lopt;
	  break;
	}
      }
    }
  } //end loop over optstring
  
  struct option lopt = {0, 0, 0, 0};
  longoptions[nopt++] = lopt;
  
  // common options
  std::list<std::string> clusterreject;
  std::list<std::string> clusterselect;
  std::list<std::string> giisurls;
  bool all = false;
  std::list<std::string> status;
  int timeout = 20;
  bool anonymous = true;
  bool debugset = false;
  // ngcat options
  int whichfile = 1;
  bool follow = false;
  // ngclean & ngsync options
  bool force = false;
  // ngget, ngkill & ngresub options
  bool keep = false;
  // ngget options
  std::string directory;
  bool usejobname = false;
  // ngstat options
  bool clusters = false;
  bool longlist = false;
  // ngsub options
  std::list<std::string> xrslstrings;
  // ngresub options
  std::list<std::string> klusterreject;
  std::list<std::string> klusterselect;
  // ngsub & ngresub options
  std::string joblistfile;
  bool dryrun = false;
  bool dumpxrsl = false;
  bool unknownattr = false;
  // ngresume options
  bool renewproxy = false;
  // ngtest options
  unsigned int runtime = 5;
  unsigned int testjob = 0;
  bool voinfo = false;
  bool resourceinfo = false;
  bool certificateinfo = false;
  bool configuration = false;
  // ngstage options
  std::string request_id;
  std::string endpoint = "";
  bool query = false;
  bool cancel = false;
  bool list_ids = false;
  // ngtransfer options
  std::list<std::string> sources;
  
  std::list<std::string> params;
  std::list<std::string> stringlist;
  
  int opt = 0;
  while (opt != -1) {
    opt = getopt_long_only(argc, argv, optstring, longoptions, NULL);
    if (opt == -1) continue;
    if (optarg) {
      switch (opt) {
      case 'c':
	if (progname == "stage") { // request id
	  request_id = optarg;
	  cancel = true;
	}
	else { // cluster
	  if (optarg[0] == '-')
	    clusterreject.push_back(&optarg[1]);
	  else if (optarg[0] == '+')
	    clusterselect.push_back(&optarg[1]);
	  else
	    clusterselect.push_back(optarg);
	}
	break;
      case 'C':  // clustlist
	if (optarg[0] == '-') {
	  /*
	  stringlist = ReadFile(&optarg[1]);
	  while (!stringlist.empty()) {
	    clusterreject.push_back(*stringlist.begin());
	    stringlist.pop_front();
	  }
	  */
	}
	else if (optarg[0] == '+') {
	  /*
	  stringlist = ReadFile(&optarg[1]);
	  while (!stringlist.empty()) {
	    clusterselect.push_back(*stringlist.begin());
	    stringlist.pop_front();
	  }
	  */
	}
	else {
	  /*
	  stringlist = ReadFile(optarg);
	  while (!stringlist.empty()) {
	    clusterselect.push_back(*stringlist.begin());
	    stringlist.pop_front();
	  }
	  */
	}
	break;
      case 'd':  // debug
	Arc::Logger::getRootLogger().setThreshold(Arc::LogLevel(Arc::stringtoi(optarg)));
	debugset = true;
	break;
      case 'D':  // dir
	directory = optarg;
	break;
      case 'e':  // xrsl
	xrslstrings.push_back(optarg);
	break;
      case 'f':  // file
	params.push_back(optarg);
	break;
      case 'g':  // giisurl
	giisurls.push_back(optarg);
	break;
      case 'G':  // giislist
	/*
	stringlist = ReadFile(optarg);
	while (!stringlist.empty()) {
	  giisurls.push_back(*stringlist.begin());
	  stringlist.pop_front();
	}
	*/
	break;
      case 'i':  // joblist (in)
	/*
	stringlist = ReadFile(optarg);
	while (!stringlist.empty()) {
	  params.push_back(*stringlist.begin());
	  stringlist.pop_front();
	}
	*/
	break;
      case 'J':
	testjob = atoi(optarg);
	break;
      case 'k':  // kluster
	if (optarg[0] == '-')
	  klusterreject.push_back(&optarg[1]);
	else if (optarg[0] == '+')
	  klusterselect.push_back(&optarg[1]);
	else
	  klusterselect.push_back(optarg);
	break;
      case 'K':  // klustlist
	if (optarg[0] == '-') {
	  /*
	  stringlist = ReadFile(&optarg[1]);
	  while (!stringlist.empty()) {
	    klusterreject.push_back(*stringlist.begin());
	    stringlist.pop_front();
	  }
	  */
	}
	else if (optarg[0] == '+') {
	  /*
	  stringlist = ReadFile(&optarg[1]);
	  while (!stringlist.empty()) {
	    klusterselect.push_back(*stringlist.begin());
	    stringlist.pop_front();
	  }
	  */
	}
	else {
	  /*
	  stringlist = ReadFile(optarg);
	  while (!stringlist.empty()) {
	    klusterselect.push_back(*stringlist.begin());
	    stringlist.pop_front();
	  }
	  */
	}
	break;
      case 'o':  // joblist (out)
	joblistfile = optarg;
	break;
      case 'q':  // requset_id
	request_id = optarg;
	query = true;
	break;
      case 'r':  // runtime of testjob 1, recursion level
	if (progname == "test")
	  runtime = atoi(optarg);
	else //ngcp option
	  //recursion = atoi(optarg);
	break;
      case 'R':  // number of retries for file transfers
	//retries = atoi(optarg);
	break;
      case 's':  // status, data sources
	if (progname == "transfer")
	  sources.push_back(optarg);
	else if (progname == "stage")
	  endpoint = optarg;
	else
	  status.push_back(optarg);
					break;
      case 't':  // timeout
	timeout = atoi(optarg);
	break;
      case 'y':  // cache
	//cache_path = optarg;
	break;
      case 'Y':  // cachedata
	//cache_data_path = optarg;
	break;
      default:
	logger.msg(Arc::ERROR, "Use -help option for help");
	return 1;
	break;
      }
    }
    else {
      switch (opt) {
      case 'a':  // all
	all = true;
	break;
      case 'D':  // dryrun
	dryrun = true;
	break;
      case 'E': // print certificate info
	certificateinfo = true;
	break;
      case 'e':  // stderr
	whichfile = 2;
	break;
      case 'f':  // force
	if (progname == "cat")
	  follow = true;
	else
	  force = true;
	break;
      case 'h':  // help
	arcusage(progname, optstring);
	return 0;
      case 'i':  // indicate
	//verbose = true;
	break;
      case 'j':  // usejobname
	usejobname = true;
	break;
      case 'l':  // gmlog or long
	if (progname == "cat")
	  whichfile = 3;
	else if (progname == "stage")
	  list_ids = true;
	else
	  longlist = true;
	break;
      case 'L':  // locations
	//locations = true;
	break;
      case 'n':  // nopassive 
	//notpassive = true;
	break;
      case 'o':  // stdout
	whichfile = 1;
	break;
      case 'O':  // user-configuration
	configuration = true;
	break;
      case 'p':  // renewproxy or passive
	if (progname == "cp")
	  bool stupid;
	  //passive = true;
	else
	  renewproxy = true;
	break;
      case 'P':  // dumpxrsl
	dumpxrsl = true;
	break;
      case 'q':  // queues
	clusters = true;
	break;
      case 'r':  // keep
	keep = true;
	break;
      case 'R': // resource-info
	resourceinfo = true;
	break;
      case 'T':  // notransfer
	//nocopy = true;
	break;
      case 'u':  // secure
	//secure = true;
	break;
      case 'U':  // unknownattr
	unknownattr = true;
	break;
      case 'v':  // version
	std::cout << progname << ": " << _("version") << " "
		  << VERSION << std::endl;
	return 0;
      case 'V': // voinfo
	voinfo = true;
	break;
      case 'x':  // anonymous
	anonymous = true;
	break;
      case 'X':  // gsi
	anonymous = false;
	break;
      default:
	logger.msg(Arc::ERROR, "Use -help option for help");
	break;
      }
    }
  }
  
  while (argc > optind) params.push_back(argv[optind++]);


  /*
  if (!debugset) {
    std::string debuglevel = GetOption("debug", "NGDEBUG");
    if (!debuglevel.empty())
      SetNotifyLevel((NotifyLevel) stringtoi(debuglevel));
  }
  */  
  if (progname == "sub" && params.empty() && xrslstrings.empty()) {
    std::string xrsl;
    std::string line;
    while (getline(std::cin, line) && line != ".") xrsl += line;
    xrslstrings.push_back(xrsl);
  }
  /*  
  ResolveAliases(clusterselect);
  ResolveAliases(clusterreject);
  ResolveAliases(klusterselect);
  ResolveAliases(klusterreject);
  */
  
  if (progname == "get") {
    if (directory.empty())
      //      directory = GetOption("downloaddir", "NGDOWNLOAD");
    if (directory.empty()) {
      char buffer[PATH_MAX];
      getcwd (buffer, PATH_MAX);
      directory = buffer;
    }
  }
  
  if (progname != "sub" && 
      progname != "sync" && 
      progname != "test" &&
      progname != "acl" &&
      progname != "ls" &&
      progname != "cp" &&
      progname != "transfer" &&
      progname != "stage" &&
      progname != "rm") {
    if (clusters) {
      if (all || !params.empty() || !status.empty())
	logger.msg(Arc::ERROR, "Incompatible Options");
    }
    else {
      if (all && !params.empty() && !clusterselect.empty() &&
	  !clusterreject.empty() && !status.empty())
	logger.msg(Arc::ERROR, "Incompatible Options");
      
      if (!all && params.empty() && clusterselect.empty() &&
	  clusterreject.empty() && status.empty())
	logger.msg(Arc::ERROR, "No valid jobnames/jobids given");
    }
  }
  
  if (progname == "stage") {
    if (!(query || list_ids || cancel) && params.empty())
      logger.msg(Arc::ERROR, "Wrong number of parameters specified. Use -h for help");
    // can only use one of -q, -c and -l
    if ( (query || list_ids || cancel) && (!((query ^ list_ids) ^ cancel)) )
      logger.msg(Arc::ERROR, "Must give only one of -q, -c or -l options");
    
    // must give service endpoint with -q, -c or -l
    if ( (query || list_ids || cancel) && endpoint == "")
      logger.msg(Arc::ERROR, "Must give service endpoint when using -q, -c or -l options");
  };
  
  /*
  if (progname == "test") {
    unsigned int giventestopts = 0;
    if (testjob>0) giventestopts++;
    if (certificateinfo) giventestopts++;
    if (resourceinfo) giventestopts++;
    if (voinfo) giventestopts++;
    if (configuration) giventestopts++;
    if (giventestopts!=1)
      logger.msg(Arc::ERROR, "Invalid options to given arctest");
    
    if (runtime<=0)
      logger.msg(Arc::ERROR, "Illegal runtime for testjob 1 specified");
    
    arctest(testjob, certificateinfo, voinfo, resourceinfo,
	    configuration, runtime,
	    clusterselect, clusterreject, giisurls,
	    joblistfile, dryrun, dumpxrsl, timeout, anonymous);
    return 0;
  }
  */

  /*
  Certificate proxy(PROXY);
  
  notify(INFO) << _("Proxy subject name") << ": "
	       << proxy.GetSN() << std::endl;
  notify(INFO) << _("Proxy valid to") << ": "
		             << proxy.ExpiryTime() << std::endl;
  
  if (proxy.IsExpired())
    throw ARCLibError(_("The proxy has expired"));
  
  notify(INFO) << _("Proxy valid for") << ": "
	       << proxy.ValidFor() << std::endl;
  */

#ifdef ARCSTAT  
  arcstat(params, clusterselect, clusterreject, status,
	  giisurls, clusters, longlist, timeout, anonymous);
#endif

#ifdef ARCSUB
  arcsub(params, xrslstrings, clusterselect, clusterreject, giisurls,
	 joblistfile, dryrun, dumpxrsl, unknownattr, timeout, anonymous);
#endif

  return 0;

} //end main
