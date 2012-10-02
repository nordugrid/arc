import testutils, arc, unittest

class EndpointTest(testutils.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.Endpoint).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        registry = arc.Endpoint()
        self.expect(registry).to_be_an_instance_of(arc.Endpoint)
        
    def test_default_attributes_are_empty(self):
        registry = arc.Endpoint()
        self.expect(registry.URLString).to_be("")
        self.expect(registry.Capability).to_have(0).capabilities()
        self.expect(registry.InterfaceName).to_be("")

    def test_constructor_with_values(self):
        registry = arc.Endpoint("test.nordugrid.org", ["information.discovery.registry"], "org.nordugrid.sertest")
        self.expect(registry.URLString).to_be("test.nordugrid.org")
        self.expect(registry.Capability[0]).to_be("information.discovery.registry")
        self.expect(registry.InterfaceName).to_be("org.nordugrid.sertest")
        
    def test_constructor_with_enum_registry(self):
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        self.expect(registry.URLString).to_be("test.nordugrid.org")
        self.expect(registry.Capability[0]).to_be("information.discovery.registry")
        self.expect(registry.InterfaceName).to_be("")

    def test_constructor_with_enum_computing(self):
        endpoint = arc.Endpoint("test.nordugrid.org", arc.Endpoint.COMPUTINGINFO)
        self.expect(endpoint.URLString).to_be("test.nordugrid.org")
        self.expect(endpoint.Capability[0]).to_be("information.discovery.resource")
        self.expect(endpoint.InterfaceName).to_be("")

    def test_has_capability_with_enum(self):
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        self.expect(registry.HasCapability(arc.Endpoint.REGISTRY)).to_be(True);

    def test_has_capability_with_string(self):
        registry = arc.Endpoint("test.nordugrid.org", arc.Endpoint.REGISTRY)
        self.expect(registry.HasCapability("information.discovery.registry")).to_be(True);
        
    def test_string_representation(self):
        registry = arc.Endpoint("test.nordugrid.org", ["information.discovery.registry"], "org.nordugrid.sertest")
        self.expect(registry.str()).to_be("test.nordugrid.org (registry, sertest)")
        
              
if __name__ == '__main__':
    unittest.main()