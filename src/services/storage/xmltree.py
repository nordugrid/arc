""" The XMLTree class provides a way to convert from XML to native python structures and vica versa.

Examples
--------

if you have an XMLNode:

>>> x = arc.XMLNode('<soap-env:Envelope xmlns:hash="urn:hash" \
xmlns:soap-enc="http://schemas.xmlsoap.org/soap/encoding/" \
xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/" \
xmlns:xsd="http://www.w3.org/2001/XMLSchema" \
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">\
<soap-env:Body>\
<hash:get>\
<hash:IDs>\
<hash:ID>0</hash:ID>\
<hash:ID>1</hash:ID>\
<hash:ID>2</hash:ID>\
</hash:IDs>\
</hash:get>\
</soap-env:Body>\
</soap-env:Envelope>')

you can convert it to an XMLTree:

>>> t = XMLTree(x)
>>> t.get()
[('soap-env:Envelope',
  [('soap-env:Body',
      [('hash:get',
            [('hash:IDs',
                    [('hash:ID', '0'), ('hash:ID', '1'), ('hash:ID', '2')])])])])]

you can specify a path:

>>> t.get('/soap-env:Envelope/soap-env:Body/hash:get')
[('hash:get', [('hash:IDs', [('hash:ID', '0'), ('hash:ID', '1'), ('hash:ID', '2')])])]

this is not an XPath, it is just a plain path
empty tagname matches to everything, so these are the same as the previous example:

>>> t.get('/soap-env:Envelope//hash:get')
[('hash:get', [('hash:IDs', [('hash:ID', '0'), ('hash:ID', '1'), ('hash:ID', '2')])])]

>>> t.get('///hash:get')
[('hash:get', [('hash:IDs', [('hash:ID', '0'), ('hash:ID', '1'), ('hash:ID', '2')])])]

there are some other query methods, e.g. get_value and get_values:

>>> t.get('/////hash:ID')
[('hash:ID', '0'), ('hash:ID', '1'), ('hash:ID', '2')]

>>> t.get_value('/////hash:ID')
'0'

>>> t.get_values('/////hash:ID')
['0', '1', '2']

if you have an XML with key-value pairs, e.g.:

>>> t = XMLTree(from_string = '<root><object><key1>value1</key1><key2>value2</key2></object>\
    <object><key1>value3</key1><key2>value4</key2></object></root>')
>>> t.get()
[('root',
  [('object', [('key1', 'value1'), ('key2', 'value2')]),
     ('object', [('key1', 'value3'), ('key2', 'value4')])])]

now you can use the get_dict and get_dicts methods:

>>> t.get('/root/object')
[('object', [('key1', 'value1'), ('key2', 'value2')]),
('object', [('key1', 'value3'), ('key2', 'value4')])]

>>> t.get_dict('/root/object')
{'key1': 'value1', 'key2': 'value2'}

>>> t.get_dicts('/root/object')
[{'key1': 'value1', 'key2': 'value2'}, {'key1': 'value3', 'key2': 'value4'}]

you can specify the needed keys, and rename them:

>>> t.get_dicts('/root/object', {'key1':'new name'})
[{'new name': 'value1'}, {'new name': 'value3'}]


you can specify default value with get_value:

>>> t.get_value('///key1','default value')
'value1'
>>> t.get_value('///key3','default value')
'default value'

you can add an XMLTree to an XMLNode with the add_to_node method:

>>> x = XMLNode('<start/>')
>>> x.GetXML()
'<start/>'
>>> t.get('/root/object')
[('object', [('key1', 'value1'), ('key2', 'value2')]),
 ('object', [('key1', 'value3'), ('key2', 'value4')])]
>>> t.add_to_node(x,'/root/object')
>>> x.GetXML()
'<start><object><key1>value1</key1><key2>value2</key2></object>\
    <object><key1>value3</key1><key2>value4</key2></object></start>'

you can create an XMLTree from the tree structure:

>>> t2 = XMLTree(from_tree = ('object', [('key1', 'value5'), ('key2', 'value6')]))
>>> t2.get()
[('object', [('key1', 'value5'), ('key2', 'value6')])]

or you can add a new subtree to an XMLTree:

>>> t2.add_tree(('key3','valuex'),'/object')
>>> t2.get()
[('object', [('key1', 'value5'), ('key2', 'value6'), ('key3', 'valuex')])]

this will actually add it to the first node which matches the path, e.g.:

>>> t.get('/root/object')
[('object', [('key1', 'value1'), ('key2', 'value2')]),
 ('object', [('key1', 'value3'), ('key2', 'value4')])]

>>> t.add_tree(('key3','valuex'),'/root/object')
>>> t.get()
[('root',
  [('object', [('key1', 'value1'), ('key2', 'value2'), ('key3', 'valuex')]),
     ('object', [('key1', 'value3'), ('key2', 'value4')])])]

you can create list of subtrees with the get_trees method:

>>> t.get_trees('/root/object')
[<hash.xmltree.XMLTree instance at 0x17a6300>,
 <hash.xmltree.XMLTree instance at 0x17a6558>]

the str() method gives a string representation of an XMLTree:

>>> [str(i) for i in t.get_trees('/root/object')]
 ["('object', [('key1', 'value1'), ('key2', 'value2'), ('key3', 'valuex')])",
  "('object', [('key1', 'value3'), ('key2', 'value4')])"]

finally, you can create complex XML structures easily with XMLTree:
(this example is from the 'get' method of the hash.HashService class,
the 'resp' is a list of (ID, object) pairs,
where 'object' is a list of (section, property, value) tuples)

# create the 'getResponse' node and its child called 'objects'
response_node = out.NewChild('hash:getResponse')
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
print tree
# convert to tree to XML via adding it to the 'getResponse' node
tree.add_to_node(response_node)

this generates an XML like this:

<hash:getResponse>
    <hash:objects>
        <hash:object>
            <hash:ID>0</hash:ID>
                <hash:lines>
                    <hash:line>
                        <hash:section>1</hash:section>
                        <hash:property>2</hash:property>
                        <hash:value>3</hash:value>
                    </hash:line>
                    <hash:line>
                        <hash:section>a</hash:section>
                        <hash:property>b</hash:property>
                        <hash:value>c</hash:value>
                    </hash:line>
                    <hash:line>
                        <hash:section>su</hash:section>
                        <hash:property>bi</hash:property>
                        <hash:value>du</hash:value>
                </hash:line>
            </hash:lines>
        </hash:object>
    </hash:objects>
</hash:getResponse>
"""

