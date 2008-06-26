import xml.dom.minidom

class FakeXMLNode(object):
    def __init__(self, from_string = '', from_node = None, doc_node = None):
        if from_node:
            self.node = from_node
            self.doc = doc_node
        else:
            self.doc = xml.dom.minidom.parseString(from_string)
            self.node = self.doc.childNodes[0]
            
    def __str__(self):
        """docstring for __str__"""
        text = ''.join([child.nodeValue for child in self.node.childNodes if child.nodeType == child.TEXT_NODE])
        return text
    
    def _get_real_children(self):
        """docstring for _get_real_children"""
        return [child for child in self.node.childNodes if child.nodeType != child.TEXT_NODE]
    
    def Size(self):
        """docstring for Size"""
        return len(self._get_real_children())
    
    def Child(self, no = 0):
        """docstring for Child"""
        return FakeXMLNode(from_node = self._get_real_children()[no], doc_node = self.doc)
    
    def FullName(self):
        """docstring for FullName"""
        return self.node.nodeName
        
    def Name(self):
        """docstring for Name"""
        return self.node.localName

    def Prefix(self):
        """docstring for Prefix"""
        return self.node.prefix
        
    def GetXML(self):
        """docstring for GetXML"""
        return self.node.toxml()
        
    def Get(self, tagname):
        """docstring for Get"""
        return [FakeXMLNode(from_node = child, doc_node = self.doc) for child in self.node.childNodes if child.nodeName == tagname or child.localName == tagname][0]
        
    def NewChild(self, tagname):
        """docstring for NewChild"""
        return FakeXMLNode(from_node = self.node.appendChild(self.doc.createElement(tagname)), doc_node = self.doc)
    
    def Set(self, text_data):
        """docstring for Set"""
        self.node.appendChild(self.doc.createTextNode(text_data))            