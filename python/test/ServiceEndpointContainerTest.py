import arcom.test, arc, unittest

class ServiceEndpointContainerTest(arcom.test.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.ServiceEndpointContainer).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        container = arc.ServiceEndpointContainer()
        self.expect(container).to_be_an_instance_of(arc.ServiceEndpointContainer)
        
    def test_adding_endpoints(self):
        container = arc.ServiceEndpointContainer()
        endpoint1 = arc.ServiceEndpoint()
        endpoint2 = arc.ServiceEndpoint()
        container.addEndpoint(endpoint1)
        container.addEndpoint(endpoint2)
        self.expect(container).to_have(2).endpoints()

if __name__ == '__main__':
    unittest.main()