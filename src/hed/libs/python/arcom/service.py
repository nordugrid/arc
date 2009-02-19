# namespace URIs of the storage services
ahash_uri = 'http://www.nordugrid.org/schemas/ahash'
librarian_uri = 'http://www.nordugrid.org/schemas/librarian'
bartender_uri = 'http://www.nordugrid.org/schemas/bartender'
shepherd_uri = 'http://www.nordugrid.org/schemas/shepherd'
gateway_uri = 'http://www.nordugrid.org/schemas/gateway'
delegation_uri = 'http://www.nordugrid.org/schemas/delegation'
rbyteio_uri = 'http://schemas.ggf.org/byteio/2005/10/random-access'

# URI for the simple transfer mechanism of ByteIO
byteio_simple_uri = 'http://schemas.ggf.org/byteio/2005/10/transfer-mechanisms/simple'
# True and False values used in the XML representation
true = '1'
false = '0'
# the defaults for TrustManager
default_checking_interval = 600
default_ahash_id = '3'

import arc
import inspect
import time
import sys
import threading
import random
from arcom import get_child_nodes
from arcom.security import AuthRequest, parse_ssl_config
from arcom.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Service'))

class Service:
    
    def __init__(self, request_config, cfg = None):
        self._trust_manager = []
        self.ssl_config = {}
        if cfg:
            self.ssl_config = parse_ssl_config(cfg)
            trust_manager_node = cfg.Get('TrustManager')
            fromFile = str(trust_manager_node.Attribute('FromFile'))
            if fromFile:
                try:
                    xml_string = file(fromFile).read()
                    trust_manager_node = arc.XMLNode(xml_string)
                except:
                    log.msg()
                    pass
            self._force_trust = str(trust_manager_node.Attribute('Force')) not in ['no', 'No', 'NO']
            entries = get_child_nodes(trust_manager_node)
            for entry in entries:
                name = entry.Name()
                if name in ['DN', 'CA']:
                    self._trust_manager.append({'type': name, 'DNs': [str(entry)]})
                if name in ['DNsFromAHash']:
                    try:
                        checking_interval = int(str(entry.Attribute('CheckingInterval')))
                    except:
                        checking_interval = None
                    if not checking_interval:
                        checking_interval = default_checking_interval
                    ahash_id = str(entry.Attribute('ID'))
                    if not ahash_id:
                        ahash_id = default_ahash_id
                    ahashes = get_child_nodes(entry)
                    ahash_urls = []
                    for ahash in ahashes:
                        if ahash.Name() == 'AHashURL':
                            ahash_urls.append(str(ahash))
                    data = {'type': name, 'DNs' : [], 'URLs' : ahash_urls,
                        'checking_interval' : checking_interval, 'ahash_id' : ahash_id}
                    self._trust_manager.append(data)
                    threading.Thread(target = self._get_dns_from_ahash, args = [data]).start()
        if not hasattr(self,'service_name'):
            self.service_name = 'Python Service With No Name'
        #if self._trust_manager:
        #    print self.service_name, "TrustManager:", self._force_trust and 'force' or 'don\'t force', self._trust_manager
        log.msg(arc.DEBUG, self.service_name, "constructor called")
        self.request_config = request_config
        self.ns = arc.NS(dict([(request_type['namespace_prefix'], request_type['namespace_uri'])
            for request_type in self.request_config]))
        
    def _get_dns_from_ahash(self, data):
        try:
            from storage.client import AHashClient
        except:
            log.msg()
        # first just wait a few seconds
        time.sleep(10)
        while True:
            try:
                #print "Start getting a list of DNs from an AHash"
                ahash_url = random.choice(data['URLs'])
                #print "Chosen AHash:", ahash_url
                ahash = AHashClient(ahash_url, ssl_config = self.ssl_config)
                results = ahash.get([data['ahash_id']])[data['ahash_id']]
                data['DNs'] = [DN for (_, DN) in results.keys()]
                #print "data", data
                #print "Done, waiting for %d seconds" % data['checking_interval']
                time.sleep(data['checking_interval'])
            except:
                log.msg()
                time.sleep(1)
    
    def _is_trusted(self, DN, CA):
        if not self._trust_manager:
            return True
        #print '_is_trusted called with', DN, CA
        trusted = False
        for entry in self._trust_manager:
            if entry['type'] == 'DN':
                if DN in entry['DNs']:
                    #print DN, 'is listed as trusted'
                    trusted = True
            if entry['type'] == 'CA':
                if CA in entry['DNs']:
                    #print DN, 'has a CA which is listed as trusted'
                    trusted = True
            if entry['type'] == 'DNsFromAHash':
                if DN in entry['DNs']:
                    #print DN, 'is listed as trusted in these AHashes:', ', '.join(entry['URLs'])
                    trusted = True
        return trusted
    
    def _new_soap_payload(self):
        return arc.PayloadSOAP(self.ns)
    
    def _call_request(self, request_name, inmsg):
        inpayload = inmsg.Payload()
        auth = AuthRequest(inmsg)
        inpayload.auth = auth
        if self._force_trust and not self._is_trusted(*auth.get_identity_and_ca()):
            raise Exception, 'client is not trusted'
        return getattr(self,request_name)(inpayload)
    
    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        
        try:
            # the first child of the payload should be the name of the request
            request_node = inpayload.Child()
            # get the namespace of the request node
            request_namespace = request_node.Namespace()
            matched_request_types = [request_type for request_type in self.request_config if request_type['namespace_uri'] == request_namespace]
            if len(matched_request_types) == 0:
                raise Exception, 'wrong namespace. expected: %s' % ', '.join([request_type['namespace_uri'] for request_type in self.request_config])
            current_request_type = matched_request_types[0]
            # get the name of the request without the namespace prefix
            request_name = request_node.Name()
            if request_name not in current_request_type['request_names']:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            log.msg(arc.DEBUG,'%s.%s called' % (self.service_name, request_name))
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            log.msg(arc.VERBOSE, inpayload.GetXML())
            outpayload = self._call_request(request_name, inmsg)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            msg = log.msg()
            # TODO: need proper fault message
            outpayload = arc.PayloadSOAP(self.ns)
            outpayload.NewChild('Fault').Set(msg)
            outmsg.Payload(outpayload)
            # TODO: return with the status GENERIC_ERROR
            return arc.MCC_Status(arc.STATUS_OK)

