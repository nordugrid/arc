#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "job_queue.h"

int main(int /* argc */, char **argv)
{   
    Arc::JobQueue jobq;
    jobq.init(argv[1], "jobq");
    for (Arc::JobQueueIterator it = jobq.getAll(); it.hasMore(); it++) {
        Arc::Job *j = *it;
        std::cout << "-------------------" << std::endl;
        std::cout << (std::string)*j << std::endl;
    }
    return 0;
}
