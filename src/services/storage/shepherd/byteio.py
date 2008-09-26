from storage.common import rbyteio_uri, byteio_simple_uri, mkuid, create_checksum, upload_to_turl, download_from_turl
from storage.client import NotifyClient, ByteIOClient
import traceback
import arc
import base64
import os

class ByteIOBackend:

    public_request_names = ['notify']
    supported_protocols = ['byteio']

    def __init__(self, backendcfg, ns_uri, file_arrived, log):
        self.log = log
        self.file_arrived = file_arrived
        self.ns = arc.NS('she', ns_uri)
        self.datadir = str(backendcfg.Get('DataDir'))
        self.transferdir = str(backendcfg.Get('TransferDir'))
        self.turlprefix = str(backendcfg.Get('TURLPrefix'))
        if not os.path.exists(self.datadir):
            os.mkdir(self.datadir)
        self.log.msg(arc.DEBUG, "ByteIOBackend datadir:", self.datadir)
        if not os.path.exists(self.transferdir):
            os.mkdir(self.transferdir)
        else:
            for filename in os.listdir(self.transferdir):
                os.remove(os.path.join(self.transferdir, filename))
        self.log.msg(arc.DEBUG, "ByteIOBackend transferdir:", self.transferdir)
        self.idstore = {}

    def copyTo(self, localID, turl, protocol):
        f = file(os.path.join(self.datadir, localID),'rb')
        self.log.msg(arc.DEBUG, self.turlprefix, 'Uploading file to', turl)
        upload_to_turl(turl, protocol, f)
        f.close()
    
    def copyFrom(self, localID, turl, protocol):
        # TODO: download to a separate file, and if checksum OK, then copy the file 
        f = file(os.path.join(self.datadir, localID), 'wb')
        self.log.msg(arc.DEBUG, self.turlprefix, 'Downloading file from', turl)
        download_from_turl(turl, protocol, f)
        f.close()

    def prepareToGet(self, referenceID, localID, protocol):
        if protocol not in self.supported_protocols:
            raise Exception, 'Unsupported protocol: ' + protocol
        turl_id = mkuid()
        try:
            os.link(os.path.join(self.datadir, localID), os.path.join(self.transferdir, turl_id))
            self.idstore[turl_id] = referenceID
            self.log.msg(arc.DEBUG, self.turlprefix, '++', self.idstore)
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
        self.log.msg(arc.DEBUG, self.turlprefix, '++', self.idstore)
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
        request_node = inpayload.Get('Body')
        subject = str(request_node.Get('subject'))
        referenceID = self.idstore.get(subject,None)
        state = str(request_node.Get('state'))
        path = os.path.join(self.transferdir, subject)
        self.log.msg(arc.DEBUG, self.turlprefix, 'Removing', path)
        os.remove(path)
        self.file_arrived(referenceID)
        out = arc.PayloadSOAP(self.ns)
        response_node = out.NewChild('she:notifyResponse').Set('OK')
        return out

    def checksum(self, localID, checksumType):
        return create_checksum(file(os.path.join(self.datadir, localID), 'rb'), checksumType)

from storage.service import Service
from storage.logger import Logger

class ByteIOService(Service):

    def __init__(self, cfg):
        self.service_name = 'ByteIO'
        self.log_level = str(cfg.Get('LogLevel'))
        if self.log_level:
            self.log_level = eval('arc.'+self.log_level)
        # init logging
        self.log = Logger(self.service_name, self.log_level)
        # names of provided methods
        request_names = ['read', 'write']
        # call the Service's constructor
        Service.__init__(self, 'ByteIO', request_names, 'rb', rbyteio_uri, self.log, cfg)
        self.transferdir = str(cfg.Get('TransferDir'))
        self.log.msg(arc.DEBUG, "ByteIOService transfer dir:", self.transferdir)
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
            self.log.msg()
            raise Exception, 'denied'
        encoded_data = str(transfer_node)
        data = base64.b64decode(encoded_data)
        try:
            f.write(data)
            f.close()
        except:
            self.log.msg()
            raise Exception, 'write failed'
        self.notify.notify(subject, 'received')
        out = self.newSOAPPayload()
        response_node = out.NewChild('rb:writeResponse').Set('OK')
        return out

    def read(self, inpayload, subject):
        try:
            data = file(self._filename(subject),'rb').read()
        except:
            self.log.msg()
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
        self.log.msg(arc.DEBUG, 'Subject:', subject)
        inpayload = inmsg.Payload()
        return getattr(self,request_name)(inpayload, subject)