def parse_node(node, names, single = False, string = True):
    """ Call node_to_data() for each child of the given node.
    
    parse_node(node, names, single = False, string = True)
    
    node is the XMLNode whose children we want to convert
    names is a list of tag names which will be returned in the specified order
    single indicates that we need only one value beside the key, do not put it into a list
    string indicates that we need the string data of the nodes, not the nodes itself.
    
    Example:
    
        xml = XMLNode('''
            <statRequestList>
                <statRequestElement>
                    <requestID>0</requestID>
                    <LN>/</LN>
                </statRequestElement>
                <statRequestElement>
                    <requestID>1</requestID>
                    <LN>/testfile</LN>
                </statRequestElement>
            </statRequestList>
        ''')
      
    parse_node(xml, ['requestID','LN']) returns:
        
        {'0': ['/'], '1': ['/testfile']}


    parse_node(xml, ['requestID','LN'], single = True) returns:
        
        {'0': '/', '1': '/testfile'}


    parse_node(xml, ['LN','requestID'], True) returns:
        
        {'/': '0', '/testfile': '1'}


    parse_node(xml, ['requestID','LN','LN']) returns:
        
        {'0': ['/', '/'], '1': ['/testfile', '/testfile']}


    """
    return dict([
        node_to_data(n, names, single, string)
            for n in get_child_nodes(node)
    ])

def parse_to_dict(node, names):
    """ Convert the children of the node to a dictionary of dictionaries.
    
    parse_to_dict(node, names)
    
    node is the XMLNode whose children we want to convert
    names is a list of tag names, for each child only these names will be included in the dictionary
    
    Example:
    
        <statResponseList>
            <statResponseElement>
                <requestID>123</requestID>
                <referenceID>abdad</referenceID>
                <state>alive</state>
                <size>871432</size>
            </statResponseElement>
            <statResponseElement>
                <requestID>456</requestID>
                <referenceID>fefeg</referenceID>
                <state>alive</state>
                <size>945</size>
            </statResponseElement>
        </statResponseList>
        
    parse_to_dict(xml, ['requestID', 'state', 'size']) returns:

        {'123': {'size': '871432', 'state': 'alive'},
         '456': {'size': '945', 'state': 'alive'}}


    parse_to_dict(xml, ['referenceID','requestID', 'state', 'size']) returns:
    
        {'abdad': {'requestID': '123', 'size': '871432', 'state': 'alive'},
         'fefeg': {'requestID': '456', 'size': '945', 'state': 'alive'}}
    """
    return dict([(str(n.Get(names[0])), dict([(name, str(n.Get(name))) for name in names[1:]]))
        for n in get_child_nodes(node)])

