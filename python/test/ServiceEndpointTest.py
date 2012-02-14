import arcom.test, arc, unittest

class ServiceEndpointTest(arcom.test.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.ServiceEndpoint).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        service = arc.ServiceEndpoint()
        self.expect(service).to_be_an_instance_of(arc.ServiceEndpoint)
        
    def test_default_attributes_are_empty(self):
        service = arc.ServiceEndpoint()
        self.expect(service.EndpointURL.str()).to_be("")
        self.expect(service.EndpointCapabilities).to_have(0).items()

    def test_constructor_with_values(self):
        service = arc.ServiceEndpoint(arc.URL("http://test.nordugrid.org:60000"), ["information.discovery.resource"])
        self.expect(service.EndpointURL.str()).to_be("http://test.nordugrid.org:60000")
        self.expect(service.EndpointCapabilities).to_contain("information.discovery.resource")
        
if __name__ == '__main__':
    unittest.main()