import arcom.test, arc, unittest, time

class ServiceEndpointRetrieverTest(arcom.test.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
        arc.ServiceEndpointRetrieverTESTControl.delay = 0
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [arc.ServiceEndpoint()]
        arc.ServiceEndpointRetrieverTESTControl.status = arc.RegistryEndpointStatus(arc.SER_SUCCESSFUL)

    def test_the_class_exists(self):
        self.expect(arc.ServiceEndpointRetriever).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, [], container)
        self.expect(retriever).to_be_an_instance_of(arc.ServiceEndpointRetriever)
        
    def test_getting_the_endpoints(self):
        container = arc.ServiceEndpointContainer()
        self.expect(container.endpoints).to_be_empty()
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "TEST")]
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
        
    def test_getting_status(self):
        container = arc.ServiceEndpointContainer()
        registry = arc.RegistryEndpoint("test.nordugrid.org", "TEST")
        arc.ServiceEndpointRetrieverTESTControl.status = arc.RegistryEndpointStatus(arc.SER_FAILED)        
        retriever = arc.ServiceEndpointRetriever(self.usercfg, [registry], container)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status).to_be_an_instance_of(arc.RegistryEndpointStatus)
        self.expect(status.status).to_be(arc.SER_FAILED)
        
    def test_constructor_returns_immediately(self):
        container = arc.ServiceEndpointContainer()
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "TEST")]
        arc.ServiceEndpointRetrieverTESTControl.delay = 1
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        # the endpoint should not arrive yet
        self.expect(container.endpoints).to_have(0).endpoints()
        # wait while it finishes to avoid the container getting out of scope
        retriever.wait()

if __name__ == '__main__':
    unittest.main()