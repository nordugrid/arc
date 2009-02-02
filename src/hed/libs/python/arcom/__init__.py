def import_class_from_string(str):
    """ Import the class which name is in the string
    
    import_class_from_string(str)

    'str' is the name of the class in [package.]*module.classname format.
    """
    module = '.'.join(str.split('.')[:-1]) # cut off class name
    cl = __import__(module) # import module
    for name in str.split('.')[1:]: # start from the first package name
        # step to the next package or the module or the class
        cl = getattr(cl, name)
    return cl # return the class

# for XMLNode

def get_attributes(node):
    return dict([(attribute.Name(), str(attribute)) for attribute in [node.Attribute(i) for i in range(node.AttributesSize())]])

def get_child_nodes(node):
    """ Get all the children nodes of an XMLNode
    
    get_child_nodes(node)
    
    node is the XMLNode whose children we need
    """
    # the node.Size() method returns the number of children
    return [node.Child(i) for i in range(node.Size())]

# for DataHandle and DataPoint

def datapoint_from_url(url):
    import arc
    handle = arc.DataHandle(url)
    handle.thisown = False
    point = handle.__ref__()
    return point

# for the URL class

def parse_url(url):
    import arc
    url = arc.URL(url)
    proto = url.Protocol()
    host = url.Host()
    port = url.Port()
    path = url.Path()
    return proto, host, int(port), path
