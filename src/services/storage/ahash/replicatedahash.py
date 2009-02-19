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
import threading
import copy
import base64
import zlib

from arcom.threadpool import ThreadPool, ReadWriteLock
from arcom.service import ahash_uri, node_to_data, get_child_nodes, parse_node, get_data_node
from storage.common import create_metadata, ahash_list_guid
from storage.xmltree import XMLTree 
from storage.ahash.ahash import CentralAHash
from storage.client import AHashClient
from arcom.store.transdbstore import TransDBStore
from arcom.logger import Logger
from arcom.security import parse_ssl_config
from bsddb3 import db
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.ReplicatedAHash'))


class ReplicatedAHash(CentralAHash):
    """ A replicated implementation of the A-Hash service. """

    def __init__(self, cfg):
        """ The constructor of the ReplicatedAHash class.

        ReplicatedAHash(cfg)

        """
        log.msg(arc.DEBUG, "ReplicatedAHash constructor called")
        # ssame ssl_config will be used for any ahash
        self.ssl_config = parse_ssl_config(cfg)
        self.store = None
        CentralAHash.__init__(self, cfg)
        self.ahashes = {}
        self.store = ReplicationStore(cfg, self.sendMsg)
        self.public_request_names = ['processMsg']
        # notify replication manager that communication is ready
        self.store.repmgr.comm_ready = True
        
    def sendMsg(self, url, repmsg):
        """
        Function used for callbacks from the communication framework
        of the replication manager
        Sends repmsg to url using HED
        """
        if not url in self.ahashes.keys():
            self.ahashes[url] = AHashClient(url, ssl_config=self.ssl_config,
                                            print_xml = False)
        ahash = self.ahashes[url]
        repmsg = base64.encodestring(str(repmsg))
        tree = XMLTree(from_tree = 
                       ('ahash:processMsg', [
                           ('ahash:msg', repmsg)
                           ]))
        log.msg(arc.DEBUG, "sending message of length %d to %s"%(len(repmsg),url))
        msg = ahash.call(tree)
        log.msg(arc.DEBUG, "sendt message")
        xml = arc.XMLNode(msg)
        objects = parse_node(get_data_node(xml), ['ID', 'metadataList'], single = True, string = False)
        success = xml.Get('ahash:success')
        return success

    def newSOAPPayload(self):
        return arc.PayloadSOAP(self.ns)

    def processMsg(self, inpayload):
        """
        processing ahash replication message
        """
        log.msg(arc.DEBUG, "processing message...")
        # get the grandchild of the root node, which is the 'changeRequestList'
        request_node = inpayload.Child()
        msg = eval(base64.decodestring(str(request_node.Get('msg'))))
        control = msg['control']
        record = msg['record']
        eid = msg['eid']
        retlsn = msg['lsn']
        sender = msg['sender']
        msgID = msg['msgID']

        resp = self.store.repmgr.processMsg(control, record, eid, retlsn, 
                                            sender, msgID)
        # prepare the response payload
        out = self.newSOAPPayload()
        # create the 'changeResponse' node
        response_node = out.NewChild('ahash:processResponse')
        # create an XMLTree for the response
        tree = XMLTree(from_tree = ('ahash:success', resp))
        # add the XMLTree to the XMLNode
        tree.add_to_node(response_node)
        log.msg(arc.DEBUG, "processing message... Finished")
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
                              ('Priority', (int(str(cfg.Get('Priority'))))),
                               ('CheckPeriod',(5)),
                              ('DBEnvFlags',
                               (str(dbenv_flags)))
                            ]])
        storecfg = arc.XMLNode(storecfg.pretty_xml()) 

        # db environment is opened in TransDBStore.__init__
        TransDBStore.__init__(self, storecfg, non_existent_object)

        # configure replication prior to initializing TransDBStore
        self.__configureReplication(cfg)


        log.msg(arc.DEBUG, "Initialized replication environment")

        # eid is a local environment id, so I will be number 1
        self.eid = 1
        # and other eid is 2
        other_eid = 2

        self.my_replica = {'url'      : self.my_url,
                           'id'       : self.eid,
                           'status'   : 'online'}
        other_replica =   {'url'      : self.other_url,
                           'id'       : other_eid,
                           'status'   : 'online'}

        # start replication manager
        self.repmgr = ReplicationManager(ahash_send, self.dbenv, 
                                         self.my_replica, other_replica)
        try:
            # start repmgr with 10 threads and always do election of master
            threading.Thread(target = self.repmgr.start, args=[1, db.DB_REP_ELECTION]).start()
        except:
            log.msg()
            log.msg(arc.ERROR, "Couldn't start replication manager.")
            self.terminate()

        # thread for maintaining host list
        threading.Thread(target = self.checkingThread, args=[self.check_period]).start()


    def __configureReplication(self, storecfg):
        self.my_url = str(storecfg.Get('MyURL'))
        self.other_url = str(storecfg.Get('OtherURL'))
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
        self.dbenv.repmgr_set_ack_policy(db.DB_REPMGR_ACKS_ONE)
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

        while True:
            log.msg(arc.DEBUG, "checkingThread slept %d s"%period)
            # self.site_list is set in event_callback when elected to master
            # and deleted if not elected in the event of election
            try:
                if self.repmgr.isMaster():
                    # get list of all registered client sites
                    site_list = self.repmgr.getSiteList()
                    # if I have come this far I am the Master
                    masterid = self.my_replica['id']
                    obj = {}
                    url = self.my_replica['url']
                    obj[('master', "%s:%d"%(url,masterid))] = url
                    # we are only interested in connected clients here
                    client_list = [(client['url'], client['id'])
                                   for id, client in site_list.items()
                                   if client['status'] == 'online']
                    for (url, id) in client_list:
                        obj[('client', "%s:%d"%(url, id))] = url
                    # store the site list
                    self.set(ahash_list_guid, obj)
                    log.msg(arc.DEBUG, "wrote ahash list %s"%str(obj))
                time.sleep(period)
            except:
                log.msg()
                time.sleep(period)

