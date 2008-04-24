#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define NOGDI
#include <windows.h>
#include <tchar.h>

#include <fstream>
#include <signal.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "options.h"
#include <string>


Arc::Config config;
Arc::Loader *loader;
Arc::Logger& logger=Arc::Logger::rootLogger;
/* Create options parser */
#ifdef HAVE_GLIBMM_OPTIONS
Glib::OptionContext opt_ctx;
Arc::ServerOptions options;
#else
Arc::ServerOptions options;
Arc::ServerOptions& opt_ctx = options;
#endif

TCHAR *serviceName = TEXT("ArcHED");
SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
HANDLE stopServiceEvent = 0;

static void l(std::string str)
{
	std::ofstream out("ize", std::ios::app);
	out << str << std::endl;
	out.close();
}

static void merge_options_and_config(Arc::Config& cfg, Arc::ServerOptions& opt)
{   
    Arc::XMLNode srv = cfg["Server"];
    if (!(bool)srv) {
      logger.msg(Arc::ERROR, "No server config part of config file");
      return;
    }
    if (opt.pid_file != "") {
        if (!(bool)srv["PidFile"]) {
           srv.NewChild("Pidfile")=opt.pid_file;
        } else {
            srv["PidFile"] = opt.pid_file;
        }
    }
    if (opt.foreground == true) {
        if (!(bool)srv["Foreground"]) {
            srv.NewChild("Foreground");
        }
    }
}

static std::string init_logger(Arc::Config& cfg)
{   
    /* setup root logger */
    Arc::XMLNode log = cfg["Server"]["Logger"];
    std::string log_file = (std::string)log;
    std::string str = (std::string)log.Attribute("level");
    Arc::LogLevel level = Arc::string_to_level(str);
    Arc::Logger::rootLogger.setThreshold(level); 
    std::fstream *dest = new std::fstream(log_file.c_str(), std::fstream::out | std::fstream::app);
    if(!(*dest)) {
      std::cerr<<"Failed to open log file: "<<log_file<<std::endl;
      _exit(1);
    }
    Arc::LogStream* sd = new Arc::LogStream(*dest); 
    Arc::Logger::rootLogger.addDestination(*sd);
    if ((bool)cfg["Server"]["Foreground"]) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    return log_file;
}

static void WINAPI ServiceControlHandler(DWORD controlCode)
{
	switch (controlCode) {
		case SERVICE_CONTROL_INTERROGATE:
			break;
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(serviceStatusHandle, &serviceStatus);
			SetEvent(stopServiceEvent);
    			delete loader;
			return;
		case SERVICE_CONTROL_PAUSE:
			break;
		case SERVICE_CONTROL_CONTINUE:
			break;
		default:
			if (controlCode >= 128 && controlCode <= 255)
				// user defined control code
				break;
			else
				// unrecognised control code
				break;
	}
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

static void install_service(void)
{
	SC_HANDLE serviceControlManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

	if (serviceControlManager != NULL) {
		TCHAR path[_MAX_PATH + 1];
		if (GetModuleFileName(0, path, sizeof(path)/sizeof(path[0])) > 0) {
			std::cout << "path: " << path << std::endl;
			SC_HANDLE service = CreateService(serviceControlManager,
							  serviceName, 
							  serviceName,
							  SERVICE_ALL_ACCESS, 
							  SERVICE_WIN32_OWN_PROCESS,
							  SERVICE_AUTO_START, 
							  SERVICE_ERROR_NORMAL, 
							  path,
							  NULL, NULL, NULL, NULL, NULL);
			if (service == NULL) {
				std::cerr << "Cannot install service" << std::endl;
				CloseServiceHandle(serviceControlManager);
				return;
			}
			CloseServiceHandle(service);
		}
		CloseServiceHandle(serviceControlManager);
		return;
	}
	std::cerr << "Cannot conntact to Service Manager" << std::endl;
}

static void uninstall_service(void)
{
	SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);

	if (serviceControlManager) {
		SC_HANDLE service = OpenService(serviceControlManager,
				                serviceName, 
						SERVICE_QUERY_STATUS | DELETE );
		if (service) {
			SERVICE_STATUS serviceStatus;
			if (QueryServiceStatus(service, &serviceStatus)) {
				if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
					DeleteService(service);
				}
			}
			CloseServiceHandle(service);
		}
		CloseServiceHandle(serviceControlManager);
	}
}

