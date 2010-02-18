import arc

import sys, exceptions, errno, copy, time
import thread
import cPickle
from arcom.store.basestore import BaseStore

from arcom.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'arcom.TransDBStore'))

try:
    from bsddb3 import db
except:
    try:
        from bsddb import db
    except:
        log.msg(arc.FATAL, "Could not import module bsddb. This is required for TransDBStore.")
        raise Exception, "Could not import module bsddb. This is required for TransDBStore."

class TransConfigInfo:
    """
    Class containing info about the TransDBStore
    """
    def __init__(self):
        self.home = "TESTDIR"


class TransDBStore(BaseStore):
    
    def __init__(self, storecfg, non_existent_object = {}):
        """ Constructor of TransDBStore.

        TransDBStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """

        BaseStore.__init__(self, storecfg, non_existent_object)
        log.msg(arc.VERBOSE, "TransDBStore constructor called")
        log.msg(arc.VERBOSE, "datadir:", self.datadir)

        # db and transaction pointers
        self.dbp  = None
        
        self.database  = "store.db"
        self.dbenv = db.DBEnv(0)
        
        self.__configureDB(storecfg)
        self.dbenv.set_cachesize(0, self.cachesize, 0)
        # if key not found, raise DBNotFoundError:
        self.dbenv.set_get_returns_none(0) 

        self.__opendbenv()
        log.msg(arc.INFO, "db environment opened")
    
    def __del__(self):
        """
        exit gracefully, i.e., close dbp and dbenv
        """
        self.__err()
        self.terminate()

    def __configureDB(self, storecfg):
        try:
            self.dbenv_flags = int(str(storecfg.Get("DBEnvFlags")))
        except:
            self.dbenv_flags = db.DB_CREATE | \
                               db.DB_RECOVER | \
                               db.DB_INIT_LOCK | \
                               db.DB_INIT_LOG | \
                               db.DB_INIT_MPOOL | \
                               db.DB_INIT_TXN
        try:
            self.deadlock_retries = int(str(storecfg.Get('DeadlockRetries')))
        except:
            log.msg(arc.WARNING, "couldn't find DeadlockRetries, using 5 as default")
            self.deadlock_retries = 5
        try:
            self.cachesize = int(str(storecfg.Get('CacheSize')))
        except:
            self.cachesize = 10 * 1024 * 1024
            log.msg(arc.WARNING, "couldn't find CacheSize, using %d as default"%self.cachesize)
        try:
            self.cachesize = int(str(storecfg.Get('SleepTime')))
        except:
            self.sleeptime = 2
            log.msg(arc.WARNING, "couldn't find SleepTime, using %d as default"%self.cachesize)

    def __err(self):
        """
        close the db pointer dpb if not closed already
        """
        if self.dbp != None:
            self.dbp.close(db.DB_NOSYNC)
            log.msg(arc.INFO, "database closed")
            #log.msg()

    def __opendbenv(self):
        """
        open the db env
        """
        try:
            self.dbenv.open(self.datadir, self.dbenv_flags, 0)
        except db.DBError, msg:
            log.msg()
            log.msg(arc.ERROR, "Caught exception during DB environment open.")
            self.terminate()
            #raise db.DBError, msg
        
    def getDBFlags(self):
        # DB_AUTO_COMMIT ensures that all db modifications will
        # automatically be enclosed in transactions
        return db.DB_CREATE | db.DB_AUTO_COMMIT

    def getDBReady(self):
        """
        Dummy method always assuming db is ready
        """
        return True

    def __opendb(self, dbp):
        """
        Open the db using dbp as db handle
        """
        while dbp == None:
            dbp = db.DB(self.dbenv,0)
                        
            # todo: db performance can be fine-tuned with
            # proper pagesize, berkeley-db/db/ref/am_conf/pagesize.html
