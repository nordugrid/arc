from storage.common import rbyteio_uri, byteio_simple_uri, mkuid, create_checksum
from storage.client import NotifyClient, ByteIOClient
import traceback
import arc
import base64
import os

class ByteIOBackend:

    public_request_names = ['notify']
    supported_protocols = ['byteio']

    def __init__(self, backendcfg, ns_uri, file_arrived):
        self.file_arrived = file_arrived
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

    def copyTo(self, localID, turl, protocol):
        f = file(os.path.join(self.datadir, localID),'rb')
        print 'Uploading file to', turl
        ByteIOClient(turl).write(f)
        f.close()
    
    def copyFrom(self, localID, turl, protocol):
        # TODO: download to a separate file, and if checksum OK, then copy the file 
        f = file(os.path.join(self.datadir, localID), 'wb')
        print 'Downloading file from', turl
        ByteIOClient(turl).read(file = f)
        f.close()

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
        f = file(datapath, 'wb')
        f.close()
        os.link(datapath, os.path.join(self.transferdir, turl_id))
        self.idstore[turl_id] = referenceID
        print '++', self.idstore
        turl = self.turlprefix + turl_id
        return turl

    def remove(self, localID):
        try:
            fn = os.path.join(self.datadir, localID)
            os.remove(fn)
        except:
            raise Exception, 'denied'
        return 'removed'

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
        referenceID = self.idstore.get(subject,None)
        state = str(request_node.Get('state'))
        path = os.path.join(self.transferdir, subject)
        print 'Removing', path
        os.remove(path)
        self.file_arrived(referenceID)
        out = arc.PayloadSOAP(self.ns)
        response_node = out.NewChild('se:notifyResponse').Set('OK')
        return out

    def checksum(self, localID, checksumType):
        return create_checksum(file(os.path.join(self.datadir, localID), 'rb'), checksumType)

from storage.service import Service

class ByteIOService(Service):

    def __init__(self, cfg):
        # names of provided methods
        request_names = ['read', 'write']
        # call the Service's constructor
        Service.__init__(self, 'ByteIO', request_names, 'rb', rbyteio_uri)
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
            f = file(fn,'wb') # open for overwriting
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
        out = self.newSOAPPayload()
        response_node = out.NewChild('rb:writeResponse').Set('OK')
        return out

    def read(self, inpayload, subject):
        try:
            data = file(self._filename(subject),'rb').read()
        except:
            print traceback.format_exc()
            data = ''
        self.notify.notify(subject, 'sent')
        out = self.newSOAPPayload()
        response_node = out.NewChild('rb:readResponse')
        transfer_node = response_node.NewChild('rb:transfer-information')
        transfer_node.NewAttribute('transfer-mechanism').Set(byteio_simple_uri)
        encoded_data = base64.b64encode(data)
        transfer_node.Set(encoded_data)
        return out


    def _call_request(self, request_name, inmsg):
        # gets the last part of the request url
        # TODO: somehow detect if this is just the path of the service which means: no subject
        subject = inmsg.Attributes().get('ENDPOINT').split('/')[-1]
        # the subject of the byteio request: reference to the file
        print 'Subject:', subject
        inpayload = inmsg.Payload()
        return getattr(self,request_name)(inpayload, subject)

