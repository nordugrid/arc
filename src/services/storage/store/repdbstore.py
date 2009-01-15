import arc

import sys, exceptions, errno, copy, time
import threading

from storage.store.transdbstore import TransDBStore

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.RepDBStore'))

from bsddb3 import db

CACHESIZE = 10 * 1024 * 1024
DATABASE  = "repdb.db"
SLEEPTIME = 3

class HostInfo:
    """
    Class for storing info on a RepDB host
    """

    def __init__(self, host=None, port=0):
        self.host = host
        self.port = port

class RepConfigInfo:
    """
    Class containing info about the RepDBStore
    """
    def __init__(self):
        self.this_host = HostInfo()
        self.start_policy = db.DB_REP_ELECTION
        self.home = "TESTDIR"
        self.got_listen_adress = False
        self.totalsites = 0
        self.priority = 100
        self.other_hosts = []

    def addOtherHost(self, host, port):
        self.other_hosts += [HostInfo(host=host, port=port)]
        

class RepData:
    def __init__(self):
        self.is_master = False

class RepDBStore(TransDBStore):
    
    def __init__(self, storecfg, non_existent_object = {}):
        """ Constructor of RepDBStore.

        RepDBStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir', 'StoreHost', 'TotalSites',
        'OtherHost' and 'Priority'
        'non_existent_object' will be returned if an object not found
        """

        self.app_data = RepData()

        if not hasattr(self, "dbenv_flags"):            
            # dbenv_flags could already have been set in some child class
            self.dbenv_flags = db.DB_CREATE | \
                               db.DB_RECOVER | \
                               db.DB_THREAD | \
                               db.DB_INIT_REP | \
                               db.DB_INIT_LOCK | \
                               db.DB_INIT_LOG | \
                               db.DB_INIT_MPOOL | \
                               db.DB_INIT_TXN
        if not hasattr(self, "db_flags"):            
            # only master can create db
            # DB_AUTO_COMMIT ensures that all db modifications will
            # automatically be enclosed in transactions
            self.db_flags = (self.app_data.is_master and 
                             db.DB_CREATE | db.DB_AUTO_COMMIT or 
                             db.DB_AUTO_COMMIT)
        if not hasattr(self, "dbenv"):
            self.dbenv = db.DBEnv(0)

        # configure replication prior to initializing TransDBStore
        self.__configureReplication(storecfg)

        # db environment is opened in TransDBStore.__init__
        TransDBStore.__init__(self, storecfg, non_existent_object)

        log.msg(arc.DEBUG, "RepDBStore constructor called")
        log.msg(arc.DEBUG, "datadir:", self.datadir)

        # start replication manager
        try:
            # start repmgr with 3 threads and always do election of master
            self.dbenv.repmgr_start(3, db.DB_REP_ELECTION)
        except:
            log.msg()
            log.msg(arc.ERROR, "Couldn't start replication manager.")
            self.terminate()

        # thread for maintaining host list
        threading.Thread(target = self.checkingThread, args=[self.app_config.check_period]).start()

    def getDBFlags(self):
            # only master can create db
            # DB_AUTO_COMMIT ensures that all db modifications will
            # automatically be enclosed in transactions
            return (self.app_data.is_master and 
                    db.DB_CREATE | db.DB_AUTO_COMMIT or 
                    db.DB_AUTO_COMMIT)

    def checkingThread(self, period):
        time.sleep(10)

        ID = 'rephosts' # GUID of the replication host list in the DB

        while True:
            # self.site_list is set in event_callback when elected to master
            # and deleted if not elected in the event of election
            try:
                # is_master can be true before DB_EVENT_REP_MASTER,
                # even though db is not ready for writing
                # so need to test for self.site_list before writing to db
                if self.app_data.is_master and hasattr(self, "site_list"):
                    # get list of all registered client sites
                    site_list = self.dbenv.repmgr_site_list()
                    obj = {}
                    host = self.app_config.this_host.host
                    port = self.app_config.this_host.port
                    # if I have come this far I am the Master
                    obj[('master',"%s:%d"%(host, port))] = "%s:%d"%(host, port)
                    # we are only interested in connected clients here
                    client_list = [(host, port) 
                                   for (host, port, status) in self.site_list.values()
                                   if status == db.DB_REPMGR_CONNECTED]
                    for host, port in client_list:
                        obj[('client', "%s:%d"%(host, port))] = "%s:%d"%(host, port)
                    # store the site list
                    self.set(ID, obj)
                    self.site_list = copy.deepcopy(site_list)
                time.sleep(period)
            except:
                log.msg()
                time.sleep(period)
                        
    def __storecfg2config(self, storecfg):
        """
        Function extracting relevant info from storecfg, returning config
        """
        self.app_config = RepConfigInfo()
        self.app_config.home = str(storecfg.Get('DataDir'))
        this_host = str(storecfg.Get('StoreHost'))
        try:
            # get host and port, must be in format host:port
            (host, port) = this_host.split(':')
            self.app_config.this_host.host = host
            self.app_config.this_host.port = int(port)
        except:
            log.msg()
            log.msg(arc.ERROR, "bad StoreHost, %s, cannot continue"%this_host)
            raise
        try:
            self.app_config.totalsites = int(str(storecfg.Get('TotalSites')))
        except:
            log.msg(arc.WARNING, "bad TotalSites, will try to find it my self")
        try:
            # we need to know about one other host to be able to 
            # start communication
            other_host = str(storecfg.Get('OtherHost'))
            host, port = other_host.split(':')
            self.app_config.addOtherHost(host, int(port))
        except:
            log.msg()
            log.msg(arc.ERROR, "Couldn't add other host")
            raise            
        try:
            # Priority used in elections: Higher priority means
            # higher chance of winning the master election
            # (i.e., becoming master)
            self.app_config.priority = int(str(storecfg.Get('Priority')))
        except:
            log.msg()
            log.msg(arc.ERROR, "Bad Priority value")
            raise            
        try:
            # In seconds, how often should we check for connected nodes
            self.app_config.check_period = float(str(storecfg.Get('CheckPeriod')))
        except:
            log.msg()
            log.msg(arc.ERROR, "Could not find checking period")
            raise            
        try:
            # amount of db cached in memory
            # Must be integer. Optionally in order of kB, MB, GB, TB or PB,
            # otherwise bytes is used
            cachesize = str(storecfg.Get('CacheSize'))
            cachemultiplier = {}
            cachemultiplier['kB']=1024            
            cachemultiplier['MB']=1024**2      
            cachemultiplier['GB']=1024**3         
            cachemultiplier['TB']=1024**4         
            cachemultiplier['PB']=1024**5
            tmp_cachesize = 0
            for m in cachemultiplier:
                if cachesize.endswith(m):
                    tmp_cachesize = int(cachesize.split(m)[0])*cachemultiplier[m]
            if tmp_cachesize == 0:
                tmp_cachesize = int(cachesize)
            CACHESIZE = tmp_cachesize
        except:
            log.msg(arc.WARNING, "Bad cache size or no cache size configured, using 10MB")
        return self.app_config

    def __configureReplication(self, storecfg):
        
        # get config from xml-config
        config = self.__storecfg2config(storecfg)        

        # tell dbenv to uset our event callback function
        # to handle various events
        self.dbenv.set_event_notify(self.event_callback)
        # Permanent messages require at least one ack
        # (i.e., at least one client must update it's db before
        # db change is ok'd
        self.dbenv.repmgr_set_ack_policy(db.DB_REPMGR_ACKS_ONE)
        # Give 500,000 microseconds to recieve the ack
        self.dbenv.rep_set_timeout(db.DB_REP_ACK_TIMEOUT, 500000)
        
        try:
            # let dbenv know I am me
            self.dbenv.repmgr_set_local_site(config.this_host.host,
                                             config.this_host.port)
        except:
            log.msg(arc.ERROR, "Could not set listen address to host:port %s:%s"%(config.this_host, 
                                                                                  config.this_host.port))
            log.msg()
            raise

        # strictly not necessary to iterate over one other host
        # could be useful for future possibility to configure more hosts
        for cur in config.other_hosts:
            try:
                self.dbenv.repmgr_add_remote_site(cur.host, cur.port)
            except:
                log.msg(arc.WARNING, "could not add site %s:%s"%(cur.host,cur.port))

        if config.totalsites > 0:
            try:
                self.dbenv.rep_set_nsites(config.totalsites)
            except db.DBError, (errnum, strerror):
                log.msg(arc.WARNING, "rep_set_nsites call failed, continuing...")

        # set priority for election
        self.dbenv.rep_set_priority(config.priority)

    def event_callback(self, dbenv, which, info):
        """
        Callback function used to determine whether the local environment is a 
        replica or a master. This is called by the replication framework
        when the local replication environment changes state
        app = dbenv.get_private()
        """

        info = None
        try:
            if which == db.DB_EVENT_REP_MASTER:
                log.msg(arc.DEBUG, "I am now a master")
                self.app_data.is_master = True
                
                # update replication host entry when master and ready
                # also set self.site_list, used in checking_thread
                # DON'T use DB_EVENT_REP_ELECTED here as db may not be ready

                ID = 'rephosts'

                self.set(ID, None)
                self.site_list = dbenv.repmgr_site_list()
                host = self.app_config.this_host.host
                port = self.app_config.this_host.port
                obj = {}
                obj[('master',"%s:%d"%(host, port))] = "%s:%d"%(host, port)
                client_list = [(host, port) 
                               for (host, port, status) in self.site_list.values()
                               if status == db.DB_REPMGR_CONNECTED]
                for host, port in client_list:
                    obj[('client', "%s:%d"%(host, port))] = "%s:%d"%(host, port)
                self.set(ID, obj)
                log.msg(arc.DEBUG, obj)
            elif which == db.DB_EVENT_REP_CLIENT:
                self.app_data.is_master = False
                log.msg(arc.DEBUG, "I am now a client")
            elif which == db.DB_EVENT_REP_STARTUPDONE:
                log.msg(arc.DEBUG, "Replication startup done")
            elif which == db.DB_EVENT_REP_PERM_FAILED:
                log.msg(arc.DEBUG, "Getting permission failed")
            elif which == db.DB_EVENT_WRITE_FAILED:
                log.msg(arc.DEBUG, "Write failed")
            elif which == db.DB_EVENT_REP_NEWMASTER:
                # lost the election, delete site_list
                log.msg(arc.DEBUG, "New master elected")
                if hasattr(self, "site_list"):
                    del self.site_list
            elif which == db.DB_EVENT_REP_ELECTED:
                log.msg(arc.DEBUG, "I won the election: I am the MASTER")
            elif which == db.DB_EVENT_PANIC:
                log.msg(arc.ERROR, "Oops! Internal DB panic!")
                raise db.DBRunRecoveryError, "Please run recovery."
        except:
            log.msg()
            raise