from arc import XMLNode

class XMLTree:
    def __init__(self, from_node = None, from_string = '', from_tree = None, rewrite = {}, forget_namespace = False):
        """ Constructor of the XMLTree class

        XMLTree(from_node = None, from_string = '', from_tree = None)

        'from_tree' could be tree structure or an XMLTree object
        'from_string' could be an XML string
        'from_node' could be an XMLNode

        'from_tree' has the highest priority, if it is not None,
            then the other two is ignored.
        If 'from_tree' is None but from_string is given, then from_node is ignored.
        If only 'from_node' is given, then it will be the choosen one.
        In this case you may simply use:
            tree = XMLTree(node)
        """
        if from_tree:
            # if a tree structure is given, set the internal variable with it
            # if this is an XMLTree object, get just the data from it
            if isinstance(from_tree,XMLTree):
                self._data = from_tree._data
            else: 
                self._data = from_tree
        else:
            if from_node:
                # if no from_tree is given, and we have an XMLNode, just save it
                x = from_node
            else:
                # if no from_tree and from_node is given, try to parse the string
                x = XMLNode(from_string)
            # set the internal tree structure to (<name of the root node>, <rest of the document>)
            # where <rest of the document> is a list of the child nodes of the root node
            self._data = (self._getname(x, rewrite, forget_namespace), self._dump(x, rewrite, forget_namespace))

    def _getname(self, node, rewrite = {}, forget_namespace = False):
        # gets the name of an XMLNode, with namespace prefix if it has one
        if not forget_namespace and node.Prefix():
            name = node.FullName()
        else: # and without namespace prefix if it has no prefix
            name = node.Name()
        return rewrite.get(name,name)

    def _dump(self, node, rewrite = {}, forget_namespace = False):
        # recursive method for converting an XMLNode to XMLTree structure
        size = node.Size() # get the number of children of the node
        if size == 0: # if it has no child, get the string
            return str(node)
        children = [] # collect the children
        for i in range(size):
            children.append(node.Child(i))
        # call itself recursively for each children
        return [(self._getname(n, rewrite, forget_namespace), self._dump(n, rewrite, forget_namespace)) for n in children ]

    def add_to_node(self, node, path = None):
        """ Adding a tree structure to an XMLNode.

        add_to_node(node, path = None)
        
        'node' is the XMLNode we want to add to
        'path' selects the part of the XMLTree we want to add
        """
        # selects the part we want
        data = self.get(path)
        # call the recursive helping method
        self._add_to_node(data, node)

    def _add_to_node(self, data, node):
        # recursively add the tree structure to the node
        for element in data:
            # we want to avoid empty tags in XML
            if element[0]:
                # for each child in the tree create a child in the XMLNode
                child_node = node.NewChild(element[0])
                # if the node has children:
                if isinstance(element[1],list):
                    self._add_to_node(element[1], child_node)
                else: # if it has no child, create a string from it
                    child_node.Set(str(element[1]))

    def __str__(self):
        return str(self._data)

    def _traverse(self, path, data):
        # helping function for recursively traverse the tree
        # path is a list of the node names, e.g. ['root','key1']
        # data is the date of a tree-node,
        # e.g. ('root', [('key1', 'value'), ('key2', 'value')])
        # if the first element of the path and the name of the node is equal
        #   or if the element of the path is empty, it matches all node names
        # if not, then we have no match here, return an empty list
        if path[0] != data[0] and path[0] != '':
            return []
        # if there are no more path-elements, then we are done
        # we've just found what we looking for
        if len(path) == 1:
            return [data]
        # if there are more path-elements, but this is a string node
        # then no luck, we cannot proceed, return an empty list
        if isinstance(data[1],str):
            return []
        # if there are more path-elements, and this node has children
        ret = []
        for d in data[1]:
            # let's recurively ask all child if they have any matches
            # and collect the matches
            ret.extend( self._traverse(path[1:], d) )
        # return the matches
        return ret

    def get(self, path = None):
        """ Returns the parts of the XMLTree which match the path.

        get(path = None)

        if 'path' is not given, it defaults to the root node
        """
        if path: # if path is given
            # if it is not starts with a slash
            if not path.startswith('/'):
                raise Exception, 'invalid path (%s)' % path
            # remove the starting slash
            path = path[1:]
            # split the path to a list of strings
            path = path.split('/')
        else: # if path is not given
            # set it to the root node
            path = [self._data[0]]
        # gets the parts which are selected by this path
        return self._traverse(path, self._data)

    def get_trees(self, path = None):
        """ Returns XMLTree object for each subtree which match the path.

        get_tress(path = None)
        """
        # get the parts match the path and convert them to XMLTree
        return [XMLTree(from_tree = t) for t in self.get(path)]

    def get_value(self, path = None, *args):
        """ Returns the value of the selected part.

        get_value(path = None, [default])

        Returns the value of the node first matched the path.
        This is one level deeper than the value returned by the 'get' method.
        If there is no such node, and a default is given,
        it will return the default.
        """
        try:
            # use the get method then get the value of the first result
            return self.get(path)[0][1]
        except:
            # there was an error
            if args: # if any more argumentum is given
                # the first will be the default
                return args[0]
            raise

    def add_tree(self, tree, path = None):
        """ Add a new subtree to a path.

        add_tree(tree, path = None)
        """
        # if this is a real XMLTree object, get just the data from it
        if isinstance(tree,XMLTree):
            tree = tree._data
        # get the first node selected by the path and append the new subtree to it
        self.get(path)[0][1].append(tree)
    
    def get_values(self, path = None):
        """ Get all the values selected by a path.

        get_values(path = None)

        Like get_value but gets all values not just the first
        This has no default value.
        """
        try:
            # get just the value of each node
            return [d[1] for d in self.get(path)]
        except:
            return []

    def _dict(self, value, keys):
        # helper method for changing keys
        if keys:
            # if keys is given use only the keys which is in it
            # and translete them to new keys (the values of the 'keys' dictionary)
            return dict([(keys[k],v) for (k,v) in value if k in keys.keys()])
        else: # if keys is empty, use all the data
            return dict(value)
    
    def get_dict(self, path = None, keys = {}):
        """ Returns a dictionary from the first node the path matches.

        get_dict(path, keys = {})

        'keys' is a dictionary which filters and translate the keys
            e.g. if keys is {'hash:line':'line'}, it will only return
            the 'hash:line' nodes, and will call them 'line'
        """
        return self._dict(self.get_value(path,[]),keys)

    def get_dicts(self, path = None, keys = {}):
        """ Returns a list of dictionaries from all the nodes the path matches.

        get_dicts(path, keys = {})

        'keys' is a dictionary which filters and translate the keys
            e.g. if keys is {'hash:line':'line'}, it will only return
            the 'hash:line' nodes, and will call them 'line'
        """
        return [self._dict(v,keys) for v in self.get_values(path)]
