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
import sys
from arcom import get_child_values_by_name
from arcom.threadpool import ThreadPool, ReadWriteLock
from arcom.service import ahash_uri, node_to_data, get_child_nodes, parse_node, get_data_node
from storage.common import create_metadata, ahash_list_guid
from arcom.xmltree import XMLTree 
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
        self.store = ReplicationStore(cfg, self.sendMessage)
        self.public_request_names = ['processMessage']
        # notify replication manager that communication is ready
        self.store.repmgr.comm_ready = True

    def sendMessage(self, url, repmsg):
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
                       ('ahash:processMessage', [
                           ('ahash:msg', repmsg)
                           ]))
        log.msg(arc.DEBUG, "sending message of length %d to %s"%(len(repmsg),url))
        msg = ahash.call(tree)
        xml = ahash.xmlnode_class(msg)
        success = str(get_data_node(xml))
        log.msg(arc.DEBUG, "sendt message, success=%s"%success)
        return success

    def newSOAPPayload(self):
        return arc.PayloadSOAP(self.ns)

    def processMessage(self, inpayload):
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

        resp = self.store.repmgr.processMessage(control, record, eid, retlsn, 
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

        RepDBStore(cfg, sendMessage, processMessage)

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
        self.dbReady(False)
        # configure replication prior to initializing TransDBStore
        self.__configureReplication(cfg)


        log.msg(arc.DEBUG, "Initialized replication environment")

        # eid is a local environment id, so I will be number 1
        self.eid = 1
        self.my_replica = {'url'      : self.my_url,
                           'id'       : self.eid,
                           'status'   : 'online'}
        other_replicas = []
        other_eid = self.eid
        for url in self.other_urls:
            other_eid += 1
            other_replicas +=  [{'url'      : url,
                                 'id'       : other_eid,
                                 'status'   : 'offline'}]

        # start replication manager
        self.repmgr = ReplicationManager(ahash_send, self.dbenv, 
                                         self.my_replica, other_replicas, self.dbReady)
        try:
            # start repmgr with 10 threads and always do election of master
            threading.Thread(target = self.repmgr.start, args=[1, db.DB_REP_ELECTION]).start()
        except:
            log.msg()
            log.msg(arc.ERROR, "Couldn't start replication manager.")
            self.terminate()

        # thread for maintaining host list
        threading.Thread(target = self.checkingThread, args=[self.check_period]).start()


    def __configureReplication(self, cfg):
        self.my_url = str(cfg.Get('MyURL'))
        self.other_urls = get_child_values_by_name(cfg, 'OtherURL')
        try:
            # Priority used in elections: Higher priority means
            # higher chance of winning the master election
            # (i.e., becoming master)
            self.priority = int(str(cfg.Get('Priority')))
        except:
            log.msg(arc.ERROR, "Bad Priority value, using default 10")
            self.priority = 10
        try:
            # In seconds, how often should we check for connected nodes
            self.check_period = float(str(cfg.Get('CheckPeriod')))
        except:
            log.msg(arc.ERROR, "Could not find checking period, using default 10s")
            self.check_period = 10.0
        try:
            # amount of db cached in memory
            # Must be integer. Optionally in order of kB, MB, GB, TB or PB,
            # otherwise bytes is used
            cachesize = str(cfg.Get('CacheSize'))
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
        self.dbenv.repmgr_set_ack_policy(db.DB_REPMGR_ACKS_ALL)
        self.dbenv.rep_set_timeout(db.DB_REP_ACK_TIMEOUT, 500000)
        self.dbenv.rep_set_priority(self.priority)
        self.dbenv.rep_set_config(db.DB_REP_CONF_BULK, True)

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
                    new_obj = {}
                    url = self.my_replica['url']
                    status = self.my_replica['status']
                    new_obj[('master', "%s:%s"%(url,status))] = url
                    # we are only interested in connected clients here
                    client_list = [(client['url'], client['status'])
                                   for id, client in site_list.items()
                                   ]
                    for (url, status) in client_list:
                        new_obj[('client', "%s:%s"%(url, status))] = url
                    curr_obj = self.get(ahash_list_guid)
                    if True: #curr_obj != new_obj:
                        # store the site list if there are changes
                        self.set(ahash_list_guid, new_obj)
                        log.msg(arc.DEBUG, "wrote ahash list %s"%str(new_obj))
                        log.msg(arc.DEBUG, "old ahash list %s"%str(curr_obj))
                        if not self.dbenv_ready:
                            log.msg(arc.DEBUG, "but dbenv wasn't ready.")
            except:
                log.msg()
            time.sleep(period)
            
    def dbReady(self, db_is_ready=True):
        """
        Callback function used by repmgr to notify me that I can accept requests
        """
        self.dbenv_ready = db_is_ready
        
    def getDBReady(self):
        """
        Overload of TransDBStore.getDBReady
        """
        return self.dbenv_ready

hostMap = {}

NEWSITE_MESSAGE = 1
REP_MESSAGE = 2
HEARTBEAT_MESSAGE = 3
MASTER_MESSAGE = 4

class ReplicationManager:
    """
    class managing replicas, elections and message handling
    """
    
    def __init__(self, ahash_send, dbenv, my_replica, other_replicas, dbReady):
        
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
        for replica in other_replicas:
            self.hostMap[replica['id']] = replica
        self.locker.release_write()
        self.my_replica = my_replica
        self.eid = my_replica['id']
        self.url = my_replica['url']
        # assume no master yet
        self.masterID = db.DB_EID_INVALID
        self.dbenv = dbenv
        self.dbReady = dbReady
        self.dbenv.set_verbose(db.DB_VERB_REPLICATION, True)
        # tell dbenv to uset our event callback function
        # to handle various events
        self.dbenv.set_event_notify(self.event_callback)
        self.pool = None
        self.election_thread = threading.Thread(target=self.electionThread, args=[])
        self.comm_ready = False
        threading.Thread(target=self.heartbeatThread, 
                         args=[10]).start()
        self.check_heartbeat = False
        
    def isMaster(self):
        self.locker.acquire_read()
        is_master = self.role == db.DB_REP_MASTER
        self.locker.release_read()
        return is_master

    def heartbeatThread(self, period):
        """
        Thread for sending heartbeat messages
        Heartbeats are only sendt when master is elected and 
        running (i.e., when check_heartbeat is true)
        Only clients sends heartbeats
        If heartbeat is not answered, re-election will be initiated in send method
        """
        time.sleep(10)
        
        # todo: implement list of time_since_heartbeat so the master can mark clients 
        # as offline when low write activity
        while True:
            if self.role == db.DB_REP_CLIENT:
                if self.masterID != db.DB_EID_INVALID:
                    # use threaded send to avoid blocking
                    self.pool.queueTask(self.sendHeartbeatMsg)
            # add some randomness to avoid bugging the master too much
            log.msg(arc.DEBUG, ("heartbeat", self.masterID, self.role))
            time.sleep(period)

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
        try:
            self.dbenv.rep_set_transport(self.eid, self.repSend)
            self.dbenv.rep_start(db.DB_REP_CLIENT)
            log.msg(arc.DEBUG, ("rep_start called with REP_CLIENT", self.hostMap))
            self.startElection()
        except:
            log.msg(arc.ERROR, "Couldn't start replication framework")
            log.msg()

    def electionThread(self):
        role = db.DB_EID_INVALID
#        while role == db.DB_EID_INVALID and not self.stop_electing:
        try:
            log.msg(arc.DEBUG, "entered election thread")
            self.locker.acquire_read()
            num_reps = len([id for id,rep in self.hostMap.items() if rep["status"] != "offline"])
            self.locker.release_read()
            votes = num_reps/2 + 1
            log.msg(arc.DEBUG, "%s: my role is %d"%(self.url, role))
            self.dbenv.rep_elect(num_reps, votes)
            # wait one second for election results
            time.sleep(1)
            role = self.getRole()
            log.msg(arc.DEBUG, "%s: my role is now %d"%(self.url, role))
            if self.elected:
                self.elected = False
                self.dbenv.rep_start(db.DB_REP_MASTER)                    
        except:
            log.msg(arc.ERROR, "Couldn't run election")
            log.msg(arc.DEBUG, "num_reps is %d, votes is %d, hostMap is %s"%(num_reps,votes,str(self.hostMap)))
            log.msg()
            time.sleep(2)
        log.msg(arc.DEBUG, "%s tried election with %d replicas"%(self.url, num_reps))
            
    def startElection(self):
        log.msg(arc.DEBUG, "entering startElection")
        self.stop_electing = False
        self.dbReady(False)
        try:
            # try to join election_thread first to make sure only one election thread is running
            self.election_thread.join()
            self.election_thread = threading.Thread(target=self.electionThread, args=[])
        except:
            pass    
        self.election_thread.start()

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
        If no reply within 10 seconds, return 1
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
        if msgID == NEWSITE_MESSAGE:
            eids = [control['id']]
            msg['control'] = None
        elif msgID == HEARTBEAT_MESSAGE:
            eids = [self.masterID]
        elif eid == db.DB_EID_BROADCAST:
            # send to all
            eids = [id for id,rep in self.hostMap.items()]    
        else:
            eids = [eid]
            if not self.hostMap.has_key(eid):
                return retval
        for id in eids:
            if id == self.eid:
                continue
            try:
                msg['eid'] = id
                resp = ["waiting"]
                url = self.hostMap[id]['url']
                time_since_send = time.time()
                resp[0] = self.ahash_send(url, msg)
                if str(resp[0]) == "processed":
                    # if at least one msg is sent, we're happy
                    retval = 0
                    # received message so sender cannot be offline
                    if self.hostMap[id]['status'] == 'offline':
                        self.locker.acquire_write()
                        self.hostMap[id]['status'] = 'online'
                        self.locker.release_write()
                elif str(resp[0]).startswith("soap-env"):
                    # time since send less than 60 sec means that message didn't just time out
                    if id == self.masterID and time.time()-time_since_send < 55:
                        log.msg(arc.INFO, "Master is offline, starting re-election")
                        # in case more threads misses the master
                        if self.masterID != db.DB_EID_INVALID:
                            self.beginRole(db.DB_EID_INVALID)
                            self.masterID = db.DB_EID_INVALID
                            self.startElection()
                    raise RuntimeError, "No connection to server"
            except:
                # assume url is disconnected
                log.msg(arc.ERROR, "failed to send to %d of %s"%(id,str(eids)))
                log.msg()
                self.locker.acquire_write()
                # cannot have less than 2 online sites
                if len([id2 for id2,rep in self.hostMap.items() if rep['status']=="online"]) > 2:
                    self.hostMap[id]['status'] = "offline"
                    if msgID == NEWSITE_MESSAGE:
                        record['status'] = "offline"
                    if id == self.masterID:
                        log.msg(arc.INFO, "Master is offline, starting re-election")
                        # in case more threads misses the master
                        if self.masterID != db.DB_EID_INVALID:
                            self.beginRole(db.DB_EID_INVALID)
                            self.masterID = db.DB_EID_INVALID
                            self.startElection()
                self.locker.release_write()
        return retval
    
    def repSend(self, env, control, record, lsn, eid, flags):
        """
        callback function for dbenv transport
        """
        log.msg(arc.DEBUG, "entering repSend")
        if flags & db.DB_REP_PERMANENT and lsn != None:
            res = self.send(env, control, record, lsn, eid, flags, REP_MESSAGE)
            return res
        else:
            self.pool.queueTask(self.send, args=[env, control, record, lsn, eid, flags, REP_MESSAGE])
            return 0

    def sendNewSiteMsg(self, new_replica):
        """
        if new site is discovered sendNewSiteMsg will send 
        the hostMap to the new site
        """
        log.msg(arc.DEBUG, "entering sendNewSiteMsg")
        self.locker.acquire_read()
        hostMap = copy.deepcopy(self.hostMap)
        self.locker.release_read()
        ret = 1
        while ret:
            ret = self.send(None, new_replica, hostMap, None, None, None, NEWSITE_MESSAGE)
            if new_replica['status'] == 'offline':
                break
            time.sleep(30)
        return ret
    
    def sendHeartbeatMsg(self):
        """
        if new site is discovered sendNewSiteMsg will send 
        the hostMap to the new site
        """
        log.msg(arc.DEBUG, "entering sendHeartbeatMsg")
        return self.send(None, None, None, None, None, None, HEARTBEAT_MESSAGE)
    
    def sendNewMasterMsg(self, eid=db.DB_EID_BROADCAST):
        """
        If elected, broadcast master id to all clients
        """
        log.msg(arc.DEBUG, "entering sendNewMasterMsg")
        return self.send(None, None, None, None, eid, None, MASTER_MESSAGE)
    
    def processMessage(self, control, record, eid, retlsn, sender, msgID):
        """
        Function to process incoming messages, forwarding 
        them to self.dbenv.rep_process_message()
        """
        log.msg(arc.DEBUG, ("entering processMessage from ", sender))

        self.locker.acquire_read()
        urls = [rep['url'] for id,rep in self.hostMap.items() if rep['status']=='online']
        self.locker.release_read()
        if not sender['url'] in urls:
            log.msg(arc.DEBUG, "received from new sender or sender back online")
            really_new = False
            try:
                # check if we know this one
                newid = [id for id,rep in self.hostMap.items() if rep['url']==sender['url']][0]
            except:
                # nope, never heard about it
                newid = len(self.hostMap)+1
                really_new = True
            sender['id'] = newid
            self.locker.acquire_write()
            self.hostMap[newid] = sender
            self.locker.release_write()
            # return hostMap to sender
            if really_new:
                # use threaded send to avoid blocking
                self.pool.queueTask(self.sendNewSiteMsg, args=sender)
        if msgID == MASTER_MESSAGE:
            # sender is master, find local id for sender and set as masterID
            log.msg(arc.DEBUG, "received master id")
            self.masterID = [id for id,rep in self.hostMap.items() if rep['url']==sender['url']][0]
            return "processed"
        if msgID == HEARTBEAT_MESSAGE:
            log.msg(arc.DEBUG, "received HEARTBEAT_MESSAGE")
            return "processed"
        if msgID == NEWSITE_MESSAGE:
            # if unknown changes in record, update hostMap
            log.msg(arc.DEBUG, "received NEWSITE_MESSAGE")
            for replica in record.values():
                if  not replica['url'] in urls:
                    really_new = False
                    try:
                        # check if we know this one
                        newid = [id for id,rep in self.hostMap.items() if rep['url']==replica['url']][0]
                    except:
                        # nope, never heard about it
                        newid = len(self.hostMap)+1
                        really_new = True
                    self.locker.acquire_write()
                    # really new sender, appending to host map
                    replica['id'] = newid
                    self.hostMap[newid] = replica
                    self.locker.release_write()
                    # say hello to my new friend
                    if really_new:
                        # use threaded send to avoid blocking
                        self.pool.queueTask(self.sendNewSiteMsg, args=replica)
            return "processed"
        try:
            eid = [id for id,rep in self.hostMap.items() if rep['url']==sender['url']][0]
        except:
            return "notfound"
        if eid == self.masterID:
            self.time_since_master_message = time.time()
        try:
            log.msg(arc.DEBUG, "processing message from %d"%eid)
            res, retlsn = self.dbenv.rep_process_message(control, record, eid)
        except db.DBNotFoundError:
            log.msg(arc.ERROR, "Got dbnotfound")
            log.msg(arc.ERROR, (control, record, eid, retlsn, sender, msgID))
            log.msg()
            return "failed"
        except:
            log.msg(arc.ERROR, "couldn't process message")
            log.msg(arc.ERROR, (control, record, eid, retlsn, sender, msgID))
            log.msg()
            return "failed"
        
        if res == db.DB_REP_NEWSITE:
            log.msg(arc.DEBUG, "received DB_REP_NEWSITE from %s"%str(sender))
            if self.isMaster():
                # use threaded send to avoid blocking
                self.pool.queueTask(self.sendNewMasterMsg, args=eid)
        elif res == db.DB_REP_HOLDELECTION:
            log.msg(arc.DEBUG, "received DB_REP_HOLDELECTION")
            self.beginRole(db.DB_EID_INVALID)
            self.masterID = db.DB_EID_INVALID
            self.startElection()
        elif res == db.DB_REP_ISPERM:
            log.msg(arc.DEBUG, "REP_ISPERM returned for LSN %s"%str(retlsn))
            self.dbReady(True)
            self.setRole(db.DB_REP_CLIENT)
        elif res == db.DB_REP_NOTPERM:
            log.msg(arc.DEBUG, "REP_NOTPERM returned for LSN %s"%str(retlsn))
        elif res == db.DB_REP_DUPMASTER:
            log.msg(arc.DEBUG, "REP_DUPMASTER received, starting new election")
            # yield to revolution, switch to client
            self.beginRole(db.DB_EID_INVALID)
            self.masterID = db.DB_EID_INVALID
            self.startElection()
        elif res == db.DB_REP_IGNORE:
            log.msg(arc.DEBUG, "REP_IGNORE received")
            log.msg(arc.ERROR, (control, record, eid, retlsn, sender, msgID))
        elif res == db.DB_REP_JOIN_FAILURE:
            log.msg(arc.ERROR, "JOIN_FAILURE received")
        else:
            log.msg(arc.DEBUG, "unknown return code %s"%str(res))        

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
                self.dbReady(True)
                # use threaded send to avoid blocking
                self.pool.queueTask(self.sendNewMasterMsg)
            elif which == db.DB_EVENT_REP_CLIENT:
                log.msg(arc.DEBUG, "I am now a client")
                self.setRole(db.DB_REP_CLIENT)
                self.dbReady(True)
            elif which == db.DB_EVENT_REP_STARTUPDONE:
                log.msg(arc.DEBUG, ("Replication startup done",which,info))
            elif which == db.DB_EVENT_REP_PERM_FAILED:
                log.msg(arc.DEBUG, "Getting permission failed")
            elif which == db.DB_EVENT_WRITE_FAILED:
                log.msg(arc.DEBUG, "Write failed")
            elif which == db.DB_EVENT_REP_NEWMASTER:
                log.msg(arc.DEBUG, "New master elected")
                # give master 5 seconds to celebrate victory
                time.sleep(5)
                self.check_heartbeat = True
            elif which == db.DB_EVENT_REP_ELECTED:
                log.msg(arc.DEBUG, "I won the election: I am the MASTER")
                self.elected = True
                # use threaded send to avoid blocking
                self.pool.queueTask(self.sendNewMasterMsg)
            elif which == db.DB_EVENT_PANIC:
                log.msg(arc.ERROR, "Oops! Internal DB panic!")
                raise db.DBRunRecoveryError, "Please run recovery."
        except db.DBRunRecoveryError:
            sys.exit(1)      
        except:
            log.msg()
