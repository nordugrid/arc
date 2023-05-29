import testutils, arc, unittest

class JobSupervisorTest(testutils.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))

    def test_constructor(self):
        id1 = "http://test.nordugrid.org/1234567890test1"
        id2 = "http://test.nordugrid.org/1234567890test2"
        js = arc.JobSupervisor(self.usercfg, [
            self.create_test_job(job_id = id1),
            self.create_test_job(job_id = id2)
        ]);
        self.expect(js.GetAllJobs()).not_to_be_empty()

        jobs = js.GetAllJobs()
        self.expect(jobs).to_have(2).jobs()

        self.expect(jobs[0].JobID).to_be(id1)
        self.expect(jobs[1].JobID).to_be(id2)

    def test_add_job(self):
        js = arc.JobSupervisor(self.usercfg, arc.JobList())
        self.expect(js.GetAllJobs()).to_be_empty()

        job = self.create_test_job(job_id = "http://test.nordugrid.org/1234567890test1")
        self.expect(js.AddJob(job)).to_be(True, message = "AddJob was expected to return True")
        self.expect(js.GetAllJobs()).not_to_be_empty()

        job.JobManagementInterfaceName = ""
        self.expect(js.AddJob(job)).to_be(False, message = "AddJob was expected to return False")
        self.expect(js.GetAllJobs()).to_have(1).job()

        job.JobManagementInterfaceName = "non.existent.interface"
        self.expect(js.AddJob(job)).to_be(False, message = "AddJob was expected to return False")
        self.expect(js.GetAllJobs()).to_have(1).job()

    def test_cancel(self):
        id1 = "http://test.nordugrid.org/1234567890test1"
        id2 = "http://test.nordugrid.org/1234567890test2"
        id3 = "http://test.nordugrid.org/1234567890test3"
        id4 = "http://test.nordugrid.org/1234567890test4"
        js = arc.JobSupervisor(self.usercfg, [
            self.create_test_job(job_id = id1, state = arc.JobState.RUNNING),
            self.create_test_job(job_id = id2, state = arc.JobState.FINISHED),
            self.create_test_job(job_id = id3, state = arc.JobState.UNDEFINED)
        ])

        arc.JobControllerPluginTestACCControl.cancelStatus = True
        self.expect(js.Cancel()).to_be(True, message = "Cancel was expected to return True")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0]).to_be(id1)
        self.expect(js.GetIDsNotProcessed()).to_have(2).IDs()
        self.expect(js.GetIDsNotProcessed()[0]).to_be(id2)
        self.expect(js.GetIDsNotProcessed()[1]).to_be(id3)
        js.ClearSelection()

        arc.JobControllerPluginTestACCControl.cancelStatus = False
        self.expect(js.Cancel()).to_be(False, message = "Cancel was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(3).IDs()
        self.expect(js.GetIDsNotProcessed()[0]).to_be(id1)
        self.expect(js.GetIDsNotProcessed()[1]).to_be(id2)
        self.expect(js.GetIDsNotProcessed()[2]).to_be(id3)
        js.ClearSelection()

        job = self.create_test_job(job_id = id4, state = arc.JobState.ACCEPTED, state_text = "Accepted")
        self.expect(js.AddJob(job)).to_be(True, message = "AddJob was expected to return True")

        arc.JobControllerPluginTestACCControl.cancelStatus = True
        js.SelectByStatus(["Accepted"])
        self.expect(js.Cancel()).to_be(True, message = "Cancel was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0]).to_be(id4)
        self.expect(js.GetIDsNotProcessed()).to_have(0).IDs()
        js.ClearSelection()

        arc.JobControllerPluginTestACCControl.cancelStatus = False
        js.SelectByStatus(["Accepted"])
        self.expect(js.Cancel()).to_be(False, message = "Cancel was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(1).ID()
        self.expect(js.GetIDsNotProcessed()[0]).to_be(id4)
        js.ClearSelection()

    def test_clean(self):
        id1 = "http://test.nordugrid.org/1234567890test1"
        id2 = "http://test.nordugrid.org/1234567890test2"
        js = arc.JobSupervisor(self.usercfg, [
          self.create_test_job(job_id = id1, state = arc.JobState.FINISHED, state_text = "Finished"),
          self.create_test_job(job_id = id2, state = arc.JobState.UNDEFINED)
        ])
        self.expect(js.GetAllJobs()).to_have(2).jobs()

        arc.JobControllerPluginTestACCControl.cleanStatus = True
        self.expect(js.Clean()).to_be(True, message = "Clean was expected to return True")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0]).to_be(id1)
        self.expect(js.GetIDsNotProcessed()).to_have(1).ID()
        self.expect(js.GetIDsNotProcessed()[0]).to_be(id2)
        js.ClearSelection()

        arc.JobControllerPluginTestACCControl.cleanStatus = False
        self.expect(js.Clean()).to_be(False, message = "Clean was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(2).IDs()
        self.expect(js.GetIDsNotProcessed()[0]).to_be(id1)
        self.expect(js.GetIDsNotProcessed()[1]).to_be(id2)
        js.ClearSelection()

        arc.JobControllerPluginTestACCControl.cleanStatus = True
        js.SelectByStatus(["Finished"])
        self.expect(js.Clean()).to_be(True, message = "Clean was expected to return True")
        self.expect(js.GetIDsProcessed()).to_have(1).ID()
        self.expect(js.GetIDsProcessed()[0]).to_be(id1)
        self.expect(js.GetIDsNotProcessed()).to_have(0).IDs()
        js.ClearSelection()

        arc.JobControllerPluginTestACCControl.cleanStatus = False
        js.SelectByStatus(["Finished"])
        self.expect(js.Clean()).to_be(False, message = "Clean was expected to return False")
        self.expect(js.GetIDsProcessed()).to_have(0).IDs()
        self.expect(js.GetIDsNotProcessed()).to_have(1).ID()
        self.expect(js.GetIDsNotProcessed()[0]).to_be(id1)
        js.ClearSelection()

if __name__ == '__main__':
    unittest.main()
