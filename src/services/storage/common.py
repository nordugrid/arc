hash_uri = 'urn:hash'
catalog_uri = 'urn:storagecatalog'
manager_uri = 'urn:storagemanager'
true = '1'
false = '0'

global_root_guid = '0'

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

def remove_trailing_slash(LN):
    if LN.endswith('/'):
        LN = LN[:-1]
    return LN

def dirname(LN):
    try:
        return LN[:LN.rindex('/')]
    except:
        return ''

def basename(LN):
    try:
        return LN[(LN.rindex('/')+1):]
    except:
        return LN
