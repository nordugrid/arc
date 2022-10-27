import testutils, arc, unittest, time

class ComputingServiceRetrieverTest(testutils.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
        self.ce = arc.Endpoint()
        self.ce.URLString = "test.nordugrid.org"
        self.ce.InterfaceName = "org.nordugrid.tirtest"
        self.ce.Capability.append(arc.Endpoint.GetStringForCapability(arc.Endpoint.COMPUTINGINFO))
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0
        arc.TargetInformationRetrieverPluginTESTControl.targets = [self.create_test_target()]
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL)

    def test_the_class_exists(self):
        self.expect(arc.ComputingServiceRetriever).to_be_an_instance_of(type)

    def test_the_constructor(self):
        retriever = arc.ComputingServiceRetriever(self.usercfg)
        self.expect(retriever).to_be_an_instance_of(arc.ComputingServiceRetriever)

    def test_getting_a_target(self):
        retriever = arc.ComputingServiceRetriever(self.usercfg)
        self.expect(retriever).to_be_empty()
        retriever.addEndpoint(self.ce)
        retriever.wait()
        self.expect(retriever).to_have(1).target()
        etlist = retriever.GetExecutionTargets()
        self.expect(etlist).to_have(1).target()


if __name__ == '__main__':
    unittest.main()
