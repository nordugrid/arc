import arc
import os, copy, threading, time
from ZODB import FileStorage, DB
from persistent import Persistent

from storage.store.basestore import BaseStore

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.ZODBStore'))

class Metadata(Persistent):
    """
    Class for persistent mutable metadata
    """
    def __init__(self): 
        self.meta={}
    
    def addMeta(self, ID, object):
        self.meta[ID]=object
        self._p_changed = 1
        
    def delMeta(self, ID):
        del(self.meta[ID])
        self._p_changed = 1
        
    def __getitem__(self, ID):
        return self.meta[ID]
    
    def list(self):
        return self.meta.keys()

class ZODBStore(BaseStore):

    def __init__(self, storecfg, non_existent_object = {}):
        """ Constructor of ZODBStore.

        ZODBStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """
        BaseStore.__init__(self, storecfg, non_existent_object)
        log.msg(arc.DEBUG, "ZODBStore constructor called")
        log.msg(arc.DEBUG, "datadir:", self.datadir)
        self.dbfile = os.path.join(self.datadir,'metadata.fs')
        if os.path.isfile(self.dbfile):
            self.db = DB(FileStorage.FileStorage(self.dbfile))
            self.db.setCacheSize(4000)
            # decrease db file size in case of previous deletions 
            # (should maybe do this in a separate script?)
            #db.pack()
            self.metadata = self.db.open().root()['meta']
        else:
            self.db = DB(FileStorage.FileStorage(self.dbfile))
            root = self.db.open().root()
            root['meta'] = Metadata()
            get_transaction().commit()
            self.metadata = root['meta']
        self.period = 60
        threading.Thread(target = self.packingThread, args = [self.period]).start()

    def packingThread(self,period):
        time.sleep(7)
        while True:
            self.db.pack()
            time.sleep(period)
            
    def list(self):
        """ List the IDs of the existing entries.
        
        list()
        
        Returns a list of IDs.
        Only relevant for shepherd
        """
        #get_transaction().commit()
        return self.metadata.list()

    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """
        try:
            #get_transaction().commit()
            return self.metadata[ID]
        except KeyError:
            # don't print 'KeyError' if there is no such ID
            pass
        except:
            # print whatever exception happened
            log.msg()
            log.msg(arc.ERROR, "ID", ID)
        # if there was an exception, return the given non_existent_object
        return copy.deepcopy(self.non_existent_object)

    def set(self, ID, object):
        """ Stores an object with the given ID..

        set(ID, object)

        'ID' is the ID of the object
        'object' is the object itself
        If there is already an object with this ID it will be overwritten completely.
        """
        if not ID:
            raise Exception, 'ID is empty'
        try:
            if object == None:
                self.metadata.delMeta(ID)
            else:
                self.metadata.addMeta(ID, object)
            get_transaction().commit()
        except:
            log.msg()
