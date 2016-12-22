"""
The ``arc.lrms`` package contains batch system specific modules and a sub-package ``arc.lrms.common``. Each module has the following functions for cancel, scan and submit jobs:

* Cancel(path to arc.conf, local job ID)
* Scan(path to arc.conf, list of control directories ...)
* Submit(path to arc.conf, instance of :py:class:`arc.JobDescription`)
"""
