import arcom.test, arc, unittest, time

class ServiceEndpointRetrieverPluginTest(arcom.test.ARCClientTestCase):

    def test_loader_exists(self):
        self.expect(arc.ServiceEndpointRetrieverPluginLoader).to_be_an_instance_of(type)
        
    def test_constructor(self):
        loader = arc.ServiceEndpointRetrieverPluginLoader()
        self.expect(loader).to_be_an_instance_of(arc.ServiceEndpointRetrieverPluginLoader)
        
    def test_loading_test_acc(self):
        loader = arc.ServiceEndpointRetrieverPluginLoader()
        plugin = loader.load("TEST")
        self.expect(plugin).to_be_an_instance_of(arc.ServiceEndpointRetrieverPlugin)

    def test_list_of_plugins(self):
        loader = arc.ServiceEndpointRetrieverPluginLoader()
        self.expect(loader.getListOfPlugins()).to_contain("TEST")

if __name__ == '__main__':
    unittest.main()