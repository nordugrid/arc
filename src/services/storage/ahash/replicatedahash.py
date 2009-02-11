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

from arcom.threadpool import ThreadPool, ReadWriteLock
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
        xml = arc.XMLNode(msg)
        objects = parse_node(get_data_node(xml), ['ID', 'metadataList'], single = True, string = False)
        success = xml.Get('ahash:success')
        return success

    def newSOAPPayload(self):
        return arc.PayloadSOAP(self.ns)

    def processMsg(self, inpayload):
        """
        processing ahash msg
        """
        
        # get the grandchild of the root node, which is the 'changeRequestList'
        requests_node = inpayload.Child().Child()
        control = cPickle.loads(str(request_node.Get['control']))
        record = cPickle.loads(str(request_node.Get['record']))
        eid = cPickle.loads(str(request_node.Get['eid']))
        retlsn = cPickle.loads(str(request_node.Get['retlsn']))
        sender = cPickle.loads(str(request_node.Get['sender']))

        resp = self.store.repmgr.processMsg(control, record, eid, retlsn, sender)

        # prepare the response payload
        out = self.newSOAPPayload()
        # create the 'changeResponse' node
        response_node = out.NewChild('ahash:processResponse')
        # create an XMLTree for the response
        tree = XMLTree(from_tree = ('ahash:success', rep))
        # add the XMLTree to the XMLNode
        tree.add_to_node(response_node)
        return out

        
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
            return (self.repmgr.isMaster() and 
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
                if self.repmgr.isMaster() and hasattr(self, "site_list"):
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

hostMap = {}

class ReplicationManager:
    """
    class managing replicas, elections and message handling
    """
    
    def __init__(self, ahash_send, dbenv, my_replica, other_replica):
        # no master is found yet
        self.masterID = db.DB_EID_INVALID
        self.ahash_send = ahash_send
        self.locker = ReadWriteLock()
        global hostMap
        self.hostMap = hostMap
        self.locker.acquire_write()
        self.hostMap[my_replica.eid] = my_replica
        self.hostMap[other_replica.eid] = other_replica
        self.locker.release_write()
        self.eid = my_replica.eid
        self.dbenv = dbenv

    def isMaster(self):
        self.locker.acquire_read()
        is_master = self.masterID == db.DB_REP_MASTER
        self.locker.release_read()
        return is_master

    def getMasterID(self):
        self.locker.acquire_read()
        masterID = self.masterID
        self.locker.release_read()        
        return self.masterID

    def setMasterID(self, masterID):
        self.locker.acquire_write()
        self.masterID = masterID
        self.locker.release_write()
    
    def start(self, nthreads, flags):
        # need some threading
        try:
            self.dbenv.rep_set_transport(self.eid, repSend)
            self.dbenv.rep_start(db.DB_REP_CLIENT, cdata=self.hostMap[self.eid].url)
            self.startElection()
        except:
            log.msg(arc.ERROR, "Couldn't start replication framework")
            log.msg()

            
    def startElection(self):
        print 'election'
        
        masterID = db.DB_EID_INVALID
        
        num_reps = len(self.hostMap)
        priority = self.hostMap[self.eid]
        votes = num_rep/2 + 1
        
        while masterID == db.DB_EID_INVALID:
            try:
                self.dbenv.rep_elect(num_reps, votes)
                # wait one second for election to finish
                # should be some better way to do this...
                time.sleep(1)
                masterID = self.getMasterID()
                log.msg(arc.DEBUG)
            except:
                log.msg(arc.ERROR, "Couldn't run election")
                log.msg()

    def beginRole(self, role):
        try:
            self.locker.acquire_write()
            self.envp.rep_start(role, cdata=self.hostMap[self.eid].url)
        except:
            log.msg(arc.ERROR, "Couldn't begin role")
            log.msg()
        self.locker.release_write()
        return
        
    def repSend(self, env, control, record, lsn, eid, flags):
        """
        callback function for dbenv transport
        """
        # wrap control, record, lsn, eid, flags and sender into dict
        # note: could be inefficient to send sender info for every message
        # if bandwidth and latency is low
        msg = {'control':cPickle.dumps(control),
               'record':cPickle.dumps(record),
               'lsn':cPickle.dumps(lsn),
               'eid':cPickle.dumps(eid),
               'sender':cPickle.dumps(self.hostMap[self.eid])}
        try:
            resp = self.ahash_send(self.hostMap[eid].url, msg)
            if str(resp) == "processed":
                return 0
        except:
            # assume url is disconnected
            if eid==self.masterID:
                self.startElection()
        return 1
    
    def processMsg(self, control, record, eid, retlsn, sender):
        """
        Function to process incoming messages, forwarding 
        them to self.envp.rep_process_message()
        """
        
        try:
            log.msg(arc.DEBUG, "processing message from %d"%eid)
            res, retlsn = self.envp.rep_process_message(control, record, eid)
        except:
            log.msg(arc.ERROR, "couldn't process message")
            log.msg()
            return "failed"
        
        if res == db.DB_REP_NEWSITE:
            # assign eid to n_reps, since I have eid 1
            self.locker.acquire_write()
            self.hostMap[len(self.hostMap)] = cPickle.loads(sender)
            self.locker.release_write()
        elif res == db.DB_REP_HOLD_ELECTION:
            self.startElection()
        elif res == db.DB_REP_NEWMASTER:
            self.setMasterID(eid)
            log.msg(arc.DEBUG, "New master is %d"%eid)
        elif res == db.DB_REP_ISPERM:
            log.msg(arc.DEBUG, "REP_ISPERM returned for LSN %s"%str(retlsn))
        elif res == db.dB_REP_NOTPERM:
            log.msg(arc.DEBUG, "REP_NOTPERM returned for LSN %s"%str(retlsn))
        elif res == db.DB_REP_DUPMASTER:
            log.msg(arc.DEBUG, "REP_DUPMASTER received, starting new election")
            # yield to revolution, switch to client
            self.beginRole(db.DB_REP_CLIENT)
            self.startElection()
        elif res == db.DB_REP_IGNORE:
            log.msg(arc.DEBUG, "REP_IGNORE received")
        elif res == db.DB_REP_JOIN_FAILURE:
            log.msg(arc.ERROR, "JOIN_FAILIURE received")
        elif res == db.DB_REP_STARTUPDONE:
            log.msg(arc.DEBUG, "Startup done")
        else:
            log.msg(arc.ERROR, "unknown return code %d"%res)
        
        return "processed"
    