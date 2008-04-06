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
            <HashClass>storage.hash.hash.CentralHash</HashClass>
            <StoreClass>storage.common.PickleStore</StoreClass>
            <StoreCfg><DataDir>hash_data</DataDir></StoreCfg>
        </Service>
"""
import arc
import traceback
from storage.common import import_class_from_string, hash_uri, node_to_data, create_metadata, get_child_nodes

class CentralHash:
    """ A centralized implementation of the Hash service. """

    def __init__(self, storeclass, storecfg):
        """ The constructor of the CentralHash class.

        CentralHash(storeclass, storecfg)

        'storeclass' is the name of the class which will store the data
        'storecfg' is an XMLNode with the configuration of the storeclass
        """
        print "CentralHash constructor called"
        # import the storeclass and call its constructor with the datadir
        self.store = import_class_from_string(storeclass)(storecfg)

    def get(self, ids, neededMetadata = []):
        """ Gets all data of the given IDs.

        get(ids)

        'ids' is a list of strings: the IDs of the requested object
        Returns a dictionary of { ID : metadata }
            where 'metadata' is a dictionary where the keys are ('section', 'property') tuples
        """
        if neededMetadata: # if the needed metadata is given
            # if a property is empty in a (section, property) pair
            # it means we need all properties from that section
            allpropsections = [section for (section, property) in neededMetadata if property == '']
            # gets the metadata for each ID, filters it and creates an {ID : metadata} dictionary
            return dict([(
                ID,
                dict([((section, property), value) # for all metadata entry of this object
                    for (section, property), value in self.store.get(ID).items()
                        # if this (section, property) is needed or if this section needs all the properties
                        if (section, property) in neededMetadata or section in allpropsections])
            ) for ID in ids])
        else: # gets the metadata for each ID, and creates an {ID : metadata} dictionary
            return dict([(ID, self.store.get(ID)) for ID in ids])

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
        # prepare the dictionary which will hold the response
        response = {}
        for (changeID, (ID, changeType, section, property, value, conditionList)) in changes.items():
            # for each change in the changes list
            # lock the store to avoid inconsistency
            while not self.store.lock(blocking = False):
                print 'Hash cannot acquire lock, waiting...'
                time.sleep(0.2)
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
                        self.store.set(ID, obj)
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
            # append the result of the change to the response list
            response[changeID] = (success, unmetConditionID)
        return response

from storage.xmltree import XMLTree

class HashService:
    """ HashService class implementing the XML interface of the Hash service. """

    def __init__(self, cfg):
        """ Constructor of the HashService

        HashService(cfg)

        'cfg' is an XMLNode which containes the config of this service.
        """
        print "HashService constructor called"
        # the name of the class which implements the business logic of the Hash service
        hashclass = str(cfg.Get('HashClass'))
        # the name of the class which is capable of storing the object
        storeclass = str(cfg.Get('StoreClass'))
        # this is a directory for storing object data
        storecfg = cfg.Get('StoreCfg')
        # import and instatiate the business logic class
        self.hash = import_class_from_string(hashclass)(storeclass, storecfg)
        # set the default namespace for the Hash service
        self.hash_ns = arc.NS({'hash':hash_uri})

    def get(self, inpayload):
        """ Returns the data of the requested objects.

        get(inpayload)

        'inpayload' is an XMLNode containing the IDs of the requested objects
        """
        # extract the IDs from the XMLNode using the '//ID' XPath expression
        ids = [str(id) for id in inpayload.XPathLookup('//hash:ID', self.hash_ns)]
        # get the neededMetadata from the XMLNode
        try:
            neededMetadata = [
                node_to_data(node, ['section', 'property'], single = True)
                    for node in get_child_nodes(inpayload.Child().Get('neededMetadataList'))
            ]
        except:
            print traceback.format_exc()
            neededMetadata = []
        # gets the result from the business logic class
        objects = self.hash.get(ids, neededMetadata)
        # create the response payload
        out = arc.PayloadSOAP(self.hash_ns)
        # create the 'getResponse' node
        response_node = out.NewChild('hash:getResponse')
        # create an XMLTree from the results
        tree = XMLTree(from_tree = 
            ('hash:objects', [
                ('hash:object', [ # for each object
                    ('hash:ID', ID),
                    # create the metadata section of the response:
                    ('hash:metadataList', create_metadata(metadata, 'hash'))
                ]) for (ID, metadata) in objects.items()
            ])
        )
        # convert to tree to XML via adding it to the 'getResponse' node
        tree.add_to_node(response_node)
        return out

    def change(self, inpayload):
        """ Make changes in objects, if given conditions are met, return which change was successful.

        change(inpayload)

        'inpayload' is an XMLNode containing the change requests.
        """
        # get the grandchild of the root node, which is the 'changeRequestList'
        requests_node = inpayload.Child().Child()
        # get all the requests:
        request_nodes = [requests_node.Child(i) for i in range(requests_node.Size())]
        # prepare the dictionary which will hold the requests
        changes = {}
        # for each request:
        for request_node in request_nodes:
            # get all the data: changeID, ID, changeType, section, property, value, conditions
            changeID, change = node_to_data(request_node,
                ['changeID', 'ID', 'changeType', 'section', 'property', 'value'])
            # get the conditionList node
            conditionList_node = request_node.Get('conditionList')
            # get the nodes of all the conditions
            condition_nodes = [conditionList_node.Child(i) for i in range(conditionList_node.Size())]
            # for each condition get all the data: conditionID, conditionType, section, property, value
            conditions = dict([
                node_to_data(condition_node, ['conditionID', 'conditionType', 'section', 'property', 'value'])
                    for condition_node in condition_nodes
            ])
            # append the conditions dict to the chnage request
            change.append(conditions)
            # put this change request into the changes dictionary
            changes[changeID] = change
        # call the business logic class
        resp = self.hash.change(changes)
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
            print '     hash.%s called' % request_name
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

    # names of provided methods
    request_names = ['get','change']

