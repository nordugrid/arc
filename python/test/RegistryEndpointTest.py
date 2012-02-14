import arcom.test, arc, unittest

class RegistryEndpointTest(arcom.test.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.RegistryEndpoint).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        registry = arc.RegistryEndpoint()
        self.expect(registry).to_be_an_instance_of(arc.RegistryEndpoint)
        
    def test_default_attributes_are_empty(self):
        registry = arc.RegistryEndpoint()
        self.expect(registry.Endpoint).to_be("")
        self.expect(registry.Type).to_be("")

    def test_constructor_with_values(self):
        registry = arc.RegistryEndpoint("test.nordugrid.org", "TEST")
        self.expect(registry.Endpoint).to_be("test.nordugrid.org")
        self.expect(registry.Type).to_be("TEST")
        
    def test_string_representation(self):
        registry = arc.RegistryEndpoint("test.nordugrid.org", "TEST")
        self.expect(registry.str()).to_be("test.nordugrid.org (TEST)")
        
    def test_check_if_service_is_registry(self):
        service = arc.ServiceEndpoint(
            arc.URL("http://test.nordugrid.org:60000"),
            ["information.discovery.registry"],
            "org.nordugrid.ldapegiis"
        )
        self.expect(arc.RegistryEndpoint.isRegistry(service)).to_be(True)
        service = arc.ServiceEndpoint(
            arc.URL("http://test.nordugrid.org:60000"),
            ["information.discovery.resource"],
            "org.nordugrid.wsrfglue2"
        )
        self.expect(arc.RegistryEndpoint.isRegistry(service)).to_be(False)
        
    def test_create_registry_from_service_endpoint(self):
        service = arc.ServiceEndpoint(
            arc.URL("http://test.nordugrid.org:60000"),
            ["information.discovery.registry"],
            "org.nordugrid.ldapegiis"
        )
        registry = arc.RegistryEndpoint(service)
        self.expect(registry).to_be_an_instance_of(arc.RegistryEndpoint)
        self.expect(registry.Endpoint).to_be("http://test.nordugrid.org:60000")
        self.expect(registry.Type).to_be("EGIIS")        
        
if __name__ == '__main__':
    unittest.main()