import arcom.test, arc, unittest

class JobSupervisorTest(arcom.test.ARCClientTestCase):
    
    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
    
    def test_constructor(self):
        id1 = arc.URL("http://test.nordugrid.org/1234567890test1")
        id2 = arc.URL("http://test.nordugrid.org/1234567890test2")
        js = arc.JobSupervisor(self.usercfg, [
            self.create_test_job(job_id = id1),
            self.create_test_job(job_id = id2)
        ]);
        self.expect(js.GetAllJobs()).not_to_be_empty()
        
        self.expect(js.GetJobControllers()).to_have(1).item()

        jobs = js.GetJobs()
        self.expect(jobs).to_have(2).jobs()
        
        self.expect(jobs[0].IDFromEndpoint.str()).to_be(id1.str())
        self.expect(jobs[1].IDFromEndpoint.str()).to_be(id2.str())
    
    def test_add_job(self):
        js = arc.JobSupervisor(self.usercfg, arc.JobList())
        self.expect(js.GetAllJobs()).to_be_empty()
        
        job = self.create_test_job(job_id = "http://test.nordugrid.org/1234567890test1")
        self.expect(js.AddJob(job)).to_be(True, message = "AddJob was expected to return True")
        self.expect(js.GetAllJobs()).not_to_be_empty()

        job.Flavour = ""
        self.expect(js.AddJob(job)).to_be(False, message = "AddJob was expected to return False")
        self.expect(js.GetAllJobs()).to_have(1).job()

        job.Flavour = "NON-EXISTENT"
        self.expect(js.AddJob(job)).to_be(False, message = "AddJob was expected to return False")
        self.expect(js.GetAllJobs()).to_have(1).job()        
    
    def test_resubmit(self):
        # self.turn_on_logging()
        
        self.usercfg.AddServices(["TEST:http://test2.nordugrid.org"], arc.COMPUTING)
        self.usercfg.Broker("TEST")

        arc.TargetRetrieverTestACCControl.foundTargets = [self.create_test_target("http://test2.nordugrid.org")]

        js = arc.JobSupervisor(self.usercfg, [
            self.create_test_job(job_id = "http://test.nordugrid.org/1234567890test1", state = arc.JobState.FAILED),
            self.create_test_job(job_id = "http://test.nordugrid.org/1234567890test2", state = arc.JobState.RUNNING)
        ])
        
        self.expect(js.GetAllJobs()).to_have(2).jobs()

        resubmitted = arc.JobList()
        result = js.Resubmit(0, resubmitted)
        self.expect(result).to_be(True)
        self.expect(resubmitted).to_have(2).jobs()
        
    def test_cancel(self):
        id1 = arc.URL("http://test.nordugrid.org/1234567890test1")
        id2 = arc.URL("http://test.nordugrid.org/1234567890test2")
        id3 = arc.URL("http://test.nordugrid.org/1234567890test3")
        id4 = arc.URL("http://test.nordugrid.org/1234567890test4")        
        js = arc.JobSupervisor(self.usercfg, [
            self.create_test_job(job_id = id1, state = arc.JobState.RUNNING),
            self.create_test_job(job_id = id2, state = arc.JobState.FINISHED),
            self.create_test_job(job_id = id3, state = arc.JobState.UNDEFINED)
        ])
        
        arc.JobControllerTestACCControl.cancelStatus = True
        self.expect(js.Cancel()).to_be(True, message = "Cancel was expected to return True")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0].str()).to_be(id1.str())
        self.expect(js.GetIDsNotProcessed()).to_have(2).IDs()
        self.expect(js.GetIDsNotProcessed()[0].str()).to_be(id2.str())
        self.expect(js.GetIDsNotProcessed()[1].str()).to_be(id3.str())
        js.ClearSelection()

        arc.JobControllerTestACCControl.cancelStatus = False
        self.expect(js.Cancel()).to_be(False, message = "Cancel was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(3).IDs()
        self.expect(js.GetIDsNotProcessed()[0].str()).to_be(id1.str())
        self.expect(js.GetIDsNotProcessed()[1].str()).to_be(id2.str())
        self.expect(js.GetIDsNotProcessed()[2].str()).to_be(id3.str())
        js.ClearSelection()
        
        job = self.create_test_job(job_id = id4, state = arc.JobState.ACCEPTED, state_text = "Accepted")
        self.expect(js.AddJob(job)).to_be(True, message = "AddJob was expected to return True")

        arc.JobControllerTestACCControl.cancelStatus = True
        js.SelectByStatus(["Accepted"])
        self.expect(js.Cancel()).to_be(True, message = "Cancel was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0].str()).to_be(id4.str())
        self.expect(js.GetIDsNotProcessed()).to_have(0).IDs()
        js.ClearSelection()

        arc.JobControllerTestACCControl.cancelStatus = False
        js.SelectByStatus(["Accepted"])
        self.expect(js.Cancel()).to_be(False, message = "Cancel was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(1).ID()
        self.expect(js.GetIDsNotProcessed()[0].str()).to_be(id4.str())
        js.ClearSelection()
        
    def test_clean(self):
        id1 = arc.URL("http://test.nordugrid.org/1234567890test1")
        id2 = arc.URL("http://test.nordugrid.org/1234567890test2")
        js = arc.JobSupervisor(self.usercfg, [
          self.create_test_job(job_id = id1, state = arc.JobState.FINISHED, state_text = "Finished"),
          self.create_test_job(job_id = id2, state = arc.JobState.UNDEFINED)
        ])
        self.expect(js.GetAllJobs()).to_have(2).jobs()

        arc.JobControllerTestACCControl.cleanStatus = True
        self.expect(js.Clean()).to_be(True, message = "Clean was expected to return True")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0].str()).to_be(id1.str())
        self.expect(js.GetIDsNotProcessed()).to_have(1).ID()
        self.expect(js.GetIDsNotProcessed()[0].str()).to_be(id2.str())
        js.ClearSelection()

        arc.JobControllerTestACCControl.cleanStatus = False
        self.expect(js.Clean()).to_be(False, message = "Clean was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(2).IDs()
        self.expect(js.GetIDsNotProcessed()[0].str()).to_be(id1.str())
        self.expect(js.GetIDsNotProcessed()[1].str()).to_be(id2.str())
        js.ClearSelection()

        arc.JobControllerTestACCControl.cleanStatus = True
        js.SelectByStatus(["Finished"])
        self.expect(js.Clean()).to_be(True, message = "Clean was expected to return True")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0].str()).to_be(id1.str())
        self.expect(js.GetIDsNotProcessed()).to_have(0).IDs()
        js.ClearSelection()

        arc.JobControllerTestACCControl.cleanStatus = False
        js.SelectByStatus(["Finished"])
        self.expect(js.Clean()).to_be(False, message = "Clean was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(1).ID()
        self.expect(js.GetIDsNotProcessed()[0].str()).to_be(id1.str())
        js.ClearSelection()        

if __name__ == '__main__':
    unittest.main()