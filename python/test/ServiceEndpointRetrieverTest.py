import testutils, arc, unittest, time

class ServiceEndpointRetrieverTest(testutils.ARCClientTestCase):

    def setUp(self):
        self.usercfg = arc.UserConfig(arc.initializeCredentialsType(arc.initializeCredentialsType.SkipCredentials))
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.clear()
        arc.ServiceEndpointRetrieverPluginTESTControl.status.clear()
        arc.ServiceEndpointRetrieverPluginTESTControl.condition.clear()
        
    def tearDown(self):
      try:
        self.condition.signal()
        del self.condition
      except AttributeError:
        pass
      try:
        self.retriever.wait()
        del self.retriever
      except AttributeError:
        pass

    def test_the_class_exists(self):
        self.expect(arc.ServiceEndpointRetriever).to_be_an_instance_of(type)
    
    def test_the_constructor(self):
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        self.expect(self.retriever).to_be_an_instance_of(arc.ServiceEndpointRetriever)
    
    def test_getting_the_endpoints(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        self.expect(container).to_be_empty()
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(1).endpoint()
    
    def test_getting_status(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.FAILED))

        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        status = self.retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be_an_instance_of(arc.EndpointQueryingStatus)
        self.expect(status).to_be(arc.EndpointQueryingStatus.FAILED)
    
    def test_the_status_is_started_first(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        self.condition = arc.SimpleCondition()
        arc.ServiceEndpointRetrieverPluginTESTControl.condition.push_back(self.condition)

        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        status = self.retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be(arc.EndpointQueryingStatus.STARTED)
        self.condition.signal()
        self.retriever.wait()
        status = self.retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)
        
    def test_constructor_returns_immediately(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        self.condition = arc.SimpleCondition()
        arc.ServiceEndpointRetrieverPluginTESTControl.condition.push_back(self.condition)
      
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        # the endpoint should not arrive yet
        self.expect(container).to_have(0).endpoints()
        self.condition.signal()
        # we are not interested in it anymore
        self.retriever.removeConsumer(container)
        # we must wait until self.retriever is done otherwise 'condition' will go out of scope while being used.
        self.retriever.wait()

    def test_same_endpoint_is_not_queried_twice(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(1).endpoint()
        
    def test_filtering(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
            arc.Endpoint("test1.nordugrid.org", ["cap1","cap2"]),
            arc.Endpoint("test2.nordugrid.org", ["cap3","cap4"]),
            arc.Endpoint("test3.nordugrid.org", ["cap1","cap3"])
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
            arc.Endpoint("test1.nordugrid.org", ["cap1","cap2"]),
            arc.Endpoint("test2.nordugrid.org", ["cap3","cap4"]),
            arc.Endpoint("test3.nordugrid.org", ["cap1","cap3"])
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
            arc.Endpoint("test1.nordugrid.org", ["cap1","cap2"]),
            arc.Endpoint("test2.nordugrid.org", ["cap3","cap4"]),
            arc.Endpoint("test3.nordugrid.org", ["cap1","cap3"])
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")

        options = arc.ServiceEndpointQueryOptions(False, ["cap1"])
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(2).endpoints()
    
        options = arc.ServiceEndpointQueryOptions(False, ["cap2"])
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(1).endpoint()
    
        options = arc.ServiceEndpointQueryOptions(False, ["cap5"])
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(0).endpoints()
    
    def test_recursivity(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
            arc.Endpoint("emir.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest"),
            arc.Endpoint("ce.nordugrid.org", arc.Endpoint.COMPUTINGINFO, "org.ogf.glue.emies.resourceinfo"),
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
            arc.Endpoint("emir.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest"),
            arc.Endpoint("ce.nordugrid.org", arc.Endpoint.COMPUTINGINFO, "org.ogf.glue.emies.resourceinfo"),
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        options = arc.ServiceEndpointQueryOptions(True)
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        # expect to get both service endpoints twice
        # once from test.nordugrid.org, once from emir.nordugrid.org
        self.expect(container).to_have(4).endpoints()
        emirs = [endpoint for endpoint in container if "emir" in endpoint.URLString]
        ces = [endpoint for endpoint in container if "ce" in endpoint.URLString]
        self.expect(emirs).to_have(2).endpoints()
        self.expect(ces).to_have(2).endpoints()

    def test_recursivity_with_filtering(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
           arc.Endpoint("emir.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest"),
           arc.Endpoint("ce.nordugrid.org", arc.Endpoint.COMPUTINGINFO, "org.ogf.glue.emies.resourceinfo"),
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
           arc.Endpoint("emir.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest"),
           arc.Endpoint("ce.nordugrid.org", arc.Endpoint.COMPUTINGINFO, "org.ogf.glue.emies.resourceinfo"),
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        options = arc.ServiceEndpointQueryOptions(True, ["information.discovery.resource"])
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
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
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([
            arc.Endpoint(rejected),
            arc.Endpoint(not_rejected)
        ])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        options = arc.ServiceEndpointQueryOptions(False, [], [rejected])
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg, options)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        
        registry = arc.Endpoint("registry.nordugrid.org", arc.Endpoint.REGISTRY)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(1).endpoint()
        self.expect(container[0].URLString).to_be(not_rejected)
    
    def test_empty_registry_type(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back(arc.EndpointList())
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        # it should fill the empty type with the available plugins:
        # among them the TEST plugin which doesn't return any endpoint
        self.expect(container).to_have(0).endpoint()
    
    def test_status_of_typeless_registry(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back(arc.EndpointList())
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        status = self.retriever.getStatusOfEndpoint(registry)
        self.expect(status).to_be(arc.EndpointQueryingStatus.SUCCESSFUL)
        
    def test_deleting_the_consumer_before_the_retriever(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back(arc.EndpointList())
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))

        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.removeConsumer(container)
        del container
        self.retriever.wait()
        # expect it not to crash
        
    def test_works_without_consumer(self):
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        # expect it not to crash
        
    def test_removing_consumer(self):
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container = arc.EndpointContainer()
        self.retriever.addConsumer(container)
        self.retriever.removeConsumer(container)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container).to_have(0).endpoints()
        
        
    def test_two_consumers(self):
        arc.ServiceEndpointRetrieverPluginTESTControl.endpoints.push_back([arc.Endpoint()])
        arc.ServiceEndpointRetrieverPluginTESTControl.status.push_back(arc.EndpointQueryingStatus(arc.EndpointQueryingStatus.SUCCESSFUL))
        self.retriever = arc.ServiceEndpointRetriever(self.usercfg)
        container1 = arc.EndpointContainer()
        container2 = arc.EndpointContainer()
        self.retriever.addConsumer(container1)
        self.retriever.addConsumer(container2)
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.sertest")
        self.retriever.addEndpoint(registry)
        self.retriever.wait()
        self.expect(container1).to_have(1).endpoint()
        self.expect(container2).to_have(1).endpoint()

if __name__ == '__main__':
    unittest.main()
