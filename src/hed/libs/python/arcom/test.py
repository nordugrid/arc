import unittest, arc

class ExpectationalTestCase(unittest.TestCase):

    class Expectation(object):
        def __init__(self, actual, testcase):
            self.actual = actual
            self.testcase = testcase
            self.number = None
        
        def toBe(self, expected):
            self.testcase.assertEqual(self.actual, expected, "%s was expected to be %s" % (self.actual, expected))

        def toHave(self, number):
            self.number = number
            return self
        
        def pretty_actual(self):
            s = str(self.actual)
            if "Swig" in s:
                try:
                    return s[s.index("<")+1:s.index(";")]
                except:
                    pass
            return s

        def __getattr__(self, name):
            if self.number:
                self.testcase.assertEqual(len(self.actual), self.number,
                    "%s was expected to have %s %s, but it only has %s" %
                        (self.pretty_actual(),
                        self.number,
                        name,
                        len(self.actual)))
            else:
                raise AttributeError
            
    def expect(self, actual):
        return self.Expectation(actual, self)


class ARCClientTestCase(ExpectationalTestCase):
    
    def turn_on_logging(self):
        import sys
        arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stdout))
        arc.Logger.getRootLogger().setThreshold(arc.DEBUG)
    
    def create_test_target(self, url = "http://test.nordugrid.org"):
        target = arc.ExecutionTarget()
        target.url = arc.URL(url)
        target.GridFlavour = "TEST"
        target.HealthState = "ok"
        return target

    def create_test_job(self,
                        job_id = "http://test.nordugrid.org/testid",
                        cluster = "http://test.nordugrid.org",
                        info_endpoint = "http://test.nordugrid.org",
                        state = arc.JobState.RUNNING,
                        job_description = "non-empty"):
        job = arc.Job()
        job.Flavour = "TEST"
        job.Cluster = arc.URL(cluster)
        job.InfoEndpoint = arc.URL(info_endpoint)
        job.State = arc.JobStateTEST(state)
        job.IDFromEndpoint = arc.URL(job_id)
        job.JobDescriptionDocument = job_description
        return job