#            try:
#                dbp.set_pagesize(512)
#            except db.DBError, msg:
#                self.__err()
#                log.msg()
#                log.msg(arc.ERROR, "Error in dbp.set_pagesize")
#                self.terminate()
            try:
                # using linear hashing for db
                dbp.open(self.database, dbtype = db.DB_HASH, 
                         flags = self.getDBFlags())
            except db.DBError, (errnum, strerror): 
                # It is expected that this condition will be triggered when
                # client sites starts up.
                # It can take a while for the master site to be found and
                # synced, and no DB will be available until then
                if errnum == errno.ENOENT:
                    log.msg(arc.VERBOSE, "No stock db available yet - retrying")
                    log.msg()
                    try:
                        dbp.close(0)
                    except db.DBError, (errnum2, strerror2):
                        self.__del__()
                        
                        log.msg()
                        log.msg(arc.ERROR, "unexpected error closing after failed open")
                        sys.exit(1)
                        raise db.DBError, strerror2
                    dbp = None
                    time.sleep(self.sleeptime)
                    continue
                elif errnum == db.DB_LOCK_DEADLOCK:
                    log.msg(arc.VERBOSE, "got deadlock - retrying")
                    try:
                        dbp.close(0)
                    except db.DBError, (errnum2, strerror2):
                        log.msg()
                        log.msg(arc.ERROR, "unexpected error closing after failed open")
                        #self.__del__()
                        
                        raise db.DBError, strerror2
                    dbp = None
                    time.sleep(self.sleeptime)
                    continue
                else:
                    log.msg()
                    return None
                    #self.__del__()                    
            except:
                log.msg()
                return None
                #self.__del__()
                
        return dbp

    
    def list(self):
        """ List the IDs of the existing entries.
        
        list()
        
        Returns a list of IDs.
        """

        if not self.getDBReady():
            raise db.DBError, "db not ready"
        self.dbp = self.__opendb(self.dbp)
        try:
            object = self.dbp.keys()
            return object
        except db.DBLockDeadlockError:
            log.msg(arc.INFO, "Got deadlock error")            
            log.msg()
            raise db.DBError, "db deadlock"
        except db.DBError:
            self.__del__()
            log.msg()
            log.msg(arc.ERROR, "Error listing db")
            sys.exit(1)
            raise db.DBError, "Error listing db"

    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """

        if not self.getDBReady():
            raise db.DBError, "db not ready"

        self.dbp = self.__opendb(self.dbp)

        try:
            object = self.dbp.get(ID)
            try:
                # using cPickle.loads for loading
                object = cPickle.loads(object)
            except:
                log.msg()
            if not object:
                return copy.deepcopy(self.non_existent_object)
            return object
        #todo: handle all errors, e.g., deadlock and dead handle
        except db.DBNotFoundError:
            return copy.deepcopy(self.non_existent_object)
        except db.DBKeyEmptyError:
            return copy.deepcopy(self.non_existent_object)
        except db.DBLockDeadlockError:
            log.msg(arc.INFO, "Got deadlock error")            
            log.msg()
            raise db.DBError, "db deadlock"
        except db.DBRepHandleDeadError:
            log.msg(arc.INFO, "Got rep_dead_handle error")
            self.__err()
            self.dbp = None
            raise
        except db.DBError, msg:
            self.__del__()
            log.msg()
            log.msg(arc.ERROR, "Error getting %s"%ID)
            sys.exit(1)
            raise db.DBError, "Error listing db"
        
    def set(self, ID, object):
        """ Stores an object with the given ID..

        set(ID, object)

        'ID' is the ID of the object
        'object' is the object itself
        If there is already an object with this ID it will be overwritten completely.
        If deadlock is caught, retry DeadlockRetries times
        """
        if not ID:
            raise Exception, 'ID is empty'

        if not self.getDBReady():
            return
        self.dbp = self.__opendb(self.dbp)
        retry = True
        retry_count = 0

        # try deadlock_retries times if receiving catching DBLockDeadlockError
        while retry:
            try:
                if object == None:
                    self.dbp.delete(ID)
                else:
                    # note that object needs to be converted to string
                    # using cPickle.dumps for converting
                    self.dbp.put(ID, cPickle.dumps(object, cPickle.HIGHEST_PROTOCOL))
                retry = False
            except db.DBLockDeadlockError:
                log.msg(arc.INFO, "Got deadlock error")
                # need to close transaction handle as well
                time.sleep(0.2)
                if retry_count < self.deadlock_retries:
                    log.msg(arc.VERBOSE, "got DBLockDeadlockError")
                    retry_count += 1
                    log.msg(arc.VERBOSE, "retrying transaction", retry_count)
                    retry = True
                else:
                    log.msg(arc.VERBOSE, "Deadlock exception, giving up...")
                    retry = False
            except db.DBRepHandleDeadError:
                log.msg(arc.INFO, "Got rep_dead_handle error")            
                #log.msg()
                self.__err()
                self.dbp = None
                raise
            except db.DBAccessError:
                log.msg(arc.WARNING,"Read-only db. I'm not a master.")
                raise
            except db.DBNotFoundError:
                log.msg(arc.WARNING, "cannot delete non-existing entries")
                retry = False
            except:
                self.__del__()
                log.msg()
                log.msg(arc.ERROR, "Error setting %s"%ID)
                retry = False
                sys.exit(1)

    def restart(self):
        try:
            self.dbenv.close(0)
            log.msg(arc.INFO, "db environment closed")
            self.dbenv = db.DBEnv(0)
            self.__opendbenv()
        except db.DBError, msg:
            log.msg()
            log.msg(arc.ERROR, "error closing environment")
            #raise db.DBError, msg

    def terminate(self):
        try:
            self.dbenv.close(0)
            log.msg(arc.INFO, "db environment closed")
        except db.DBError, msg:
            log.msg()
            log.msg(arc.ERROR, "error closing environment")
            #raise db.DBError, msg
