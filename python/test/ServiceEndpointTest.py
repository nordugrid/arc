import arcom.test, arc, unittest

class ServiceEndpointTest(arcom.test.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.ServiceEndpoint).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        service = arc.ServiceEndpoint()
        self.expect(service).to_be_an_instance_of(arc.ServiceEndpoint)
        
    def test_default_attributes_are_empty(self):
        service = arc.ServiceEndpoint()
        self.expect(service.URLString).to_be("")

    def test_constructor_with_values(self):
        service = arc.ServiceEndpoint("test.nordugrid.org", "org.nordugrid.wsrfglue2")
        self.expect(service.URLString).to_be("test.nordugrid.org")
        self.expect(service.InterfaceName).to_contain("org.nordugrid.wsrfglue2")
        
if __name__ == '__main__':
    unittest.main()