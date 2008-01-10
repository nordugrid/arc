"""
Hash
----

Centralized prototype implementation of the Hash service.
This service is a metadat store capable of storing (section, property, value) tuples
in objects which have IDs.

Currently supported methods:
    - get
    - change

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
        print 'CentralHash.get(%s)' % ids
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
        """ Change the (section, property, value) members of an object.

        change(changes)

        'changes' is a list of changes
        a change is a dictionary with members such as:
            'changeID' is a reference ID used in the response list
            'ID' is the ID of the object which we want to change
            'changeType' could be 'add', 'remove', 'delete' or 'reset'
                'add' adds a new (section, property, value) line to the object
                'remove' removes a (section, property, value) line from the object
                'delete' removes all values of a corresponding section and property
                    (the value does not matter at this changeType)
                'reset' removes all values of a corresponding section and property
                    then adds the new value which is given
            'section', 'property', 'value' are the chosen ones we praise
        
        This method returns a list of (changeID, success) pairs,
        where success could be:
            - 'added'
            - 'removed'
            - 'deleted'
            - 'reseted'
            - 'failed'
            - 'unknown'
        """
        # prepare the list which will hold the response
        resp = []
        for ch in changes:
            # for each change in the changes list
            print ch
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
                # put the requested section, property and value into one tuple
                line = (ch['section'],ch['property'],ch['value'])
                print lines
                if ch['changeType'] == 'add':
                    # if changeType is add, simply add the new line to the object
                    lines.append(line)
                    print lines
                    # store the changed object
                    self.store.set(ID,lines)
                    # set the result of this change
                    success = 'added'
                elif ch['changeType'] == 'remove':
                    # if changeType is remove, simply remove the given line from the object
                    lines.remove(line) 
                    print lines
                    # store the changed object
                    self.store.set(ID,lines)
                    # set the result of this change
                    success = 'removed'
                elif ch['changeType'] in ['delete','reset']:
                    print 'delete', lines
                    # if it is a 'delete' or a 'reset'
                    # remove all the lines which has the given section and the given property
                    lines = [l for l in lines if not (l[0:2] == line[0:2])]
                    print 'deleted', lines
                    if ch['changeType'] == 'delete':
                        # if this was a 'delete' we are done
                        success = 'removed'
                    else:
                        # if this was a reset, we need to add the new line
                        lines.append(line)
                        print 'reseted', lines
                        success = 'reseted'
                    # both cases we need to store the changed object
                    self.store.set(ID,lines)
                else:
                    # there is no other changeType
                    success = 'invalid changeType'
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

    def addline(self, node, line):
        """ Adds a new line of (section, property, value) to the given XMLNode.

        addline(node, line)

        'node' is an XMLNode which will be the parent of the new node
        'line' is a (section, property, value) tuple we want to add to the node
        """
        # break the line into its three parts
        section, property, value = line
        # add a new child node called 'line'
        line_node = node.NewChild('hash:line')
        # add three child nodes to the 'line' node
        line_node.NewChild('hash:section').Set(section)
        line_node.NewChild('hash:property').Set(property)
        line_node.NewChild('hash:value').Set(value)
    
    def addobject(self, node, obj):
        """ Adds an object with all of its (section, property, value) lines to the given XMLNode.

        addobject(node, obj)

        'node' is the XMLNode which will be the parent of the new node.
        'obj' is a pair of (ID, lines) where ID is the ID of the object,
            and lines a list of (section, property, value) tuples
        """
        # break the object into its two parts
        ID, lines = obj
        # create a new child called 'object'
        object_node = node.NewChild('hash:object')
        # create a child of the 'object' node, call it 'ID' and put the ID in it
        object_node.NewChild('hash:ID').Set(ID)
        # create a child of the 'object' node, call it 'lines'
        lines_node = object_node.NewChild('hash:lines')
        for line in lines:
            # for each (section, proprety, value) line add it to the 'lines' node
            self.addline(lines_node, line)

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
        # create the 'getResponse' node and its child called 'objects'
        objects_node = out.NewChild('hash:getResponse').NewChild('hash:objects')
        for obj in resp:
            # for each object add the results to the 'objects' node
            self.addobject(objects_node, obj)
        return out

    def change(self, inpayload):
        """ Make changes in objects, return which change was successful and which wasn't

        change(inpayload)

        'inpayload' is an XMLNode containing the change requests.
        """
        # extract the change requests with the '//changeRequestElement' XPath expression
        requests = inpayload.XPathLookup('//hash:changeRequestElement', self.hash_ns)
        # prepare an empty list for the results
        req_list = []
        for request in requests:
            # for each request convert the XML structure to a dictionary structure
            # start with an empty dictionary
            req_dict = {}
            # these are the valid keys
            for key in ['changeID', 'ID', 'changeType', 'section', 'property', 'value']:
                # get the data from the XMLNode and put it into our dictionary
                req_dict[key] = str(request.Get(key))
            # append it to the list of the request dictionaries
            req_list.append(req_dict)
        # get the results from the business logic class
        resp = self.hash.change(req_list)
        # prepare the response payload
        out = arc.PayloadSOAP(self.hash_ns)
        # add the 'changeResponse' node and its child called 'changeResponseList'
        responses_node = out.NewChild('hash:changeResponse').NewChild('hash:changeResponseList')
        for (changeID, success) in resp:
            # for each result create a new child called 'changeResponseElement'
            response_node = responses_node.NewChild('hash:changeResponseElement')
            # and put the changeID and success into it
            response_node.NewChild('changeID').Set(changeID)
            response_node.NewChild('success').Set(success)
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
            print request_name
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
            print traceback.format_exc()
            # set an empty outgoing payload
            outmsg.Payload(arc.PayloadSOAP(self.hash_ns))
            # return with the status GENERIC_ERROR
            return arc.MCC_Status(arc.GENERIC_ERROR)
