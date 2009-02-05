"""
Replicated A-Hash
----

Replicated prototype implementation of the A-Hash service.
This service builds on the centralized A-Hash and 
Berkeley DB High Availability.

Methods:
    - get
    - change
    - sendMessage
    - processMessage
Sample configuration:

        <Service name="pythonservice" id="ahash">
            <ClassName>storage.ahash.ahash.AHashService</ClassName>
            <AHashClass>storage.ahash.replicatedahash.ReplicatedAHash</AHashClass>
            <LocalDir>ahash_data</LocalDir>
            <MyURL>http://localhost:60000/RepAHash</MyURL>
            <OtherURL>http://otherhost:60000/RepAHash</OtherURL>
        </Service>
"""
import arc
import traceback
import time
import cPickle

from storage.common import ahash_uri, node_to_data, create_metadata, get_child_nodes
from storage.xmltree import XMLTree 
from storage.ahash.ahash import CentralAHash
from storage.client import AHashClient
from storage.store.transdbstore import TransDBStore
from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.ReplicatedAHash'))


class ReplicatedAHash(CentralAHash):
    """ A replicated implementation of the A-Hash service. """

    def __init__(self, cfg):
        """ The constructor of the ReplicatedAHash class.

        ReplicatedAHash(cfg)

        """
        log.msg(arc.DEBUG, "ReplicatedAHash constructor called")
        # same ssl_config will be used for any ahash
        self.ssl_config = parse_ssl_config(cfg)
        self.ahashes = {}
        self.store = ReplicationStore(cfg, self.sendMsg)
        self.public_request_names = ['processMsg']
        

    def sendMsg(self, url, repmsg):
        if not url in self.ahashes.keys():
            self.ahashes[url] = AHashClient(url, ssl_config = self.ssl_config)
        ahash = self.ahashes[url]
        tree = XMLTree(from_tree = 
                       ('ahash:processMsg', [
                           ('ahash:%s'%arg, value) for arg,value in repmsg.items()
                           ]))
        msg = ahash.call(tree)
        return msg

    def processMsg(self, msg):
        """
        processing ahash msg
        """
        ### unwrap msg and call store.processMsg

        
        
class ReplicationStore(TransDBStore):
    """
    Wrapper class for enabling replication in TransDBStore
    """
    
    def __init__(self, cfg, ahash_send, 
                 non_existent_object = {}):
        """ Constructor of ReplicationStore.

        RepDBStore(cfg, sendMsg, processMsg)

        """
        
        dbenv_flags = db.DB_CREATE | \
                      db.DB_RECOVER | \
                      db.DB_THREAD | \
                      db.DB_INIT_REP | \
                      db.DB_INIT_LOCK | \
                      db.DB_INIT_LOG | \
                      db.DB_INIT_MPOOL | \
                      db.DB_INIT_TXN

        storecfg = XMLTree(from_tree = 
                           ['StoreCfg', 
                            [('DataDir', 
                              (str(cfg.Get('LocalDir')))),
                              ('DBEnvFlags',
                               (str(dbenv_flags)))
                            ]])
        storecfg = arc.XMLNode(storecfg.pretty_xml()) 

        # configure replication prior to initializing TransDBStore
        self.__configureReplication(cfg)

        # db environment is opened in TransDBStore.__init__
        TransDBStore.__init__(self, storecfg, non_existent_object)

        log.msg(arc.DEBUG, "Initialized replication environment")

        # eid is a local environment id, so I will be number 1
        self.eid = 1 
        # and other eid is 2
        other_eid = 2
        self.my_replica = Replica(self.my_url, self.eid, self.priority)
        other_replica = Replica(self.my_url, other_eid, 0)
        # start replication manager
        self.repmgr = ReplicationManager(ahash_send, self.dbenv, 
                                         self.my_replica, other_replica)
        try:
            # start repmgr with 3 threads and always do election of master
            self.repmgr.start(3, db.DB_REP_ELECTION)
        except:
            log.msg()
            log.msg(arc.ERROR, "Couldn't start replication manager.")
            self.terminate()

        # thread for maintaining host list
        threading.Thread(target = self.checkingThread, args=[self.app_config.check_period]).start()

    def __configureReplication(self, cfg):
        self.my_url = str(storecfg.Get('MyURL'))
        self.other_urls = [str(storecfg.Get('OtherURL'))]
        try:
            # Priority used in elections: Higher priority means
            # higher chance of winning the master election
            # (i.e., becoming master)
            self.priority = int(str(storecfg.Get('Priority')))
        except:
            log.msg(arc.ERROR, "Bad Priority value, using default 10")
            self.priority = 10
        try:
            # In seconds, how often should we check for connected nodes
            self.check_period = float(str(storecfg.Get('CheckPeriod')))
        except:
            log.msg(arc.ERROR, "Could not find checking period, using default 10s")
            self.check_period = 10.0
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
            self.cachesize = tmp_cachesize
        except:
            log.msg(arc.WARNING, "Bad cache size or no cache size configured, using 10MB")
            self.cachesize = 10*(1024**2)
        self.dbenv.set_event_notify(self.event_callback)
        #self.dbenv.repmgr_set_ack_policy(db.DB_REPMGR_ACKS_ONE)
        self.dbenv.rep_set_timeout(db.DB_REP_ACK_TIMEOUT, 500000)
        self.dbenv.rep_set_priority(self.priority)


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

class Replica:
    """
    class holding info about replica
    """
    def __init__(self, url, id, priority):
        self.url = url
        self.id = id
        self.priority = priority
        self.sentLsn = 0
        self.retLsn = 0

class ReplicationManager:
    """
    class managing replicas, elections and message handling
    """
    
    def __init__(self, ahash_send, dbenv, my_replica, other_replica):
        # no master is found yet
        self.masterID = db.DB_EID_INVALID
        self.ahash_send = ahash_send
        self.hostMap = {my_replica.eid:my_replica, other_replica.eid:other_replica}
        self.eid = my_replica.eid
        self.dbenv = dbenv

    def isMaster(self):
        if self.masterID == db.DB_REP_MASTER:
            return True
        else:
            return False

    def start(self, nthreads, flags):
        # need some threading
        try:
            self.dbenv.rep_set_transport(self.eid, repSend)
            self.dbenv.rep_start(db.DB_REP_CLIENT, cdata=self.hostMap[self.eid].url)
            self.startElection()
        except:
            log.msg(arc.ERROR, "Couldn't start replication framework")
            log.msg()
        # whait for db
        # start thread pool here
            
    def startElection(self):
        print 'election'
        
        master = db.DB_EID_INVALID
        
    
    def repSend(self, env, control, record, lsn, eid, flags):
        """
        callback function for dbenv transport
        """
        # wrap control, record, lsn, eid and flags into dict
        msg = {'control':cPickle.dumps(control),
               'record':cPickle.dumps(record),
               'lsn':cPickle.dumps(lsn),
               'eid':eid}
        try:
            resp = self.ahash_send(self.hostMap[eid].url, msg)
            return 0
        except:
            # assume url is disconnected
            if eid==self.masterID:
                self.startElection()

    def processMsg(self, control, record, eid, retlsn):
        """
        Function to process incoming messages, forwarding 
        them to self.envp.rep_process_message()
        """
        return
