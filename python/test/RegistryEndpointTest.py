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
        
if __name__ == '__main__':
    unittest.main()