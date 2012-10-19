import testutils, arc, unittest, time

class TargetInformationRetrieverTest(testutils.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
        self.ce = arc.Endpoint()
        self.ce.URLString = "test.nordugrid.org"
        self.ce.InterfaceName = "org.nordugrid.tirtest"
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0
        arc.TargetInformationRetrieverPluginTESTControl.targets = [arc.ComputingServiceType()]
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL)

    def test_the_class_exists(self):
        self.expect(arc.TargetInformationRetriever).to_be_an_instance_of(type)

    def test_the_constructor(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        self.expect(retriever).to_be_an_instance_of(arc.TargetInformationRetriever)

    def test_getting_a_target(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        self.expect(container).to_be_empty()
        retriever.addEndpoint(self.ce)
        retriever.wait()
        self.expect(container).to_have(1).target()

    def test_getting_a_target_without_interfacename_specified(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        self.expect(container).to_be_empty()
        self.ce.InterfaceName = ""
        retriever.addEndpoint(self.ce)
        retriever.wait()
        self.expect(container).to_have(1).target()

    def test_getting_status(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL, "TEST")
        retriever.addEndpoint(self.ce)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be_an_instance_of(arc.EndpointQueryingStatus)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)
        self.expect(status.getDescription()).to_be("TEST")

    def test_getting_status_without_interfacename_specified(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL, "TEST")
        self.ce.InterfaceName = ""
        retriever.addEndpoint(self.ce)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)
        self.expect(status.getDescription()).to_be("TEST")

    def test_the_status_is_started_first(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0.1
        retriever.addEndpoint(self.ce)
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be(arc.EndpointQueryingStatus.STARTED)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)

    def test_same_endpoint_is_not_queried_twice(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        retriever.addEndpoint(self.ce)
        retriever.addEndpoint(self.ce)
        retriever.wait()
        self.expect(container).to_have(1).target()

    def test_removing_the_consumer(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0.1
        retriever.addEndpoint(self.ce)
        retriever.removeConsumer(container)
        retriever.wait()
        self.expect(container).to_have(0).targets()

    def test_deleting_the_consumer_before_the_retriever(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ComputingServiceContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0.1
        retriever.addEndpoint(self.ce)
        retriever.removeConsumer(container)
        del container
        retriever.wait()
        # expect it not to crash

    def test_two_consumers(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container1 = arc.ComputingServiceContainer()
        container2 = arc.ComputingServiceContainer()
        retriever.addConsumer(container1)
        retriever.addConsumer(container2)
        retriever.addEndpoint(self.ce)
        retriever.wait()
        self.expect(container1).to_have(1).target()
        self.expect(container2).to_have(1).target()

if __name__ == '__main__':
    unittest.main()
