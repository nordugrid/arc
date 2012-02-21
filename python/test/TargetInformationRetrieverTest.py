import arcom.test, arc, unittest, time

class TargetInformationRetrieverTest(arcom.test.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
        self.ce = arc.ComputingInfoEndpoint()
        self.ce.Endpoint = "test.nordugrid.org"
        self.ce.InterfaceName = "org.nordugrid.tirtest"
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0
        arc.TargetInformationRetrieverPluginTESTControl.targets = [arc.ExecutionTarget()]
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL)

    def test_the_class_exists(self):
        self.expect(arc.TargetInformationRetriever).to_be_an_instance_of(type)
    
    def test_the_constructor(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        self.expect(retriever).to_be_an_instance_of(arc.TargetInformationRetriever)

    def test_getting_a_target(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        self.expect(container.targets).to_be_empty()
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.wait()
        self.expect(container.targets).to_have(1).target()

    def test_getting_a_target_without_interfacename_specified(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        self.expect(container.targets).to_be_empty()
        self.ce.InterfaceName = ""
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.wait()
        self.expect(container.targets).to_have(1).target()

    def test_getting_status(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.FAILED)
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be_an_instance_of(arc.EndpointQueryingStatus)
        self.expect(status).to_be(arc.EndpointQueryingStatus.FAILED)

    def test_getting_status_without_interfacename_specified(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.FAILED)
        self.ce.InterfaceName = ""
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be(arc.EndpointQueryingStatus.FAILED)
    
    def test_the_status_is_STARTED_first(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0.1
        retriever.addComputingInfoEndpoint(self.ce)
        time.sleep(0.08)
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be(arc.EndpointQueryingStatus.STARTED)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(self.ce)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)

    def test_same_endpoint_is_not_queried_twice(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.wait()
        self.expect(container.targets).to_have(1).target()

    def test_removing_the_consumer(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0.1
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.removeConsumer(container)
        retriever.wait()
        self.expect(container.targets).to_have(0).targets()

    def test_deleting_the_consumer_before_the_retriever(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container = arc.ExecutionTargetContainer()
        retriever.addConsumer(container)
        arc.TargetInformationRetrieverPluginTESTControl.delay = 0.1
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.removeConsumer(container)
        del container
        retriever.wait()
        # expect it not to crash
 
    def test_two_consumers(self):
        retriever = arc.TargetInformationRetriever(self.usercfg)
        container1 = arc.ExecutionTargetContainer()
        container2 = arc.ExecutionTargetContainer()
        retriever.addConsumer(container1)
        retriever.addConsumer(container2)
        retriever.addComputingInfoEndpoint(self.ce)
        retriever.wait()
        self.expect(container1.targets).to_have(1).target()
        self.expect(container2.targets).to_have(1).target()

if __name__ == '__main__':
    unittest.main()