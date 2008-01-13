"""
Hash
----

Centralized prototype implementation of the Hash service.
This service is a metadat store capable of storing (section, property, value) tuples
in objects which have IDs.

Methods:
    - get
    - change (may be conditional)

Sample configuration:

        <Service name="pythonservice" id="hash">
            <ClassName>hash.hash.HashService</ClassName>
            <DataDir>hash_data</DataDir>
            <HashClass>hash.hash.CentralHash</HashClass>
            <StoreClass>hash.hash.PickleStore</StoreClass>
        </Service>

see also: hash_client.py for a very basic client for testing.
"""
import arc
import base64
import os
import pickle
import time
import threading
import traceback

hash_uri = 'urn:hash'

def import_class_from_string(str):
    """ Import the class which name is in the string
    
    import_class_from_string(str)

    'str' is the name of the class in [package.]*module.classname format.
    """
    module = '.'.join(str.split('.')[:-1]) # cut off class name
    cl = __import__(module) # import module
    for name in str.split('.')[1:]: # start from the first package name
        # step to the next package or the module or the class
        cl = getattr(cl, name)
    return cl # return the class


class PickleStore:
    """ Class for storing (section, property, value) lists in serialized python format. """

    def __init__(self, datadir):
        """ Constructor of PickleStore.

        PickleStore(datadir)

        'datadir' is a string, a path to the directory where to store the files
        """
        print "PickleStore constructor called"
        self.datadir = datadir
        print "datadir:", datadir
        # initialize a lock which can be used to avoid race conditions
        self.llock = threading.Lock()

    def filename(self, ID):
        """ Creates a filename from an ID.

        filename(ID)

        'ID' is the ID of the given object.
        The filename will be the datadir and the base64 encoded form of the ID.
        """
        return os.path.join(self.datadir,base64.b64encode(ID))

    def get(self, ID):
        """ Returns the (section, property, value) list of the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns an empty list.
        """
        try:
            # generates a filename from the ID
            # then use pickle to load the previously serialized data
            return pickle.load(file(self.filename(ID)))
        except:
            # print whatever exception happened
            print traceback.format_exc()
        # if there was an exception, return an empty list
        return []

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

    def set(self, ID, lines):
        """ Stores an object with the given ID and (section, property, value) list.

        set(ID, lines)

        'ID' is the ID of the object
        'lines' is a list of (section, property, value) tuple, where all three are strings
        If there is already an object with this ID it will be overwritten completely.
        """
        try:
            # generates a filename from the ID, then serialize the given list into it
            pickle.dump(lines, file(self.filename(ID),'w'))
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
        Returns a list of (ID, lines) tuples where
            'lines' is a list of (section, property, value) strings
        """
        #print 'CentralHash.get(%s)' % ids
        # initialize a list which will hold the results
        resp = []
        for ID in ids:
            # for each ID gets the lines from the store
            lines = self.store.get(ID)
            # appends the (ID, lines) tuple to the resulting list
            resp.append((ID,lines))
        # returns the resulting list
        return resp

    def change(self, changes):
        """ Change the (section, property, value) members of an object, if the given conditions are met.

        change(changes)

        'changes' is a list of conditional changes
        a conditional change is a dictionary with members such as:
            'changeID' is a reference ID used in the response list
            'ID' is the ID of the object which we want to change
            'type' could be 'add', 'remove', 'delete' or 'reset'
                'add' adds a new (section, property, value) line to the object
                'remove' removes a (section, property, value) line from the object
                'delete' removes all values of a corresponding section and property
                    (the value does not matter at this type)
                'reset' removes all values of a corresponding section and property
                    then adds the new value which is given
            'line' is a (section, property, value) tuple of the change
            'conditions' is a list of conditions
            a condition is a dictionary with members such as:
                'type' could be 'has', 'not', 'exists', 'empty'
                    'has': there is such a (section, property, value) line
                    'not': there is no such (section, property, value) line
                    'exists': there is a (section, property, *) line with any value
                    'empty': there is no (section, proprety, *) line with any value
                'line' is a (section, property, value) tuple of the condition
        
        This method returns a list of (changeID, success) pairs,
        where success could be:
            - 'added'
            - 'removed'
            - 'deleted'
            - 'reseted'
            - 'not changed'
            - 'invalid change type'
            - 'failed'
            - 'unknown'
        """
        # prepare the list which will hold the response
        resp = []
        for ch in changes:
            # for each change in the changes list
            #print ch
            print 'Prepare to lock store'
            # lock the store to avoid inconsistency
            self.store.lock()
            print 'Store locked'
            # prepare the 'success' of this change
            success = 'unknown'
            try:
                # get the ID of the object we want to change for easy referencing
                ID = ch['ID']
                # get the current (section, property, value) lines of the object
                lines = self.store.get(ID)
                #print lines
                # now check all the conditions if there is any
                ok = True
                for cond in ch.get('conditions',[]):
                    if cond['type'] == 'has':
                        # if there is no such section, property, value line in the object
                        if not (cond['line'] in lines):
                            ok = False
                            # jump out of the for statement
                            break
                    elif cond['type'] == 'not':
                        # if there is such a section, property, value line in the object
                        if (cond['line'] in lines):
                            ok = False
                            break
                    elif cond['type'] == 'exists':
                        # get all the lines which has the given section and property
                        cond_lines = [l for l in lines if l[0:2] == cond['line'][0:2]]
                        # if this is empty, there is no such line
                        if len(cond_lines) == 0:
                            ok = False
                            break
                    elif cond['type'] == 'empty':
                        # get all the lines which has the given section and property
                        cond_lines = [l for l in lines if l[0:2] == cond['line'][0:2]]
                        # if this is not empty
                        if len(cond_lines) > 0:
                            ok = False
                            break
                # if 'ok' is true then all conditions are met
                if ok:
                    if ch['type'] == 'add':
                        # if changeType is add, simply add the new line to the object
                        lines.append(ch['line'])
                        #print lines
                        # store the changed object
                        self.store.set(ID,lines)
                        # set the result of this change
                        success = 'added'
                    elif ch['type'] == 'remove':
                        # if changeType is remove, simply remove the given line from the object
                        lines.remove(ch['line']) 
                        #print lines
                        # store the changed object
                        self.store.set(ID,lines)
                        # set the result of this change
                        success = 'removed'
                    elif ch['type'] in ['delete','reset']:
                        #print 'delete', lines
                        # if it is a 'delete' or a 'reset'
                        # remove all the lines which has the given section and the given property
                        lines = [l for l in lines if not (l[0:2] == ch['line'][0:2])]
                        #print 'deleted', lines
                        if ch['type'] == 'delete':
                            # if this was a 'delete' we are done
                            success = 'removed'
                        else:
                            # if this was a reset, we need to add the new line
                            lines.append(ch['line'])
                            #print 'reseted', lines
                            success = 'reseted'
                        # in both cases we need to store the changed object
                        self.store.set(ID,lines)
                    else:
                        # there is no other changeType
                        success = 'invalid change type'
                else:
                    success = 'not changed'
            except:
                # if there was an exception, set this to failed
                success = 'failed'
                print traceback.format_exc()
            # we are done, release the lock
            self.store.unlock()
            # append the result of the change to the response list
            resp.append((ch['changeID'],success))
            print 'Store unlocked'
        return resp

from xmltree import XMLTree

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
        resp = self.hash.get(ids)
        # create the response payload
        out = arc.PayloadSOAP(self.hash_ns)
        # create the 'getResponse' node
        response_node = out.NewChild('hash:getResponse')
        #print resp
        # create an XMLTree from the results
        tree = XMLTree(from_tree = 
            ('hash:objects',
                [('hash:object', # for each object
                    [('hash:ID', ID),
                    ('hash:lines',
                        [('hash:line', # for each line in the object
                            [('hash:section', section),
                            ('hash:property', property),
                            ('hash:value', value)]
                        ) for (section, property, value) in lines]
                    )]
                ) for (ID, lines) in resp]
            ))
        #print tree
        # convert to tree to XML via adding it to the 'getResponse' node
        tree.add_to_node(response_node)
        return out

    def _cond_dict(self, cond):
        # helper method which convert a condition to a simple dictionary
        d = dict(cond)
        return {'type' : d['hash:conditionType'],
            'line' : (d['hash:section'], d['hash:property'], d['hash:value'])}

    def change(self, inpayload):
        """ Make changes in objects, if given conditions are met, return which change was successful.

        change(inpayload)

        'inpayload' is an XMLNode containing the change requests.
        """
        # get the 'changeRequestList' node and convert it to XMLTree
        tree = XMLTree(inpayload.XPathLookup('//hash:changeRequestList', self.hash_ns)[0])
        # create a list of dictionaries from the 'changeRequestElement' nodes
        req_list = tree.get_dicts('/hash:changeRequestList/hash:changeRequestElement',
            {'hash:changeID' : 'changeID',
            'hash:ID' : 'ID', 
            'hash:changeType' : 'type',
            'hash:conditionList' : 'conditions',
            'hash:section' : 'section',
            'hash:property' : 'property',
            'hash:value' : 'value'})
        # make some more conversions
        for req in req_list:
            # create a 3-tuple instead of separate section, property, value elements
            req['line'] = (req['section'], req['property'], req['value'])
            # convert the XMLTree structure to a list of dictionaries
            conditions = [self._cond_dict(cond) for (_, cond) in req.get('conditions',[])]
            # put this list into the dictionary of the request
            req['conditions'] = conditions
        # call the business logic class
        resp = self.hash.change(req_list)
        # prepare the response payload
        out = arc.PayloadSOAP(self.hash_ns)
        # create the 'changeResponse' node
        response_node = out.NewChild('hash:changeResponse')
        # create an XMLTree for the response
        tree = XMLTree(from_tree = 
            ('hash:changeResponseList',
                [('hash:changeResponseElement', # for each change
                    [('hash:chageID', changeID),
                    ('hash:success', success)]
                ) for (changeID, success) in resp]
            ))
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
