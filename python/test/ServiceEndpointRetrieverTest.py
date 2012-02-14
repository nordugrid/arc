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
        arc.ServiceEndpointRetrieverTESTControl.delay = 0.1
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        # the endpoint should not arrive yet
        self.expect(container.endpoints).to_have(0).endpoints()
        # wait while it finishes to avoid the container getting out of scope
        retriever.wait()
        
    def test_same_endpoint_is_not_queried_twice(self):
        container = arc.ServiceEndpointContainer()
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "TEST"), arc.RegistryEndpoint("test.nordugrid.org", "TEST")]
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
        
    def test_filtering(self):
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "TEST")]
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [
            arc.ServiceEndpoint(arc.URL("http://test1.nordugrid.org:60000"),["cap1","cap2"]),
            arc.ServiceEndpoint(arc.URL("http://test2.nordugrid.org:60000"),["cap3","cap4"]),
            arc.ServiceEndpoint(arc.URL("http://test3.nordugrid.org:60000"),["cap1","cap3"])
        ]
        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container, False, ["cap1"])
        retriever.wait()
        self.expect(container.endpoints).to_have(2).endpoints()

        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container, False, ["cap2"])
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()

        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container, False, ["cap5"])
        retriever.wait()
        self.expect(container.endpoints).to_have(0).endpoints()

    def test_recursivity(self):
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "TEST")]
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [
            arc.ServiceEndpoint(arc.URL("http://emir.nordugrid.org"), ["information.discovery.registry"], "org.nordugrid.test"),
            arc.ServiceEndpoint(arc.URL("http://ce.nordugrid.org"), ["information.discovery.resource"], "org.ogf.emies"),            
        ]
        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container, True)
        retriever.wait()
        # expect to get both service endpoints twice
        # once from test.nordugrid.org, once from emir.nordugrid.org
        self.expect(container.endpoints).to_have(4).endpoints()
        emirs = [endpoint for endpoint in container.endpoints if "emir" in endpoint.EndpointURL.str()]
        ces = [endpoint for endpoint in container.endpoints if "ce" in endpoint.EndpointURL.str()]
        self.expect(emirs).to_have(2).endpoints()
        self.expect(ces).to_have(2).endpoints()
        

if __name__ == '__main__':
    unittest.main()