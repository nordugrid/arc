from storage.common import rbyteio_uri, byteio_simple_uri, mkuid, create_checksum
from storage.client import NotifyClient
import traceback
import arc
import base64
import os

class ByteIOBackend:

    public_request_names = ['notify']
    supported_protocols = ['byteio']

    def __init__(self, backendcfg, ns_uri, changeState):
        self.changeState = changeState
        self.ns = arc.NS({'se' : ns_uri})
        self.datadir = str(backendcfg.Get('DataDir'))
        self.transferdir = str(backendcfg.Get('TransferDir'))
        self.turlprefix = str(backendcfg.Get('TURLPrefix'))
        if not os.path.exists(self.datadir):
            os.mkdir(self.datadir)
        print "ByteIOBackend datadir:", self.datadir
        if not os.path.exists(self.transferdir):
            os.mkdir(self.transferdir)
        else:
            for filename in os.listdir(self.transferdir):
                os.remove(os.path.join(self.transferdir, filename))
        print "ByteIOBackend transferdir:", self.transferdir
        self.idstore = {}

    def prepareToGet(self, referenceID, localID, protocol):
        if protocol not in self.supported_protocols:
            raise Exception, 'Unsupported protocol: ' + protocol
        turl_id = mkuid()
        try:
            os.link(os.path.join(self.datadir, localID), os.path.join(self.transferdir, turl_id))
            self.idstore[turl_id] = referenceID
            print '++', self.idstore
            turl = self.turlprefix + turl_id
            return turl
        except:
            return None

    def prepareToPut(self, referenceID, localID, protocol):
        if protocol not in self.supported_protocols:
            raise Exception, 'Unsupported protocol: ' + protocol
        turl_id = mkuid()
        datapath = os.path.join(self.datadir, localID)
        f = file(datapath, 'w')
        f.close()
        os.link(datapath, os.path.join(self.transferdir, turl_id))
        self.idstore[turl_id] = referenceID
        print '++', self.idstore
        turl = self.turlprefix + turl_id
        return turl

    def list(self):
        return os.listdir(os.datadir)

    def getAvailableSpace(self):
        return None

    def generateLocalID(self):
        return mkuid()

    def matchProtocols(self, protocols):
        return [protocol for protocol in protocols if protocol in self.supported_protocols]

    def notify(self, inpayload):
        request_node = inpayload.Child()
        subject = str(request_node.Get('subject'))
        print 'idstore[%s] = %s' % (subject, self.idstore.get(subject, None))
        state = str(request_node.Get('state'))
        path = os.path.join(self.transferdir, subject)
        print 'Removing', path
        os.remove(path)
        out = arc.PayloadSOAP(self.ns)
        response_node = out.NewChild('se:notifyResponse').Set('OK')
        return out

    def checksum(self, localID, checksumType):
        return create_checksum(file(os.path.join(self.datadir, localID), 'rb'), checksumType)

class ByteIOService:

    def __init__(self, cfg):
        print "ByteIOService constructor called"
        self.rb_ns = arc.NS({'rb' : rbyteio_uri})
        self.transferdir = str(cfg.Get('TransferDir'))
        print "ByteIOService transfer dir:", self.transferdir
        self.notify = NotifyClient(str(cfg.Get('NotifyURL')))

    def _filename(self, subject):
        return os.path.join(self.transferdir, subject)

    def write(self, inpayload, subject):
        request_node = inpayload.Child()
        transfer_node = request_node.Get('transfer-information')
        if str(transfer_node.Attribute(0)) != byteio_simple_uri:
            raise Exception, 'transfer-mechanism not supported'
        try:
            fn = self._filename(subject)
            file(fn) # check existance
            f = file(fn,'w') # open for overwriting
        except:
            print traceback.format_exc()
            raise Exception, 'denied'
        encoded_data = str(transfer_node)
        data = base64.b64decode(encoded_data)
        try:
            f.write(data)
            f.close()
        except:
            print traceback.format_exc()
            raise Exception, 'write failed'
        self.notify.notify(subject, 'received')
        out = arc.PayloadSOAP(self.rb_ns)
        response_node = out.NewChild('rb:writeResponse').Set('OK')
        return out

    def read(self, inpayload, subject):
        try:
            data = file(self._filename(subject)).read()
        except:
            print traceback.format_exc()
            data = ''
        self.notify.notify(subject, 'sent')
        out = arc.PayloadSOAP(self.rb_ns)
        response_node = out.NewChild('rb:readResponse')
        transfer_node = response_node.NewChild('rb:transfer-information')
        transfer_node.NewAttribute('transfer-mechanism').Set(byteio_simple_uri)
        encoded_data = base64.b64encode(data)
        transfer_node.Set(encoded_data)
        return out

    def process(self, inmsg, outmsg):
        print "Process called"
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        # gets the last part of the request url
        # TODO somehow detect if this is just the path of the service which means: no subject
        subject = inmsg.Attributes().get('ENDPOINT').split('/')[-1]
        print 'Subject:', subject
        try:
            # gets the namespace prefix of the Hash namespace in its incoming payload
            rbyteio_prefix = inpayload.NamespacePrefix(rbyteio_uri)
            request_node = inpayload.Child()
            if request_node.Prefix() != rbyteio_prefix:
                raise Exception, 'wrong namespace (%s)' % request_name
            # get the name of the request without the namespace prefix
            request_name = request_node.Name()
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload and the subject
            # and which will return the response payload
            outpayload = getattr(self,request_name)(inpayload, subject)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            exc = traceback.format_exc()
            print exc
            # set an empty outgoing payload
            outpayload = arc.PayloadSOAP(arc.NS({}))
            outpayload.NewChild('Fault').Set(exc)
            outmsg.Payload(outpayload)
            # return with the status GENERIC_ERROR
            return arc.MCC_Status(arc.STATUS_OK)

    # names of provided methods
    request_names = ['read', 'write']

