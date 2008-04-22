#include <getopt.h>

#include <arc/ArcLocation.h>

#define _(A) (A)

static void arcusage(const std::string& progname, const char *optstring) {

	std::cout << _("Usage") << ": arc" << progname
	          << " [" << _("options") << "]";

	if (progname == "cp")
		std::cout << " " << _("source") << " " << _("destination");
	else if (progname == "ls")
		std::cout << " " << _("URL");
	else if (progname == "rm")
		std::cout << " " << _("URL");

	std::cout << std::endl << std::endl;
	std::cout << _("Options") << ":" << std::endl;
	for (int i = 0; optstring[i]; i++) {
		if (optstring[i] == ':') continue;
		if (optstring[i + 1] == ':') {
			switch (optstring[i]) {
			case 'd':
				std::cout << _(
"  -d, -debug     debuglevel    from -3 (quiet) to 3 (verbose) - default 0"
				                       ) << std::endl;
				break;
			case 'r':
				std::cout << _(
"  -r, -recursive level         operate recursively (if possible) up to\n"
"                                 specified level (0 - no recursion)"
				                       ) << std::endl;
				break;
			case 'R':
				std::cout << _(
"  -R, -retries   number        how many times to retry transfer of\n"
"                                 every file before failing"
				                       ) << std::endl;
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
			case 'f':
				if (progname == "cp") {
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
			case 'l':
				std::cout << _(
"  -l, -long                    long format (more information)"
				                       ) << std::endl;
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
			case 'p':
				std::cout << _(
"  -p, -passive                 use passive transfer (does not work if\n"
"                                 secure is on, default if secure is not\n"
"                                 requested)"
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
			case 'v':
				std::cout << _(
"  -v, -version                 print version information"
				                       ) << std::endl;
				break;
			}
		}
	}
}


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
		progname = progname.substr(progname.length() - 2);

		char * optstring;

		if (progname == "ls")
			optstring = "lLr:d:vh";
		else if (progname == "cp")
			optstring = "pnfiTuy:Y:r:R:t:d:vh";
		else if (progname == "rm")
			optstring = "fd:vh";
		else {
			logger.msg(Arc::ERROR, "Unknown program name: %s",
			           progname);
			return 1;
		}

		struct option longoptions[MAX_EXPECTED_OPTIONS+1];
		int nopt = 0;
		for (int i = 0; optstring[i]; i++) {
			if (optstring[i] == ':') continue;
			if (nopt >= MAX_EXPECTED_OPTIONS) break;
			if (optstring[i + 1] == ':') {
				switch (optstring[i]) {
				case 'd':
					{
						struct option lopt = {"debug", 1, 0, 'd'};
						longoptions[nopt++] = lopt;
						break;
					}
				case 'r':
					{
						struct option lopt = {"recursive", 1, 0, 'r'};
						longoptions[nopt++] = lopt;
						break;
					}
				case 'R':
					{
						struct option lopt = {"retries", 1, 0, 'R'};
						longoptions[nopt++] = lopt;
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
			}
			else {
				switch (optstring[i]) {
				case 'f':
					{
						struct option lopt = {"force", 0, 0, 'f'};
						longoptions[nopt++] = lopt;
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
				case 'l':
					{
						struct option lopt = {"long", 0, 0, 'l'};
						longoptions[nopt++] = lopt;
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
				case 'p':
					{
						struct option lopt = {"passive", 0, 0, 'p'};
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
				case 'v':
					{
						struct option lopt = {"version", 0, 0, 'v'};
						longoptions[nopt++] = lopt;
						break;
					}
				}
			}
		}

		struct option lopt = {0, 0, 0, 0};
		longoptions[nopt++] = lopt;

		int timeout = 20;
		bool force = false;
		bool longlist = false;
		bool locations = false;
		bool secure = false;
		bool passive = false;
		bool notpassive = false;
		bool nocopy = false;
		bool verbose = false;
		std::string cache_path;
		std::string cache_data_path;
		int recursion = 0;
		int retries = 0;

		std::list<std::string> params;

		int opt = 0;
		while (opt != -1) {
			opt = getopt_long_only(argc, argv, optstring, longoptions, NULL);
			if (opt == -1) continue;
			if (optarg) {
				switch (opt) {
				case 'd':  // debug
					Arc::Logger::getRootLogger().setThreshold(Arc::LogLevel(Arc::stringtoi(optarg)));
					break;
				case 'r':  // recursion level
					recursion = Arc::stringtoi(optarg);
					break;
				case 'R':  // number of retries for file transfers
					retries = Arc::stringtoi(optarg);
					break;
				case 't':  // timeout
					timeout = Arc::stringtoi(optarg);
					break;
				case 'y':  // cache
					cache_path = optarg;
					break;
				case 'Y':  // cachedata
					cache_data_path = optarg;
					break;
				default:
					logger.msg(Arc::ERROR, "Use -help option for help");
					return 1;
					break;
				}
			}
			else {
				switch (opt) {
				case 'f':  // force
					force = true;
					break;
				case 'h':  // help
					arcusage(progname, optstring);
					return 0;
				case 'i':  // indicate
					verbose = true;
					break;
				case 'l':  // long
					longlist = true;
					break;
				case 'L':  // locations
					locations = true;
					break;
				case 'n':  // nopassive 
					notpassive = true;
					break;
				case 'p':  // passive
					passive = true;
					break;
				case 'T':  // notransfer
					nocopy = true;
					break;
				case 'u':  // secure
					secure = true;
					break;
				case 'v':  // version
					std::cout << "arc" << progname << ": " << _("version") << " "
					          << VERSION << std::endl;
					return 0;
				default:
					logger.msg(Arc::ERROR, "Use -help option for help");
					return 1;
					break;
				}
			}
		}

		while (argc > optind) params.push_back(argv[optind++]);

#ifdef ARCLS
			if(params.size() != 1) {
				logger.msg(Arc::ERROR, "Wrong number of parameters specified");
				return 1;
			}
			std::list<std::string>::iterator it = params.begin();
			arcls(*it, longlist, locations, recursion, timeout);
#endif
#ifdef ARCCP
			if(params.size() != 2) {
				logger.msg(Arc::ERROR, "Wrong number of parameters specified");
				return 1;
			}
			if(passive && notpassive) {
				logger.msg(Arc::ERROR, "Options 'p' and 'n' can't be used simultaneously");
				return 1;
			}
			std::list<std::string>::iterator it = params.begin();
			std::string source = *it; ++it;
			std::string destination = *it;
			if((!secure) && (!notpassive)) passive=true;
			if(nocopy) {
				arcregister(source, destination, secure, passive, force,
				            timeout);
			}
			else {
				arccp(source, destination, secure, passive, force,
				      recursion, retries+1, verbose, timeout);
			}
#endif
#ifdef ARCRM
			if(params.size() != 1) {
				logger.msg(Arc::ERROR, "Wrong number of parameters specified");
				return 1;
			}
			std::list<std::string>::iterator it = params.begin();
			arcrm(*it, force, timeout);
#endif

	return 0;
}
