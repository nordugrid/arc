from storage.common import mkuid, create_checksum, upload_to_turl, download_from_turl
import traceback
import arc
import base64
import os
import threading
import time
import stat


class HardlinkingBackend:
    """
    Superclass for http protocols using hardlinking scheme.
    Don't use this class directly. Use subclasses instead.
    """
    public_request_names = []
    supported_protocols = ['http']

    def __init__(self, backendcfg, ns_uri, file_arrived, log, ssl_config):
        """docstring for __init__"""
        self.ssl_config = ssl_config
        self.log = log
        self.file_arrived = file_arrived
        #self.ns = arc.NS({'she' : ns_uri})
        self.datadir = str(backendcfg.Get('DataDir'))
        self.transferdir = str(backendcfg.Get('TransferDir'))
        # TODO: add '/' at the end of TURLPrefix if needed
        self.turlprefix = str(backendcfg.Get('TURLPrefix'))
        if not os.path.exists(self.datadir):
            os.mkdir(self.datadir)
        if not os.path.exists(self.transferdir):
            os.mkdir(self.transferdir)
        else:
            for filename in os.listdir(self.transferdir):
                if not ".py" in filename: # need this for apache backend
                    os.remove(os.path.join(self.transferdir, filename))
        self.idstore = {}
        threading.Thread(target = self.checkingThread, args = [5]).start()        
    
    def checkingThread(self, period):
        """docstring for checkingThread"""
        while True:
            try:
                time.sleep(period)
                for localID, referenceID in self.idstore.items():
                    filename = os.path.join(self.datadir, localID)
                    try:
                        nlink = os.stat(filename)[stat.ST_NLINK]
                    except:
                        # if the file does not exist, maybe it's already removed
                        del self.idstore[localID]
                        nlink = 0
                    self.log('DEBUG', 'checking', localID, referenceID, nlink)
                    if nlink == 1:
                        # if there is just one link for this file, it is already removed from the transfer dir
                        self.file_arrived(referenceID)
                        del self.idstore[localID]
            except:
                self.log()
    
        

    def copyTo(self, localID, turl, protocol):
        f = file(os.path.join(self.datadir, localID),'rb')
        self.log('DEBUG', self.turlprefix, 'Uploading file to', turl)
        upload_to_turl(turl, protocol, f, ssl_config = self.ssl_config)
        f.close()
    
    def copyFrom(self, localID, turl, protocol):
        # TODO: download to a separate file, and if checksum OK, then copy the file 
        f = file(os.path.join(self.datadir, localID), 'wb')
        self.log('DEBUG', self.turlprefix, 'Downloading file from', turl)
        download_from_turl(turl, protocol, f, ssl_config = self.ssl_config)
        f.close()

    def prepareToGet(self, referenceID, localID, protocol):
        if protocol not in self.supported_protocols:
            raise Exception, 'Unsupported protocol: ' + protocol
        turl_id = mkuid()
        try:
            filepath = os.path.join(self.datadir, localID)
            # set it to readonly
            os.chmod(filepath, 0444)
            os.link(filepath, os.path.join(self.transferdir, turl_id))
            self.log('DEBUG', self.turlprefix, '++', self.idstore)
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
        self.idstore[localID] = referenceID
        self.log('DEBUG', self.turlprefix, '++', self.idstore)
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

    def checksum(self, localID, checksumType):
        return create_checksum(file(os.path.join(self.datadir, localID), 'rb'), checksumType)


class HopiBackend( HardlinkingBackend ):
    """
    Backend for Hopi with hardlinking
    """

    def __init__(self, *args):
        HardlinkingBackend.__init__(self, *args)
        self.log('DEBUG', "HopiBackend datadir:", self.datadir)
        self.log('DEBUG', "HopiBackend transferdir:", self.transferdir)        


import pwd

class ApacheBackend( HardlinkingBackend ):
    """
    Backend for Apache with hardlinking
    """

    def __init__(self, backendcfg, ns_uri, file_arrived, log, ssl_config):
        HardlinkingBackend.__init__(self, backendcfg, ns_uri, file_arrived, log, ssl_config)
        self.apacheuser = str(backendcfg.Get('ApacheUser'))
        self.uid = os.getuid()
        _, _, _, self.apachegid, _, _, _ = pwd.getpwnam(self.apacheuser) 
        self.log('DEBUG', "ApacheBackend datadir:", self.datadir)
        self.log('DEBUG', "ApacheBackend transferdir:", self.transferdir)
        

    def prepareToPut(self, referenceID, localID, protocol):
        turl = HardlinkingBackend.prepareToPut(self, referenceID, localID, protocol)
        datapath = os.path.join(self.datadir, localID)
        # set group to apache to make it readable to Apache
        os.chown(datapath, self.uid, self.apachegid)
        # set file group writable
        os.chmod(datapath, 0664)
        return turl
