import testutils, arc, unittest

class EndpointContainerTest(testutils.ARCClientTestCase):

    def test_the_class_exists(self):
        self.expect(arc.EndpointContainer).to_be_an_instance_of(type)
        
    def test_the_constructor(self):
        container = arc.EndpointContainer()
        self.expect(container).to_be_an_instance_of(arc.EndpointContainer)
        
    def test_adding_endpoints(self):
        container = arc.EndpointContainer()
        endpoint1 = arc.Endpoint()
        endpoint2 = arc.Endpoint()
        container.addEntity(endpoint1)
        container.addEntity(endpoint2)
        self.expect(container).to_have(2).endpoints()

if __name__ == '__main__':
    unittest.main()