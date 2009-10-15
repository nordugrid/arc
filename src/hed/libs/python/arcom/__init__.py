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

def get_child_values_by_name(node, name):
    """ Get the non-empty children nodes with a given name.
    
    get_child_values_by_name(node, name):
    
    node is the XMLNode whose children we need
    name is the name of the children we need
    
    e.g.
    In [23]: print x.GetXML()
    <Service name="pythonservice" id="librarian">
        <ClassName>storage.librarian.librarian.LibrarianService</ClassName>
        <ISISURL>https://localhost:60000/ISIS1</ISISURL>
        <ISISURL>https://localhost:60000/ISIS2</ISISURL>
        <ISISURL/>
        <ISISURL/>
        <ISISURL/>
        <HeartbeatTimeout>60</HeartbeatTimeout>
        <CheckPeriod>20</CheckPeriod>
        <ClientSSLConfig>
            <KeyPath>/etc/arc-security/hostkey.pem</KeyPath>
            <CertificatePath>/etc/arc-security/hostcert.pem</CertificatePath>
            <CACertificatesDir>/etc/arc-security/certificates</CACertificatesDir>
        </ClientSSLConfig>
    </Service>

    In [24]: get_child_values_by_name(x,'ISISURL')
    Out[24]: ['https://localhost:60000/ISIS1', 'https://localhost:60000/ISIS2']    
    """
    return [str(child) for child in get_child_nodes(node) if child.Name() == name and len(str(child)) > 0]  

# for DataHandle and DataPoint

def datapoint_from_url(url_string, ssl_config = None):
    import arc
    user_config = arc.UserConfig.CreateEmpty()
    if ssl_config:
        if ssl_config.has_key('key_file'): user_config.KeyPath(ssl_config['key_file'])
        if ssl_config.has_key('cert_file'): user_config.CertificatePath(ssl_config['cert_file'])
        if ssl_config.has_key('proxy_file'): user_config.ProxyPath(ssl_config['proxy_file'])
        if ssl_config.has_key('ca_file'): user_config.CACertificatePath(ssl_config['ca_file'])
        if ssl_config.has_key('ca_dir'): user_config.CACertificatesDirectory(ssl_config['ca_dir'])
    url = arc.URL(url_string)
    handle = arc.DataHandle(url, user_config)
    point = handle.__ref__()
    # possible memory leaks - TODO: investigate
    url.thisown = False
    user_config.thisown = False
    handle.thisown = False
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
