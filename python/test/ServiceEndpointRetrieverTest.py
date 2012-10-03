import testutils, arc, unittest, time

class ServiceEndpointRetrieverTest(testutils.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
        arc.ServiceEndpointRetrieverPluginTESTControl.delay = 0
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints = [arc.Endpoint()]
        arc.ServiceEndpointRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL)

    def test_the_class_exists(self):
        self.expect(arc.ServiceEndpointRetriever).to_be_an_instance_of(type)
    
    def test_the_constructor(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        self.expect(retriever).to_be_an_instance_of(arc.ServiceEndpointRetriever)
    
    def test_getting_the_endpoints(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        self.expect(container).to_be_empty()
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(1).endpoint()
    
    def test_getting_status(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverPluginTESTControl.status = arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.FAILED)        
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be_an_instance_of(arc.EndpointQueryingStatus)
        self.expect(status).to_be(arc.EndpointQueryingStatus.FAILED)
    
    def test_the_status_is_started_first(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverPluginTESTControl.delay = 1.0
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        time.sleep(0.8)
        status = retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be(arc.EndpointQueryingStatus.STARTED)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)        
        
    def test_constructor_returns_immediately(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverPluginTESTControl.delay = 0.01
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        # the endpoint should not arrive yet
        self.expect(container).to_have(0).endpoints()
        # we are not interested in it anymore
        retriever.removeConsumer(container)
        
    def test_same_endpoint_is_not_queried_twice(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(1).endpoint()
        
    def test_filtering(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints = [
            arc.Endpoint("test1.nordugrid.org", ["cap1","cap2"]),
            arc.Endpoint("test2.nordugrid.org", ["cap3","cap4"]),
            arc.Endpoint("test3.nordugrid.org", ["cap1","cap3"])
        ]
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")

        options = arc.ServiceEndpointQueryOptions(False, ["cap1"])
        retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(2).endpoints()
    
        options = arc.ServiceEndpointQueryOptions(False, ["cap2"])
        retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(1).endpoint()
    
        options = arc.ServiceEndpointQueryOptions(False, ["cap5"])
        retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(0).endpoints()
    
    def test_recursivity(self):
        options = arc.ServiceEndpointQueryOptions(True)
        retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints = [
            arc.Endpoint("emir.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest"),
            arc.Endpoint("ce.nordugrid.org", arc.Endpoint.COMPUTINGINFO, "org.ogf.glue.emies.resourceinfo"),
        ]
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        # expect to get both service endpoints twice
        # once from test.nordugrid.org, once from emir.nordugrid.org
        self.expect(container).to_have(4).endpoints()
        emirs = [endpoint for endpoint in container if "emir" in endpoint.URLString]
        ces = [endpoint for endpoint in container if "ce" in endpoint.URLString]
        self.expect(emirs).to_have(2).endpoints()
        self.expect(ces).to_have(2).endpoints()

    def test_recursivity_with_filtering(self):
        options = arc.ServiceEndpointQueryOptions(True, ["information.discovery.resource"])
        retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints = [
           arc.Endpoint("emir.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest"),
           arc.Endpoint("ce.nordugrid.org", arc.Endpoint.COMPUTINGINFO, "org.ogf.glue.emies.resourceinfo"),
        ]
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        # expect to only get the ce.nordugrid.org, but that will be there twice
        # once from test.nordugrid.org, once from emir.nordugrid.org
        self.expect(container).to_have(2).endpoints()
        emirs = [endpoint for endpoint in container if "emir" in endpoint.URLString]
        ces = [endpoint for endpoint in container if "ce" in endpoint.URLString]
        self.expect(emirs).to_have(0).endpoints()
        self.expect(ces).to_have(2).endpoints()
        
    def test_rejected_services(self):
        rejected = "http://test.nordugrid.org"
        not_rejected = "http://test2.nordugrid.org"
        options = arc.ServiceEndpointQueryOptions(False, [], [rejected])
        retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints = [
            arc.Endpoint(rejected),
            arc.Endpoint(not_rejected)
        ]
        registry = arc.Endpoint("registry.nordugrid.org", arc.Endpoint.REGISTRY)
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(1).endpoint()
        self.expect(container[0].URLString).to_be(not_rejected)
    
    def test_empty_registry_type(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        retriever.addEndpoint(registry)
        retriever.wait()
        # it should fill the empty type with the available plugins:
        # among them the TEST plugin which returns one endpoint
        self.expect(container).to_have(1).endpoint()
    
    def test_status_of_typeless_registry(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        retriever.addEndpoint(registry)
        retriever.wait()
        status = retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)
        
    def test_deleting_the_consumer_before_the_retriever(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        arc.ServiceEndpointRetrieverPluginTESTControl.delay = 0.01
        retriever.addEndpoint(registry)
        retriever.removeConsumer(container)
        del container
        retriever.wait()
        # expect it not to crash
        
    def test_works_without_consumer(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        # expect it not to crash
        
    def test_removing_consumer(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        retriever.addConsumer(container)
        retriever.removeConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container).to_have(0).endpoints()
        
        
    def test_two_consumers(self):
        retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container1 = arc.EndpointContainer()
        container2 = arc.EndpointContainer()
        retriever.addConsumer(container1)
        retriever.addConsumer(container2)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        retriever.addEndpoint(registry)
        retriever.wait()
        self.expect(container1).to_have(1).endpoint()
        self.expect(container2).to_have(1).endpoint()

if __name__ == '__main__':
    unittest.main()
