#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "job_queue.h"

static void 
dump(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Invalid argument" << std::endl;
        return;
    }
    Arc::JobQueue jobq;
    jobq.init(argv[2], "jobq");
    for (Arc::JobQueueIterator it = jobq.getAll(); it.hasMore(); it++) {
        Arc::Job *j = *it;
        std::cout << "-------------------" << std::endl;
        std::cout << (std::string)*j << std::endl;
    }
}

static void 
remove (int argc, char **argv)
{
    if (argc != 4) {
        std::cerr << "Invalid argument" << std::endl;
        return;
    }
    Arc::JobQueue jobq;
    jobq.init(argv[2], "jobq");
    std::string job_id = argv[3];
    jobq.remove(job_id);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Invalid argument" << std::endl;
        return -1;
    }
    std::string cmd = argv[1];
    if (cmd == "dump") {
        dump(argc, argv);
    } else if (cmd == "remove") {
        remove(argc, argv);
    } else {
        std::cerr << "Unknown command: " << cmd << std::endl;
        return -1;
    }
    return 0;
}
