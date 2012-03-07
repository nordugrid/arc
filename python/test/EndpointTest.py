import arcom.test, arc, unittest

class EndpointTest(arcom.test.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.Endpoint).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        registry = arc.Endpoint()
        self.expect(registry).to_be_an_instance_of(arc.Endpoint)
        
    def test_default_attributes_are_empty(self):
        registry = arc.Endpoint()
        self.expect(registry.URLString).to_be("")
        self.expect(registry.InterfaceName).to_be("")

    def test_constructor_with_values(self):
        registry = arc.Endpoint("test.nordugrid.org", "org.nordugrid.sertest")
        self.expect(registry.URLString).to_be("test.nordugrid.org")
        self.expect(registry.InterfaceName).to_be("org.nordugrid.sertest")
        
    def test_string_representation(self):
        registry = arc.Endpoint("test.nordugrid.org", "org.nordugrid.sertest")
        self.expect(registry.str()).to_be("test.nordugrid.org (org.nordugrid.sertest)")
        
    def test_check_if_service_is_registry(self):
        service = arc.Endpoint(
            "test.nordugrid.org",
            "org.nordugrid.ldapegiis",
            ["information.discovery.registry"]
        )
        self.expect(service.HasCapability(arc.Endpoint.REGISTRY)).to_be(True)
        service = arc.Endpoint(
            "test.nordugrid.org",
            "org.nordugrid.wsrfglue2",
            ["information.discovery.resource"]
        )
        self.expect(service.HasCapability(arc.Endpoint.REGISTRY)).to_be(False)
              
if __name__ == '__main__':
    unittest.main()