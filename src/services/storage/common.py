hash_uri = 'urn:hash'
catalog_uri = 'urn:storagecatalog'
manager_uri = 'urn:storagemanager'
element_uri = 'urn:storageelement'
rbyteio_uri = 'http://schemas.ggf.org/byteio/2005/10/random-access'
byteio_simple_uri = 'http://schemas.ggf.org/byteio/2005/10/transfer-mechanisms/simple'
true = '1'
false = '0'

global_root_guid = '0'
sestore_guid = '1'

def create_checksum(file, type):
    if type == 'md5':
        return _md5sum(file)
    else:
        raise Exception, 'Unsupported checksum type'

import md5

def _md5sum(file):
    m = md5.new()
    while True:
        t = file.read(1024)
        if len(t) == 0: break # end of file
        m.update(t)
    return m.hexdigest()

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

def serialize_ids(ids):
    return ' '.join([str(id) for id in ids])

def parse_ids(s):
    return s.split()

def mkuid():
    import random
    rnd = ''.join([ chr(random.randint(0,255)) for i in range(16) ])
    rnd = rnd[:6] + chr((ord(rnd[6]) & 0x4F) | 0x40) + rnd[7:8]  + chr((ord(rnd[8]) & 0xBF) | 0x80) + rnd[9:]
    uuid = 0L
    for i in range(16):
        uuid = (uuid << 8) + ord(rnd[i])
    rnd_str = '%032x' % uuid
    return '%s-%s-%s-%s-%s' % (rnd_str[0:8], rnd_str[8:12], rnd_str[12:16],  rnd_str[16:20], rnd_str[20:])

def parse_metadata(metadatalist_node):
    metadatalist_number = metadatalist_node.Size()
    metadata = []
    for j in range(metadatalist_number):
        metadata_node = metadatalist_node.Child(j)
        metadata.append((
            (str(metadata_node.Get('section')),str(metadata_node.Get('property'))),
                str(metadata_node.Get('value'))))
    return dict(metadata)

def create_metadata(metadata, prefix = ''):
    if not metadata:
        return []
    if prefix:
        return [
            (prefix + ':metadata', [ 
                (prefix + ':section', section),
                (prefix + ':property', property),
                (prefix + ':value', value)
            ]) for ((section, property), value) in metadata.items()
        ]
    else:
        return [
            ('metadata', [ 
                ('section', section),
                ('property', property),
                ('value', value)
            ]) for ((section, property), value) in metadata.items()
        ]

def node_to_data(node, names, single = False, string = True):
    if string:
        data = [str(node.Get(name)) for name in names]
    else:
        data = [node.Get(name) for name in names]
    if single:
        return data[0], data[1]
    else:
        return data[0], data[1:]

def get_child_nodes(node):
    return [node.Child(i) for i in range(node.Size())]

def parse_node(node, names, single = False, string = True):
    return dict([
        node_to_data(n, names, single, string)
            for n in get_child_nodes(node)
    ])

def parse_to_dict(node, names):
    return dict([(str(n.Get(names[0])), dict([(name, str(n.Get(name))) for name in names[1:]]))
        for n in get_child_nodes(node)])

def create_response(method_name, tag_names, elements, ns, single = False):
    from storage.xmltree import XMLTree
    import arc
    if single:
        tree = XMLTree(from_tree =
            (method_name + 'ResponseList', [
                (method_name + 'ResponseElement', [
                    (tag_names[0], element[0]),
                    (tag_names[1], element[1])
                ]) for element in elements.items()
            ])
        )
    else:
        tree = XMLTree(from_tree =
            (method_name + 'ResponseList', [
                (method_name + 'ResponseElement', [
                    (tag_names[0], element[0])
                ] + [
                    (tag_names[i + 1], element[1][i]) for i in range(len(element[1]))
                ]) for element in elements.items()
            ])
        )
    out = arc.PayloadSOAP(ns)
    response_node = out.NewChild(method_name + 'Response')
    tree.add_to_node(response_node)
    return out

def remove_trailing_slash(LN):
    if LN.endswith('/'):
        LN = LN[:-1]
    return LN

def splitLN(LN):
    parts = LN.split('/')
    rootguid = parts.pop(0)
    try:
        basename = parts.pop()
    except:
        basename = ''
    dirname = '/'.join(parts)
    return rootguid, dirname, basename

import pickle, threading, copy, os, base64, traceback

class PickleStore:
    """ Class for storing object in a serialized python format. """

    def __init__(self, storecfg, non_existent_object = {}):
        """ Constructor of PickleStore.

        PickleStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """
        print "PickleStore constructor called"
        # get the datadir from the storecfg XMLNode
        self.datadir = str(storecfg.Get('DataDir'))
        # set the value which we should return if there an object does not exist
        self.non_existent_object = non_existent_object
        # if the given data directory does not exist, try to create it
        if not os.path.exists(self.datadir):
            os.mkdir(self.datadir)
        print "datadir:", self.datadir
        # initialize a lock which can be used to avoid race conditions
        self.llock = threading.Lock()

    def _filename(self, ID):
        """ Creates a filename from an ID.

        _filename(ID)

        'ID' is the ID of the given object.
        The filename will be the datadir and the base64 encoded form of the ID.
        """
        name = base64.b64encode(ID)
        return os.path.join(self.datadir, name[:2], name)

    def _list(self):
        names = []
        for subdir in os.listdir(self.datadir):
            names.extend(os.listdir(os.path.join(self.datadir, subdir)))
        return names

    def list(self):
        IDs = []
        for name in self._list():
            try:
                ID = base64.b64decode(name)
                IDs.append(ID)
            except:
                print traceback.format_exc()
        return IDs

    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """
        try:
            # generates a filename from the ID
            # then use pickle to load the previously serialized data
            return pickle.load(file(self._filename(ID)))
        except IOError:
            # don't print 'file not found' if there is no such ID
            pass
        except:
            # print whatever exception happened
            print traceback.format_exc()
        # if there was an exception, return the given non_existent_object
        return copy.deepcopy(self.non_existent_object)

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
        if not ID:
            raise Exception, 'ID is empty'
        try:
            # generates a filename from the ID
            fn = self._filename(ID)
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
                pickle.dump(object, file(self._filename(ID),'w'))
        except:
            print traceback.format_exc()