def create_response(method_name, tag_names, elements, payload, single = False):
    """ Creates an XMLNode payload from a dictionary of tag names and list of values.
    
    create_response(method_name, tag_names, elements, payload, single = False)
    
    method_name is the name of the method which will be used as a prefix in the name of the 'Response' tag
    tag_names is a list of names which will be used in the specified order as tag names
    elements is a dictionary where the key will be tagged as the first tag name,
        and the value is a list whose items will be tagged in the order of the tag_names list
    payload is an XMLNode, the response will be added to that
    single indicates if there is only one value per key
    
    Example:
    
        elements = {'123': ['alive', '871432'], '456': ['alive', '945']}
        tag_names = ['requestID', 'state', 'size']
        method_name = 'stat'
        payload = arc.PayloadSOAP(arc.NS())

    after create_response(method_name, tag_names, elements, payload, single = False) payload will contain:
    
        <statResponse>
            <statResponseList>
                <statResponseElement>
                    <requestID>123</requestID>
                    <state>alive</state>
                    <size>871432</size>
                </statResponseElement>
                <statResponseElement>
                    <requestID>456</requestID>
                    <state>alive</state>
                    <size>945</size>
                </statResponseElement>
            </statResponseList>
        </statResponse>

    The method_name used to prefix the 'Response' the 'ResponseList' and 'ResponseElement' node names.
    We could say
        method_name = 'ns:stat'
        tag_names = ['ns:requestID', 'ns:state', 'ns:size']
    if we want namespace prefixes.
    """
    # first create an XMLTree, then add it to the payload XMLNode
    from arcom.xmltree import XMLTree
    if single:
        # if there is only a single value for each key
        tree = XMLTree(from_tree =
            (method_name + 'ResponseList', [
                (method_name + 'ResponseElement', [
                    (tag_names[0], key),
                    (tag_names[1], value)
                ]) for key, value in elements.items()
            ])
        )
    else:
        # if there is more values for a key
        tree = XMLTree(from_tree =
            (method_name + 'ResponseList', [
                (method_name + 'ResponseElement', [
                    (tag_names[0], key) # tag the key with the first item in tag_names
                ] + [ # for each item in the values list pick the next name from tag_names
                    (tag_names[i + 1], values[i]) for i in range(len(values))
                ]) for key, values in elements.items()
            ])
        )
    # create a <method_name>Response child node in the payload
    response_node = payload.NewChild(method_name + 'Response')
    # add the XMLTree to this newly created node
    tree.add_to_node(response_node)
    # return the payload XMLNode
    return payload

def node_to_data(node, names, single = False, string = True):
    """ Get some children of an XMLNode and return them in a list in the specified order using the first one as a key.
    
    node_to_data(node, names, single = False, string = True)
    
    node is an XMLNode which has some children
    names is a list of strings, the names of the children we want to extract, the first name always will be a key
    single is a boolean indicating if we want only a single value thus do not put it in a list
    string is a boolean indicating if we want the string values of the nodes or the nodes itself
    
    Example:
    
        node:
            <changeRequest>
                <changeID>0</changeID>
                <ID>123</ID>
                <section>states</section>
                <property>neededReplicas</property>
                <value>3</value>
                <somethingElse>not interesting</somethingElse>
                <changeType>set</changeType>
            </changeRequest>
            
        names: ['changeID', 'ID', 'changeType', 'section', 'property', 'value']
        
        here changeID will be the key, and all the other names will be in a list in the specified order
        
        so it returns ('0', ['123', 'set', 'states', 'neededReplicas', '3'])
        
            ('somethingElse' is not returned)
    
    Example:
    
        node:
            <getRequest>
                <GUID>11</GUID>
                <requestID>99</requestID>
            </getRequest>
            
        names: ['requestID', 'GUID']
        single: True
        
        here requestID will be the key, and GUID is the single value which won't be in a list
        
        so it returns ('99', '11')
        
            (instead of '99', ['11'])
    """
    if string:
        # if we need the strings
        # for each name get the string data of the child with that name,
        data = [str(node.Get(name)) for name in names]
    else:
        # for each name get the child node itself
        data = [node.Get(name) for name in names]
    if single:
        # return the first item as a key and the second item as a single value
        return data[0], data[1]
    else:
        # return the first item as a key, and all the rest items as a list
        return data[0], data[1:]

def get_data_node(node):
    return node.Get('Body').Child().Child()