hostMap = {}

FIRST_MESSAGE = 1
REP_MESSAGE = 2
NEWSITE_MESSAGE = 3

class ReplicationManager:
    """
    class managing replicas, elections and message handling
    """
    
    def __init__(self, ahash_send, dbenv, my_replica, other_replica):
        # no master is found yet
        self.role = db.DB_EID_INVALID
        self.elected = False
        self.stop_electing = False
        self.ahash_send = ahash_send
        self.locker = ReadWriteLock()
        global hostMap
        self.hostMap = hostMap
        self.locker.acquire_write()
        self.hostMap[my_replica['id']] = my_replica
        self.hostMap[other_replica['id']] = other_replica
        self.locker.release_write()
        self.my_replica = my_replica
        self.eid = my_replica['id']
        self.url = my_replica['url']
        self.dbenv = dbenv
        # tell dbenv to uset our event callback function
        # to handle various events
        self.dbenv.set_event_notify(self.event_callback)
        self.pool = None
        self.comm_ready = False

    def isMaster(self):
        self.locker.acquire_read()
        is_master = self.role == db.DB_REP_MASTER
        self.locker.release_read()
        return is_master

    def getRole(self):
        self.locker.acquire_read()
        role = self.role
        self.locker.release_read()        
        return role

    def setRole(self, role):
        self.locker.acquire_write()
        self.role = role
        self.locker.release_write()
    
    def getSiteList(self):
        self.locker.acquire_read()
        site_list = copy.deepcopy(self.hostMap)
        self.locker.release_read()
        return site_list
    
    def start(self, nthreads, flags):
        log.msg(arc.DEBUG, "entering start")
        self.pool = ThreadPool(nthreads)
        while not self.comm_ready:
            time.sleep(2)
        # todo: need some threading
        try:
            # send message to say I'm here first
            self.sendFirstMsg()
            self.dbenv.rep_set_transport(self.eid, self.repSend)
            self.dbenv.rep_start(db.DB_REP_CLIENT)
            log.msg(arc.DEBUG, ("rep_start called with REP_CLIENT", self.hostMap))
            self.pool.queueTask(self.startElection)
        except:
            log.msg(arc.ERROR, "Couldn't start replication framework")
            log.msg()

            
    def startElection(self):
        log.msg(arc.DEBUG, "entering startElection")
        role = db.DB_EID_INVALID
        self.stop_electing = False  
        
        while role == db.DB_EID_INVALID and not self.stop_electing:
            try:
                self.locker.acquire_read()
                num_reps = len(self.hostMap)
                self.locker.release_read()
                votes = num_reps/2 + 1
                self.dbenv.rep_elect(num_reps, votes)
                # wait one second for election to finish
                # should be some better way to do this...
                time.sleep(2)
                role = self.getRole()
                log.msg(arc.DEBUG, "%s: my role is now %d"%(self.url, role))
                if self.elected:
                    self.elected = False
                    self.dbenv.rep_start(db.DB_REP_MASTER)                    
            except:
                log.msg(arc.ERROR, "Couldn't run election")
                log.msg(arc.DEBUG, "num_reps is %d, votes is %d"%(num_reps,votes))
                log.msg()
                time.sleep(2)
            log.msg(arc.DEBUG, "%s tried election with %d replicas"%(self.url, num_reps))

    def beginRole(self, role):
        try:
            # check if role has changed, to avoid too much write blocking
            if self.getRole() != role:
                self.setRole(role)
            self.dbenv.rep_start(role==db.DB_REP_MASTER and 
                                 db.DB_REP_MASTER or db.DB_REP_CLIENT)
        except:
            log.msg(arc.ERROR, "Couldn't begin role")
            log.msg()
        return
        
    def send(self, env, control, record, lsn, eid, flags, msgID):
        """
        callback function for dbenv transport
        """
        # wrap control, record, lsn, eid, flags and sender into dict
        # note: could be inefficient to send sender info for every message
        # if bandwidth and latency is low
        log.msg(arc.DEBUG, "entering send")
        sender = self.my_replica
        msg = {'control':control,
               'record':record,
               'lsn':lsn,
               'eid':eid,
               'msgID':msgID,
               'sender':sender}
        
        retval = 1
        if msgID==FIRST_MESSAGE or msgID==NEWSITE_MESSAGE:
            # send to all we know about
            eids = [id for id,rep in self.hostMap.items() if id != self.eid]    
        elif eid == db.DB_EID_BROADCAST:
            # send to all we know are online
            eids = [id for id,rep in self.hostMap.items() if rep['status']=="online" and id != self.eid]    
        else:
            eids = [eid]
            if not self.hostMap.has_key(eid):
                return retval
        for id in eids:
            try:
                msg['eid'] = id
                # no need to send to self

                resp = self.ahash_send(self.hostMap[id]['url'], msg)
                if str(resp) == "processed":
                    # if at least one msg is sent, we're happy
                    retval = 0
            except:
                # assume url is disconnected
                log.msg(arc.ERROR, "failed to send to %d of %s"%(id,str(eids)))
                log.msg()
                self.locker.acquire_write()
                #self.hostMap[id]['status'] = "offline"
                #self.hostMap.pop(id)
                self.locker.release_write()
        return retval
    
    def repSend(self, env, control, record, lsn, eid, flags):
        """
        callback function for dbenv transport
        """
        log.msg(arc.DEBUG, "entering repSend")
        res = self.send(env, control, record, lsn, eid, flags, REP_MESSAGE)
        return res
    
    def sendFirstMsg(self):
        log.msg(arc.DEBUG, "entering sendFirstMsg")
        return self.send(None, None, None, None, None, None, FIRST_MESSAGE)

    def sendNewSiteMsg(self, new_replica):
        log.msg(arc.DEBUG, "entering sendNewSiteMsg")
        self.locker.acquire_read()
        hostMap = copy.deepcopy(self.hostMap)
        self.locker.release_read()
        return self.send(None, None, hostMap, None, None, None, NEWSITE_MESSAGE)
    
    def processMsg(self, control, record, eid, retlsn, sender, msgID):
        """
        Function to process incoming messages, forwarding 
        them to self.dbenv.rep_process_message()
        """
        log.msg(arc.DEBUG, ("entering processMsg from ", sender))

        self.locker.acquire_read()
        urls = [rep['url'] for id,rep in self.hostMap.items()]
        self.locker.release_read()
        if not sender['url'] in urls:
            log.msg(arc.DEBUG, "received from new sender")
            self.locker.acquire_write()
            # really new sender, appending to host map
            newid = len(self.hostMap)+1
            sender['id'] = newid
            self.hostMap[newid] = sender
            self.hostMap[newid]['status'] = "online"
            self.locker.release_write()
            # send sender to everyone
            self.sendNewSiteMsg(sender)
        if msgID == NEWSITE_MESSAGE:
            log.msg(arc.DEBUG, "received NEWSITE_MESSAGE, %s"%(str(record)))
            for rep in record.values():
                if  not rep['url'] in urls:
                    self.locker.acquire_write()
                    # really new sender, appending to host map
                    newid = len(self.hostMap)+1
                    rep['id'] = newid
                    self.hostMap[newid] = rep
                    self.hostMap[newid]['status'] = "online"
                    self.locker.release_write()
            return "processed"

        if msgID == FIRST_MESSAGE:
            if sender['url'] == self.url:
                return "processed"
            log.msg(arc.DEBUG, "received FIRST_MESSAGE")
            self.locker.acquire_write()
            try:
                newid = [id for id,rep in self.hostMap.items() if rep['url']==sender['url']][0]
                # we know this one from before
                self.hostMap[newid]['status'] = "online"
            except:
                log.msg()
                # really new sender, appending to host map
                newid = len(self.hostMap)+1
                sender['id'] = newid
                self.hostMap[newid] = sender
                self.hostMap[newid]['status'] = "online"
            self.locker.release_write()
            log.msg(arc.DEBUG, ("FIRST_MESSAGE", self.url,  sender['url'], self.hostMap))
            return "processed"

        eid = [id for id,rep in self.hostMap.items() if rep['url']==sender['url']][0]
        try:
            log.msg(arc.DEBUG, "processing message from %d"%eid)
            res, retlsn = self.dbenv.rep_process_message(control, record, eid)
        except:
            log.msg(arc.ERROR, "couldn't process message")
            log.msg(arc.ERROR, (control, record, eid, retlsn, sender, msgID))
            log.msg()
            return "failed"
        
        if res == db.DB_REP_NEWSITE:
            # assign eid to n_reps, since I have eid 1
            if sender['url'] == self.url:
                return "processed"
            log.msg(arc.DEBUG, "received DB_REP_NEWSITE")
            self.locker.acquire_write()
            try:
                newid = [id for id,rep in self.hostMap.items() if rep['url']==sender['url']][0]
                # we know this one from before
                self.hostMap[newid]['status'] = "online"
            except:
                log.msg()
                # really new sender, appending to host map
                newid = len(self.hostMap)+1
                sender['id'] = newid
                self.hostMap[newid] = sender
                self.hostMap[newid]['status'] = "online"
            self.locker.release_write()
            urls = [rep['url'] for id,rep in self.hostMap.items()]
            log.msg(arc.DEBUG, ("NEWSITE", self.url,  sender['url'], urls))
        elif res == db.DB_REP_HOLDELECTION:
            log.msg(arc.DEBUG, "received DB_REP_HOLDELECTION")
            self.beginRole(db.DB_EID_INVALID)
            self.stop_electing = True
            nthreads = self.pool.getThreadCount()
            self.pool.joinAll()
            self.pool = ThreadPool(nthreads)
            self.pool.queueTask(self.startElection)
        elif res == db.DB_REP_ISPERM:
            log.msg(arc.DEBUG, "REP_ISPERM returned for LSN %s"%str(retlsn))
        elif res == db.DB_REP_NOTPERM:
            log.msg(arc.DEBUG, "REP_NOTPERM returned for LSN %s"%str(retlsn))
        elif res == db.DB_REP_DUPMASTER:
            log.msg(arc.DEBUG, "REP_DUPMASTER received, starting new election")
            # yield to revolution, switch to client
            self.beginRole(db.DB_EID_INVALID)
            self.stop_electing = True
            nthreads = self.pool.getThreadCount()
            self.pool.joinAll()
            self.pool = ThreadPool(nthreads)
            self.pool.queueTask(self.startElection)
        elif res == db.DB_REP_IGNORE:
            log.msg(arc.DEBUG, "REP_IGNORE received")
            log.msg(arc.ERROR, (control, record, eid, retlsn, sender, msgID))
        elif res == db.DB_REP_JOIN_FAILURE:
            log.msg(arc.ERROR, "JOIN_FAILIURE received")
        else:
            log.msg(arc.ERROR, "unknown return code %s"%str(res))
        
        return "processed"

    def event_callback(self, dbenv, which, info):
        """
        Callback function used to determine whether the local environment is a 
        replica or a master. This is called by the replication framework
        when the local replication environment changes state
        app = dbenv.get_private()
        """

        log.msg("entering event_callback")
        info = None
        try:
            if which == db.DB_EVENT_REP_MASTER:
                log.msg(arc.DEBUG, "I am now a master")
                log.msg(arc.DEBUG, "received DB_EVENT_REP_MASTER")
                self.setRole(db.DB_REP_MASTER)
            elif which == db.DB_EVENT_REP_CLIENT:
                log.msg(arc.DEBUG, "I am now a client")
                self.beginRole(db.DB_REP_CLIENT)
            elif which == db.DB_EVENT_REP_STARTUPDONE:
                log.msg(arc.DEBUG, ("Replication startup done",which,info))
                # I am now online, let others know about it
                self.sendFirstMsg()
            elif which == db.DB_EVENT_REP_PERM_FAILED:
                log.msg(arc.DEBUG, "Getting permission failed")
                self.dbenv.log_flush()
            elif which == db.DB_EVENT_WRITE_FAILED:
                log.msg(arc.DEBUG, "Write failed")
            elif which == db.DB_EVENT_REP_NEWMASTER:
                # lost the election, delete site_list
                log.msg(arc.DEBUG, "New master elected")
                log.msg(arc.DEBUG, (which, info))
                self.beginRole(db.DB_REP_CLIENT)
            elif which == db.DB_EVENT_REP_ELECTED:
                log.msg(arc.DEBUG, "I won the election: I am the MASTER")
                self.elected = True
                #self.beginRole(db.DB_REP_MASTER)
            elif which == db.DB_EVENT_PANIC:
                log.msg(arc.ERROR, "Oops! Internal DB panic!")
                raise db.DBRunRecoveryError, "Please run recovery."
        except:
            log.msg()
    