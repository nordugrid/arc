#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <arc/data/DMC.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataCache.h>
#include <arc/data/URLMap.h>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);
    Arc::OptionParser options(istring("service_url jsdl_file info_file"));

    std::string debug;
    options.AddOption('d', "debug",
                      istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                      istring("debuglevel"), debug);

    std::list<std::string> params = options.Parse(argc, argv);

    if (!debug.empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

    std::vector< std::pair<std::string, std::string> > fileList;
    std::list<std::string>::iterator it = params.begin();
    std::string& src (*it++); std::string& dst (*it++);

    fileList.push_back(std::make_pair(src, dst));
    
    // Create mover
    Arc::DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(false);
    mover.verbose(true);
    //mover->set_default_max_inactivity_time(300);
    
    // Create cache
    Arc::User cache_user;
    std::string cache_path2;
    std::string cache_data_path;
    //Arc::DataCache cache(cache_path, cache_data_path, "", job.jobId, cache_user);
    std::string id = "<ngcp>";
    std::string job_root = "/tmp/cream_client_test/";
    Arc::DataCache cache(cache_path2, cache_data_path, "", id, cache_user);
    for (std::vector< std::pair< std::string, std::string > >::const_iterator file = fileList.begin(); file != fileList.end(); file++) {
        std::string src = job_root + "/" + (*file).first;
        //std::string dst = Glib::build_filename(job.ISB_URI, (*file).second);
        std::string dst = (*file).second;
        std::cout << src << " -> " << dst << std::endl;
        //Arc::DataPoint *source(Arc::DMC::GetDataPoint(src));
        //Arc::DataPoint *destination(Arc::DMC::GetDataPoint(dst));
        Arc::DataHandle source(src);
        Arc::DataHandle destination(dst);
        
        //source->SetTries(5);
        //destination->SetTries(5);

        std::string failure;
        int timeout = 300;
        if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(), 0, 0, 0, timeout, failure)) {
            if (!failure.empty()) std::cerr << "File moving was not succeeded: " << failure << std::endl;
            else std::cerr << "File moving was not succeeded." << std::endl;
        }
        
        //if (source) delete source;
        //if (destination) delete destination;
    }
    
    //if (mover) delete mover;
    //if (cache) delete cache;
} // main()
