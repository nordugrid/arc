import unittest, arc

class ExpectationalTestCase(unittest.TestCase):

    class Expectation(object):
        def __init__(self, actual, testcase):
            self.actual = actual
            self.testcase = testcase
            self.number = None
                        
        def to_be(self, expected, message = None):
            if message is None:
                message = "%s was expected to be %s" % (self.actual, expected)
            self.testcase.assertEqual(self.actual, expected, message)
        
        def to_be_empty(self):
            self.testcase.assertEqual(len(self.actual), 0, "%s was expected to be empty" % (self.actual,))

        def not_to_be_empty(self):
            self.testcase.assertNotEqual(len(self.actual), 0, "%s was expected to be empty" % (self.actual,))

        def to_have(self, number):
            self.number = number
            return self
        
        def _test_having(self):
            self.testcase.assertEqual(len(self.actual), self.number,
                "%s was expected to have %s %s, but it only has %s" %
                    (self.actual,
                    self.number,
                    self.item_name,
                    len(self.actual)))

        def __getattr__(self, name):
            if self.number is not None:
                self.item_name = name
                return self._test_having
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
                        state_text = None,
                        job_description = "non-empty"):
        job = arc.Job()
        job.Flavour = "TEST"
        job.Cluster = arc.URL(cluster)
        job.InfoEndpoint = arc.URL(info_endpoint)
        if state_text is None:
            job.State = arc.JobStateTEST(state)
        else:
            job.State = arc.JobStateTEST(state, state_text)
        if isinstance(job_id, str):
            job_id = arc.URL(job_id)
        job.IDFromEndpoint = job_id
        job.JobDescriptionDocument = job_description
        return job
