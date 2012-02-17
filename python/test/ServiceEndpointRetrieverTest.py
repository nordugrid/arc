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
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")]
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
    
    def test_getting_status(self):
        container = arc.ServiceEndpointContainer()
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        arc.ServiceEndpointRetrieverTESTControl.status = arc.RegistryEndpointStatus(arc.SER_FAILED)        
        retriever = arc.ServiceEndpointRetriever(self.usercfg, [registry], container)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status).to_be_an_instance_of(arc.RegistryEndpointStatus)
        self.expect(status.status).to_be(arc.SER_FAILED)
    
    def test_the_status_is_started_first(self):
        container = arc.ServiceEndpointContainer()
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        arc.ServiceEndpointRetrieverTESTControl.delay = 0.3
        retriever = arc.ServiceEndpointRetriever(self.usercfg, [registry], container)
        time.sleep(0.2)
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status.status).to_be(arc.SER_STARTED)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status.status).to_be(arc.SER_SUCCESSFUL)        
        
    def test_constructor_returns_immediately(self):
        container = arc.ServiceEndpointContainer()
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")]
        arc.ServiceEndpointRetrieverTESTControl.delay = 0.1
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        # the endpoint should not arrive yet
        self.expect(container.endpoints).to_have(0).endpoints()
        
    def test_same_endpoint_is_not_queried_twice(self):
        container = arc.ServiceEndpointContainer()
        registries = [
            arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest"),
            arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        ]
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
        
    def test_filtering(self):
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")]
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [
            arc.ServiceEndpoint("test1.nordugrid.org",["cap1","cap2"]),
            arc.ServiceEndpoint("test2.nordugrid.org",["cap3","cap4"]),
            arc.ServiceEndpoint("test3.nordugrid.org",["cap1","cap3"])
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
        registries = [arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")]
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [
            arc.ServiceEndpoint("emir.nordugrid.org", ["information.discovery.registry"], "org.nordugrid.sertest"),
            arc.ServiceEndpoint("ce.nordugrid.org", ["information.discovery.resource"], "org.ogf.emies"),            
        ]
        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container, True)
        retriever.wait()
        # expect to get both service endpoints twice
        # once from test.nordugrid.org, once from emir.nordugrid.org
        self.expect(container.endpoints).to_have(4).endpoints()
        emirs = [endpoint for endpoint in container.endpoints if "emir" in endpoint.EndpointURL]
        ces = [endpoint for endpoint in container.endpoints if "ce" in endpoint.EndpointURL]
        self.expect(emirs).to_have(2).endpoints()
        self.expect(ces).to_have(2).endpoints()
    
    def test_empty_registry_type(self):
        registries = [arc.RegistryEndpoint("test.nordugrid.org")]
        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, registries, container)
        retriever.wait()
        # it should fill the empty type with the available plugins:
        # among them the TEST plugin which returns one endpoint
        self.expect(container.endpoints).to_have(1).endpoint()
    
    def test_status_of_typeless_registry(self):
        registry = arc.RegistryEndpoint("test.nordugrid.org")
        container = arc.ServiceEndpointContainer()
        retriever = arc.ServiceEndpointRetriever(self.usercfg, [registry], container)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status.status).to_be(arc.SER_SUCCESSFUL)


if __name__ == '__main__':
    unittest.main()