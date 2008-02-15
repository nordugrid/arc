import urlparse
import httplib
import arc
import random
from storage.xmltree import XMLTree
import traceback

catalog_uri = 'urn:scatalog'
manager_uri = 'urn:smanager'
global_root_guid= '0'

def mkuid():
    rnd = ''.join([ chr(random.randint(0,255)) for i in range(16) ])
    rnd = rnd[:6] + chr((ord(rnd[6]) & 0x4F) | 0x40) + rnd[7:8]  + chr((ord(rnd[8]) & 0xBF) | 0x80) + rnd[9:]
    uuid = 0L
    for i in range(16):
        uuid = (uuid << 8) + ord(rnd[i])
    rnd_str = '%032x' % uuid
    return '%s-%s-%s-%s-%s' % (rnd_str[0:8], rnd_str[8:12], rnd_str[12:16],  rnd_str[16:20], rnd_str[20:])

class CatalogClient:
    
    def __init__(self, url):
        (_, host_port, path, _, _, _) = urlparse.urlparse(url)
        if ':' in host_port:
            host, port = host_port.split(':')
        else:
            host = host_port
            port = 80
        self._host = host
        self._port = port
        self._path = path
        self._ns = arc.NS({'cat':catalog_uri})

    def get(self, IDs):
        tree = XMLTree(from_tree =
            ('cat:get', [
                ('cat:getRequestList', [
                    ('cat:getRequestElement', [
                        ('cat:GUID', i)
                    ]) for i in IDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        list_node = xml.Child().Child().Child()
        list_number = list_node.Size()
        elements = []
        for i in range(list_number):
            element_node = list_node.Child(i)
            GUID = str(element_node.Get('GUID'))
            metadatalist_node = element_node.Get('metadataList')
            metadatalist_number = metadatalist_node.Size()
            metadata = []
            for j in range(metadatalist_number):
                metadata_node = metadatalist_node.Child(j)
                metadata.append((
                    (str(metadata_node.Get('section')),str(metadata_node.Get('property'))),
                        str(metadata_node.Get('value'))))
            elements.append((GUID,dict(metadata)))
        return dict(elements)

    def call(self, tree, return_tree_only = False):
        out = arc.PayloadSOAP(self._ns)
        tree.add_to_node(out)
        msg = out.GetXML()
        h = httplib.HTTPConnection(self._host, self._port)
        h.request('POST', self._path, msg)
        r = h.getresponse()
        if return_tree_only:
            resp = r.read()
            return XMLTree(from_string=resp).get_trees('///')[0]
        else:
            return r.read(), r.status, r.reason

class Manager:
    def __init__(self, catalog):
        self.catalog = catalog

    def _parse_LN(self, LN):
        try:
            splitted= LN.split('/')
        except:
            raise Exception, 'Invalid Logical Name: `%s`' % LN
        guid = splitted[0]
        path = splitted[1:]
        if not guid:
            guid = global_root_guid
        return guid, path

    def stat(self, requests):
        for LN in requests:
            guid, path = self._parse_LN(LN)
            print '\n\n\n*******', guid, path
        return self.catalog.get(requests)
        

class ManagerService:

    # names of provided methods
    request_names = ['stat']

    def __init__(self, cfg):
        print "Storage Manager service constructor called"
        catalog_url = str(cfg.Get('CatalogURL'))
        catalog = CatalogClient(catalog_url)
        self.manager = Manager(catalog)
        self.man_ns = arc.NS({'man':manager_uri})
    
    def stat(self, inpayload):
        requests_node = inpayload.Child().Child()
        request_number = requests_node.Size()
        requests = []
        for i in range(request_number):
            request_node = requests_node.Child(i)
            requests.append(str(request_node.Get('LN')))
        print requests
        response = self.manager.stat(requests)
        out = arc.PayloadSOAP(self.man_ns)
        response_node = out.NewChild('man:statResponse')
        response_node.Set(str(response))
        return out

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        print "Process called"
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the manager namespace in its incoming payload
            manager_prefix = inpayload.NamespacePrefix(manager_uri)
            # gets the namespace prefix of the request
            request_prefix = inpayload.Child().Prefix()
            if request_prefix != manager_prefix:
                # if the request is not in the manager namespace
                raise Exception, 'wrong namespace (%s)' % request_prefix
            # get the name of the request without the namespace prefix
            request_name = inpayload.Child().Name()
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            outpayload = getattr(self,request_name)(inpayload)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            exc = traceback.format_exc()
            print exc
            # set an empty outgoing payload
            outpayload = arc.PayloadSOAP(self.man_ns)
            outpayload.NewChild('manager:Fault').Set(exc)
            outmsg.Payload(outpayload)
            return arc.MCC_Status(arc.STATUS_OK)
