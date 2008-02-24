"""
Hash
----

Centralized prototype implementation of the Hash service.
This service is a metadata-store capable of storing object with unique IDs,
where each object has a key-value pairs organized in sections.

Methods:
    - get
    - change

Sample configuration:

        <Service name="pythonservice" id="hash">
            <ClassName>storage.hash.hash.HashService</ClassName>
            <DataDir>hash_data</DataDir>
            <HashClass>storage.hash.hash.CentralHash</HashClass>
            <StoreClass>storage.hash.hash.PickleStore</StoreClass>
        </Service>
"""
import arc
import os
import traceback
from storage.common import import_class_from_string, hash_uri

import pickle, threading

class PickleStore:
    """ Class for storing object in a serialized python format. """

    def __init__(self, datadir, non_existent_object = {}):
        """ Constructor of PickleStore.

        PickleStore(datadir)

        'datadir' is a string, a path to the directory where to store the files
        'non_existent_object' will be returned if an object not found
        """
        print "PickleStore constructor called"
        self.datadir = datadir
        self.non_existent_object = non_existent_object
        # if the given data directory does not exist, try to create it
        if not os.path.exists(self.datadir):
            os.mkdir(self.datadir)
        print "datadir:", datadir
        # initialize a lock which can be used to avoid race conditions
        self.llock = threading.Lock()

    def filename(self, ID):
        """ Creates a filename from an ID.

        filename(ID)

        'ID' is the ID of the given object.
        The filename will be the datadir and the base64 encoded form of the ID.
        """
        import base64
        name = base64.b64encode(ID)
        return os.path.join(self.datadir, name[:2], name)

    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """
        try:
            # generates a filename from the ID
            # then use pickle to load the previously serialized data
            return pickle.load(file(self.filename(ID)))
        except IOError:
            # don't print 'file not found' if there is no such ID
            pass
        except:
            # print whatever exception happened
            print traceback.format_exc()
        # if there was an exception, return the given non_existent_object
        return self.non_existent_object

    def lock(self, blocking = True):
        """ Acquire the lock.

        lock(blocking = True)

        'blocking': if blocking is True, then this only returns when the lock is acquired.
        If it is False, then it returns immediately with False if the lock is not available,
        or with True if it could be acquired.
        """
        return self.llock.acquire(blocking)

    def unlock(self):
        """ Release the lock.

        unlock()
        """
        self.llock.release()

    def set(self, ID, object):
        """ Stores an object with the given ID..

        set(ID, object)

        'ID' is the ID of the object
        'object' is the object itself
        If there is already an object with this ID it will be overwritten completely.
        """
        try:
            # generates a filename from the ID
            fn = self.filename(ID)
            # if 'object' is empty, remove the file
            if not object:
                try:
                    os.remove(fn)
                except:
                    pass
            else:
                # try to open the file
                try:
                    f = file(fn,'w')
                except:
                    # try to create parent dir first, then open the file
                    os.mkdir(os.path.dirname(fn))
                    f = file(fn,'w')
                # serialize the given list into it
                pickle.dump(object, file(self.filename(ID),'w'))
        except:
            print traceback.format_exc()

class CentralHash:
    """ A centralized implementation of the Hash service. """

    def __init__(self, datadir, storeclass):
        """ The constructor of the CentralHash class.

        CentralHash(datadir, storeclass)

        'datadir' is the directory which can be used for storing the objects
            TODO: change this to 'general config of the storeclass'
        'storeclass' is the name of the class which will store the data
        """
        print "CentralHash constructor called"
        # import the storeclass and call its constructor with the datadir
        self.store = import_class_from_string(storeclass)(datadir)

    def get(self, ids):
        """ Gets all data of the given IDs.

        get(ids)

        'ids' is a list of strings: the IDs of the requested object
        Returns a dictionary of { ID : metadata }
            where 'metadata' is a dictionary where the keys are ('section', 'property') tuples
        """
        # initialize a list which will hold the results
        resp = []
        for ID in ids:
            # for each ID gets the object from the store
            metadata = self.store.get(ID)
            # appends the (ID, metadata) tuple to the resulting list
            resp.append((ID, metadata))
        # returns the resulting list
        return dict(resp)

    def change(self, changes):
        """ Change the '(section, property) : value' entries of an object, if the given conditions are met.

        change(changes)

        'changes' is a dictionary of {'changeID' : 'change'}, where
            'changeID' is a reference ID used in the response list
            'change' is a (ID, changeType, section, property, value, conditions) tuple, where:
                'ID' is the ID of the object which we want to change
                'changeType' could be 'set', 'unset' or 'delete'
                    'set' sets '(section, property)' to 'value'
                    'unset' removes '(section, property)', the value does not matter
                    'delete' removes the whole objects
                'conditions' is a dictionary of {'conditionID' : 'condition'}, where
                    'conditionID' is an ID of this condition
                    'condition' is a (type, section, property, value) tuple:
                        'type' could be 'is', 'not', 'exists', 'empty'
                            'is': '(section, property)' is set to 'value'
                            'isnot': '(section, property)' is not set to 'value'
                            'isset': '(section, property)' is set to any value
                            'unset': '(section, property)' is not set at all
        
        This method returns a dictionary of {'changeID' : (success, conditionID)},
            where success could be:
                - 'set'
                - 'unset'
                - 'deleted'
                - 'failed'
                - 'condition not met' (in this case, 'conditionID' gives the ID of the failed condition)
                - 'invalid change type'
                - 'unknown'
        """
        # prepare the list which will hold the response
        resp = []
        for (changeID, (ID, changeType, section, property, value, conditionList)) in changes.items():
            # for each change in the changes list
            print 'Prepare to lock store'
            # lock the store to avoid inconsistency
            self.store.lock()
            print 'Store locked'
            # prepare the 'success' of this change
            success = 'unknown'
            # prepare the 'conditionID' for an unmet condition
            unmetConditionID = ''
            try:
                # get the current content of the object
                obj = self.store.get(ID)
                # now check all the conditions if there is any
                ok = True
                for (conditionID, (conditionType, conditionSection,
                        conditionProperty, conditionValue)) in conditionList.items():
                    # get the current value of the conditional (section, property), or None if it is not set
                    currentValue = obj.get((conditionSection, conditionProperty), None)
                    if conditionType == 'is':
                        # if the (section, property) is not set to value
                        if currentValue != conditionValue:
                            ok = False
                            unmetConditionID = conditionID
                            # jump out of the for statement
                            break
                    elif conditionType == 'isnot':
                        # if the (section, property) is set to value
                        if currentValue == conditionValue:
                            ok = False
                            unmetConditionID = conditionID
                            break
                    elif conditionType == 'isset':
                        # if the (section, property) is not set:
                        if currentValue is None:
                            ok = False
                            unmetConditionID = conditionID
                            break
                    elif conditionType == 'unset':
                        # if the (section, property) is set:
                        if currentValue is not None:
                            ok = False
                            unmetConditionID = conditionID
                            break
                # if 'ok' is true then all conditions are met
                if ok:
                    if changeType == 'set':
                        # sets the new value (overwrites old value of this (section, property))
                        obj[(section, property)] = value
                        # store the changed object
                        self.store.set(ID, obj)
                        # set the result of this change
                        success = 'set'
                    elif changeType == 'unset':
                        # removes the entry of (section, property)
                        del obj[(section, property)]
                        # store the changed object
                        self.store.set(ID,lines)
                        # set the result of this change
                        success = 'unset'
                    elif changeType == 'delete':
                        # remove the whole object
                        self.store.set(ID, None)
                        success = 'deleted'
                    else:
                        # there is no other changeType
                        success = 'invalid change type'
                else:
                    success = 'condition not met'
            except:
                # if there was an exception, set this to failed
                success = 'failed'
                print traceback.format_exc()
            # we are done, release the lock
            self.store.unlock()
            print 'Store unlocked'
            # append the result of the change to the response list
            resp.append((changeID, (success, unmetConditionID)))
        return dict(resp)

from storage.xmltree import XMLTree

class HashService:
    """ HashService class implementing the XML interface of the Hash service. """

    # names of provided methods
    request_names = ['get','change']

    def __init__(self, cfg):
        """ Constructor of the HashService

        HashService(cfg)

        'cfg' is an XMLNode which containes the config of this service.
        """
        print "HashService constructor called"
        # this is a directory for storing object data
        # TODO: change this to general storeclass config
        datadir = str(cfg.Get('DataDir'))
        # the name of the class which implements the business logic of the Hash service
        hashclass = str(cfg.Get('HashClass'))
        # the name of the class which is capable of storing the object
        storeclass = str(cfg.Get('StoreClass'))
        # import and instatiate the business logic class
        self.hash = import_class_from_string(hashclass)(datadir, storeclass)
        # set the default namespace for the Hash service
        self.hash_ns = arc.NS({'hash':hash_uri})

    def get(self, inpayload):
        """ Returns the data of the requested objects.

        get(inpayload)

        'inpayload' is an XMLNode containing the IDs of the requested objects
        """
        # extract the IDs from the XMLNode using the '//ID' XPath expression
        ids = [str(id) for id in inpayload.XPathLookup('//hash:ID', self.hash_ns)]
        # gets the result from the business logic class
        objects = self.hash.get(ids)
        # create the response payload
        out = arc.PayloadSOAP(self.hash_ns)
        # create the 'getResponse' node
        response_node = out.NewChild('hash:getResponse')
        # create an XMLTree from the results
        tree = XMLTree(from_tree = 
            ('hash:objects', [
                ('hash:object', [ # for each object
                    ('hash:ID', ID),
                    ('hash:metadataList', [
                        ('hash:metadata', [ # for each dictionary item of this object
                            ('hash:section', section),
                            ('hash:property', property),
                            ('hash:value', value)
                        ]) for ((section, property), value) in metadata.items()
                    ])
                ]) for (ID, metadata) in objects.items()
            ])
        )
        # convert to tree to XML via adding it to the 'getResponse' node
        tree.add_to_node(response_node)
        return out

    def _condition(self, cond):
        # helper method which convert a condition to a tuple
        d = dict(cond)
        return d['hash:conditionID'], (d['hash:conditionType'],
                    d['hash:section'], d['hash:property'], d['hash:value'])

    def change(self, inpayload):
        """ Make changes in objects, if given conditions are met, return which change was successful.

        change(inpayload)

        'inpayload' is an XMLNode containing the change requests.
        """
        # get the 'changeRequestList' node and convert it to XMLTree
        try:
            tree = XMLTree(inpayload.XPathLookup('//hash:changeRequestList', self.hash_ns)[0])
        except:
            raise Exception, 'wrong request: //hash:changeRequestList not found'
        # create a list of dictionaries from the 'changeRequestElement' nodes
        req_list = tree.get_dicts('/hash:changeRequestList/hash:changeRequestElement')
        requests = []
        for req in req_list:
            # convert the XMLTree structure to a list of condition
            conditions = [self._condition(cond) for (_, cond) in req.get('hash:conditionList',[])]
            request = (req['hash:changeID'], (req['hash:ID'], req['hash:changeType'], \
                req['hash:section'], req['hash:property'], req['hash:value'], dict(conditions)))
            requests.append(request)
        # call the business logic class
        resp = self.hash.change(dict(requests))
        # prepare the response payload
        out = arc.PayloadSOAP(self.hash_ns)
        # create the 'changeResponse' node
        response_node = out.NewChild('hash:changeResponse')
        # create an XMLTree for the response
        tree = XMLTree(from_tree = 
            ('hash:changeResponseList', [
                ('hash:changeResponseElement', [ # for each change
                    ('hash:changeID', changeID),
                    ('hash:success', success),
                    ('hash:conditionID', conditionID)
                ]) for (changeID, (success, conditionID)) in resp.items()
            ])
        )
        # add the XMLTree to the XMLNode
        tree.add_to_node(response_node)
        return out

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        print "Process called"
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the Hash namespace in its incoming payload
            hash_prefix = inpayload.NamespacePrefix(hash_uri)
            # get the qualified name of the request
            request_name = inpayload.Child().FullName()
            #print request_name
            if not request_name.startswith(hash_prefix + ':'):
                # if the request is not in the Hash namespace
                raise Exception, 'wrong namespace (%s)' % request_name
            # get the name of the request without the namespace prefix
            request_name = inpayload.Child().Name()
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            outpayload = getattr(self,request_name)(inpayload)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            exc = traceback.format_exc()
            print exc
            # set an empty outgoing payload
            outpayload = arc.PayloadSOAP(self.hash_ns)
            outpayload.NewChild('hash:Fault').Set(exc)
            outmsg.Payload(outpayload)
            # return with the status GENERIC_ERROR
            return arc.MCC_Status(arc.STATUS_OK)
