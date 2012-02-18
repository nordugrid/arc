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
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        self.expect(retriever).to_be_an_instance_of(arc.ServiceEndpointRetriever)
    
    def test_getting_the_endpoints(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        self.expect(container.endpoints).to_be_empty()
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
    
    def test_getting_status(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverTESTControl.status = arc.RegistryEndpointStatus(arc.SER_FAILED)        
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status).to_be_an_instance_of(arc.RegistryEndpointStatus)
        self.expect(status.status).to_be(arc.SER_FAILED)
    
    def test_the_status_is_started_first(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverTESTControl.delay = 0.3
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        time.sleep(0.2)
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status.status).to_be(arc.SER_STARTED)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status.status).to_be(arc.SER_SUCCESSFUL)        
        
    def test_constructor_returns_immediately(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverTESTControl.delay = 0.1
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        # the endpoint should not arrive yet
        self.expect(container.endpoints).to_have(0).endpoints()
        # we are not interested in it anymore
        retriever.removeConsumer(container)
        
    def test_same_endpoint_is_not_queried_twice(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
        
    def test_filtering(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg, False, ["cap1"])
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [
            arc.ServiceEndpoint("test1.nordugrid.org",["cap1","cap2"]),
            arc.ServiceEndpoint("test2.nordugrid.org",["cap3","cap4"]),
            arc.ServiceEndpoint("test3.nordugrid.org",["cap1","cap3"])
        ]
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container.endpoints).to_have(2).endpoints()
    
        retriever = arc.ServiceEndpointRetriever(self.usercfg, False, ["cap2"])
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container.endpoints).to_have(1).endpoint()
    
        retriever = arc.ServiceEndpointRetriever(self.usercfg, False, ["cap5"])
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container.endpoints).to_have(0).endpoints()
    
    def test_recursivity(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg, True)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverTESTControl.endpoints = [
            arc.ServiceEndpoint("emir.nordugrid.org", ["information.discovery.registry"], "org.nordugrid.sertest"),
            arc.ServiceEndpoint("ce.nordugrid.org", ["information.discovery.resource"], "org.ogf.emies"),            
        ]
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        # expect to get both service endpoints twice
        # once from test.nordugrid.org, once from emir.nordugrid.org
        self.expect(container.endpoints).to_have(4).endpoints()
        emirs = [endpoint for endpoint in container.endpoints if "emir" in endpoint.EndpointURL]
        ces = [endpoint for endpoint in container.endpoints if "ce" in endpoint.EndpointURL]
        self.expect(emirs).to_have(2).endpoints()
        self.expect(ces).to_have(2).endpoints()
    
    def test_empty_registry_type(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        registry = arc.RegistryEndpoint("test.nordugrid.org")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        # it should fill the empty type with the available plugins:
        # among them the TEST plugin which returns one endpoint
        self.expect(container.endpoints).to_have(1).endpoint()
    
    def test_status_of_typeless_registry(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        registry = arc.RegistryEndpoint("test.nordugrid.org")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        status = retriever.getStatusOfRegistry(registry)
        self.expect(status.status).to_be(arc.SER_SUCCESSFUL)
        
    def test_deleting_the_consumer_before_the_retriever(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        arc.ServiceEndpointRetrieverTESTControl.delay = 0.1
        retriever.addRegistryEndpoint(registry)
        retriever.removeConsumer(container)
        del container
        retriever.wait()
        # expect it not to crash
        
    def test_works_without_consumer(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        # expect it not to crash
        
    def test_removing_consumer(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.ServiceEndpointContainer()
        retriever.addConsumer(container)
        retriever.removeConsumer(container)
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container.endpoints).to_have(0).endpoints()
        
        
    def test_two_consumers(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container1 = arc.ServiceEndpointContainer()
        container2 = arc.ServiceEndpointContainer()
        retriever.addConsumer(container1)
        retriever.addConsumer(container2)
        registry = arc.RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest")
        retriever.addRegistryEndpoint(registry)
        retriever.wait()
        self.expect(container1.endpoints).to_have(1).endpoint()
        self.expect(container2.endpoints).to_have(1).endpoint()

if __name__ == '__main__':
    unittest.main()