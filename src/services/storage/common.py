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

def create_metadata(metadata, prefix = ''):
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
