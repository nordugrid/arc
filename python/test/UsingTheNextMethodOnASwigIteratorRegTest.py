'''
This regression test tests whether invoking the next method on a Swig
iterator obtained from a std::list of ARC C++ objects generates a
segmentation fault. That issue was reported in bug 2683.
'''

import arc

jobs = arc.JobList()

jobs.push_back(arc.Job())

itJobs = jobs.__iter__()

next(itJobs)