static void report_event(TCHAR *function)
{
	HANDLE event_source;
	const char *strings[2];
	TCHAR Buffer[80];

	event_source = RegisterEventSource(NULL, serviceName);
	if (event_source != NULL) {
		snprintf(Buffer, 80, TEXT("%s failed with %d"), function, GetLastError());
		strings[0] = serviceName;
		strings[1] = Buffer;
		ReportEvent(event_source, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 2,
			    0, strings, NULL);
		DeregisterEventSource(event_source);
	
	}
}

static void init_config(void)
{
  	/* Load and parse config file */
      	config.parse(options.config_file.c_str());
       	if(!config) {
		logger.msg(Arc::ERROR, "Failed to load service configuration");
     		exit(1);
      	};
       	if(!MatchXMLName(config,"ArcConfig")) {
       		logger.msg(Arc::ERROR, "Configuration root element is not ArcConfig");
     		exit(1);
      	}

       	/* overwrite config variables by cmdline options */
       	merge_options_and_config(config, options);
       	/* initalize logger infrastucture */
       	std::string root_log_file = init_logger(config);
}
static void init(void)
{
	// bootstrap
	loader = new Arc::Loader(&config);
	// Arc::Loader loader(&config);
	logger.msg(Arc::INFO, "Service side MCCs are loaded");
}

static void WINAPI service_main(DWORD /*argc*/, TCHAR* /*argv*/[])
{
	// initialise service status
	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_STOPPED;
	serviceStatus.dwControlsAccepted = 0;
	serviceStatus.dwWin32ExitCode = NO_ERROR;
	serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	serviceStatusHandle = RegisterServiceCtrlHandler(serviceName, ServiceControlHandler);
	if (serviceStatusHandle) {
		// service is starting
		serviceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		// do initialisation here
		stopServiceEvent = CreateEvent(0, FALSE, FALSE, 0);
		// running
		serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		serviceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		
		do {
			Beep(1000, 1000);
		} while(WaitForSingleObject(stopServiceEvent, 5000) == WAIT_TIMEOUT);	
#if 0
#ifdef HAVE_GLIBMM_OPTIONS
    		opt_ctx.set_help_enabled();
    		opt_ctx.set_main_group(options);
#endif
		
		// bootstrap
		init_config();
		init();
		// wait for events
		while(1) {
			// sleep forever
			WaitForSingleObject(stopServiceEvent, INFINITE);
			// service was stopped
			serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(serviceStatusHandle, &serviceStatus);

			// do cleanup here
			CloseHandle(stopServiceEvent);
			stopServiceEvent = 0;

			// service is now stopped
			serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
			serviceStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(serviceStatusHandle, &serviceStatus);
		}
#endif

	}
}

static const char *get_path_from_exe(void)
{
	static char path[512];
	if (!GetModuleFileName(0, path, sizeof(path))) {
		std::cerr << "Cannot get module filename" << std::endl;
		return NULL;
	}
	
	std::string foo = path;
	foo.resize(foo.rfind("\\"));
	
	return foo.c_str();
}

static void run_service(void)
{
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ serviceName, service_main },
		{ 0, 0 }
	};
	
	const char *p = get_path_from_exe();
	if (SetCurrentDirectory(p) == 0) {
		std::cerr << "Set working directory failed: " << p << std::endl;
		return;
	}
	if(!StartServiceCtrlDispatcher(serviceTable)) {
		char buf[255];
		int err = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, LANG_NEUTRAL, buf, sizeof(buf), NULL);
		std::cerr << "Error initialize service: " << buf << std::endl;
	}
}

int main(int argc, char **argv)
{
#ifdef HAVE_GLIBMM_OPTIONS
    opt_ctx.set_help_enabled();
    opt_ctx.set_main_group(options);
#endif
    try {
        int status = opt_ctx.parse(argc, argv);
        if (status == 1) {
	    init_config();
	    if (options.install == true) {
		std::cout << "Install service ..." << std::endl;
		install_service();
		return 0;
	    }
	    if (options.uninstall == true) {
		std::cout << "Uninstall service ..." << std::endl;
		uninstall_service();
		return 0;
	    }
	    run_service();
#if 0
	    init();
	    while(1) {
		Sleep(1000);
	    }
#endif
	}
    } catch (const Glib::Error& error) {
      logger.msg(Arc::ERROR, error.what().c_str());
    }

    return 0;
}
