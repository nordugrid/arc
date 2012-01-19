import arcom.test, arc, unittest

class JobSupervisorTest(arcom.test.ARCClientTestCase):
    
    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
    
    def test_resubmit(self):
        # self.turn_on_logging()
        
        self.usercfg.AddServices(["TEST:http://test2.nordugrid.org"], arc.COMPUTING)
        self.usercfg.Broker("TEST")

        arc.TargetRetrieverTestACCControl.foundTargets = [self.create_test_target("http://test2.nordugrid.org")]

        js = arc.JobSupervisor(self.usercfg, [
            self.create_test_job(job_id = "http://test.nordugrid.org/1234567890test1", state = arc.JobState.FAILED),
            self.create_test_job(job_id = "http://test.nordugrid.org/1234567890test2", state = arc.JobState.RUNNING)
        ])
        
        self.expect(js.GetAllJobs()).toHave(2).jobs

        resubmitted = arc.JobList()
        result = js.Resubmit(0, resubmitted)
        self.expect(result).toBe(True)
        self.expect(resubmitted).toHave(2).jobs

if __name__ == '__main__':
    unittest.main()