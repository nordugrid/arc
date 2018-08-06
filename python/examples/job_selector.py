#!/usr/bin/python2
'''
Create a JobSelector class in order to specify a custom selection to be used
with the JobSupervisor class.
'''

import arc, sys

# Extend the arc.compute.JobSelector class and the select method.
class ThreeDaysOldJobSelector(arc.compute.JobSelector):
    def __init__(self):
        super(ThreeDaysOldJobSelector, self).__init__()
        self.now = arc.common.Time()
        self.three_days = arc.common.Period(60*60*24*3)
        #self.three_days = arc.common.Period("P3D") # ISO duration
        #self.three_days = arc.common.Period(3*arc.common.Time.DAY)

    # The select method recieves a arc.compute.Job instance and must return a
    # boolean, indicating whether the job should be selected or rejected.
    # All attributes of the arc.compute.Job object can be used in this method.
    def Select(self, job):
        return (self.now - job.EndTime) > self.three_days


uc = arc.common.UserConfig()

arc.common.Logger_getRootLogger().addDestination(arc.common.LogStream(sys.stderr))
arc.common.Logger_getRootLogger().setThreshold(arc.common.VERBOSE)

j = arc.compute.Job()
j.JobManagementInterfaceName = "org.ogf.glue.emies.activitymanagement"
j.JobManagementURL = arc.common.URL("https://localhost")
j.JobStatusInterfaceName = "org.ogf.glue.emies.activitymanagement"
j.JobStatusURL = arc.common.URL("https://localhost")

js = arc.compute.JobSupervisor(uc)

j.JobID = "test-job-1-day-old"
j.EndTime = arc.common.Time()-arc.common.Period("P1D")
js.AddJob(j)

j.JobID = "test-job-2-days-old"
j.EndTime = arc.common.Time()-arc.common.Period("P2D")
js.AddJob(j)

j.JobID = "test-job-3-days-old"
j.EndTime = arc.common.Time()-arc.common.Period("P3D")
js.AddJob(j)

j.JobID = "test-job-4-days-old"
j.EndTime = arc.common.Time()-arc.common.Period("P4D")
js.AddJob(j)

selector = ThreeDaysOldJobSelector()
js.Select(selector)

for j in js.GetSelectedJobs():
    print (j.JobID)

# Make operation on selected jobs. E.g.:
#js.Clean()
